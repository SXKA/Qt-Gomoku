#include  "transpositiontable.h"

using namespace Zobrist;

TranspositionTable::TranspositionTable() : TranspositionTable(65536)
{
};

TranspositionTable::TranspositionTable(const int &size)
    : innerTable(QVarLengthArray<HashEntry>(size))
    , outerTable(QVarLengthArray<HashEntry>(size))
    , checkSum(0)
    , mask(size - 1)
{
    std::random_device device;
    std::default_random_engine engine(device());
    std::uniform_int_distribution<unsigned long long> distribution;

    for (int i = 0; i < size; ++i) {
        innerTable[i] = HashEntry{0, HashEntry::Empty, 0, 0, {-1, -1}};
        outerTable[i] = HashEntry{0, HashEntry::Empty, 0, 0, {-1, -1}};
    }

    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            blackRandomTable[i][j] = distribution(engine);
            whiteRandomTable[i][j] = distribution(engine);
        }
    }
}

void TranspositionTable::insert(const unsigned long long &hashKey, const HashEntry::Type &type,
                                const int &depth, const int &score, const QPoint &point)
{
    const auto &index = hashKey & mask;
    auto &innerEntry = innerTable[index];
    auto &outerEntry = outerTable[index];

    if (outerEntry.depth <= depth) {
        innerEntry = outerEntry;
        outerEntry.lock = hashKey;
        outerEntry.type = type;
        outerEntry.depth = depth;
        outerEntry.score = score;
        outerEntry.move = point;
    } else {
        innerEntry.lock = hashKey;
        innerEntry.type = type;
        innerEntry.depth = depth;
        innerEntry.score = score;
        innerEntry.move = point;
    }
}

void TranspositionTable::transpose(const QPoint &point, const Gomoku::Stone &stone)
{
    const auto &randomTable = stone == Gomoku::Black ? blackRandomTable : whiteRandomTable;

    checkSum ^= randomTable[point.x()][point.y()];
}

unsigned long long TranspositionTable::hash() const
{
    return checkSum;
}

int TranspositionTable::probe(const unsigned long long &hashKey, const int &alpha, const int &beta,
                              const int &depth, QPair<QPoint, QPoint> &pair) const
{
    HashEntry entry{0, HashEntry::Empty, 0, 0, {-1, -1}};
    const auto &index = hashKey & mask;
    const auto &innerEntry = innerTable[index];
    const auto &outerEntry = outerTable[index];

    if (outerEntry.type != HashEntry::Empty && outerEntry.lock == hashKey) {
        if (outerEntry.depth >= depth) {
            entry = outerEntry;
        }

        pair.first = outerEntry.move;
    }

    if (innerEntry.type != HashEntry::Empty && innerEntry.lock == hashKey) {
        if (entry.type == HashEntry::Empty && innerEntry.depth >= depth) {
            entry = innerEntry;
        }

        pair.second = innerEntry.move;
    }

    switch (entry.type) {
    case HashEntry::Empty:
        break;
    case HashEntry::Exact:
        return entry.score;
    case HashEntry::LowerBound:
        if (entry.score >= beta) {
            return entry.score;
        }

        break;
    case HashEntry::UpperBound:
        if (entry.score <= alpha) {
            return entry.score;
        }

        break;
    }

    return MISS;
}
