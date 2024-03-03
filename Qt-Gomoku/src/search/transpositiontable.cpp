#include  "transpositiontable.h"

#include <random>

using namespace Search;

TranspositionTable::TranspositionTable() : TranspositionTable((1 << 24) / sizeof(
                                                                      std::array<HashEntry, 8>))
{
};

TranspositionTable::TranspositionTable(const size_t &size)
    : hashTable(QVarLengthArray<std::array<HashEntry, 8>>(size))
    , mask(size - 1)
    , checkSum(0)
    , age(0)
{
    std::random_device device;
    std::default_random_engine engine(device());
    std::uniform_int_distribution<unsigned long long> distribution;

    for (size_t i = 0; i <= mask; ++i) {
        hashTable[i].fill(HashEntry{0, HashEntry::Exact, {-1, -1}, 0, 0, MISS});
    }

    for (size_t i = 0; i < 15; ++i) {
        for (size_t j = 0; j < 15; ++j) {
            blackRandomTable[i][j] = distribution(engine);
            whiteRandomTable[i][j] = distribution(engine);
        }
    }
}

void TranspositionTable::insert(const unsigned long long &hashKey, const HashEntry::Type &type,
                                const QPoint &move,
                                const int &depth, const int &score)
{
    const auto index = hashKey & mask;
    auto &entries = hashTable[index];
    auto *replacement = &entries.front();

    for (auto &entry : entries) {
        if (entry.lock == hashKey) {
            replacement = &entry;

            break;
        }

        if (entry.depth - (age - entry.age) < replacement->depth - (age - replacement->age)) {
            replacement = &entry;
        }
    }

    if (type != HashEntry::Exact && depth + 2 < replacement->depth) {
        return;
    }

    replacement->lock = hashKey;
    replacement->type = type;
    replacement->move = move == QPoint(-1, -1) ? replacement->move : move;
    replacement->depth = depth;
    replacement->score = score;
    replacement->age = age;
}

void TranspositionTable::aging()
{
    ++age;
}

void TranspositionTable::transpose(const QPoint &point, const Stone &stone)
{
    const auto &randomTable = stone == Black ? blackRandomTable : whiteRandomTable;

    checkSum ^= randomTable[point.x()][point.y()];
}

unsigned long long TranspositionTable::hash() const
{
    return checkSum;
}

int TranspositionTable::probe(const unsigned long long &hashKey, const int &alpha, const int &beta,
                              const int &depth, QPoint &move)
{
    const auto index = hashKey & mask;
    auto &entries = hashTable[index];

    for (auto &[entryLock, entryType, entryMove, entryAge, entryDepth, entryScore] : entries) {
        if (entryLock == hashKey) {
            move = entryMove;

            entryAge = age;

            if (entryDepth >= depth) {
                switch (entryType) {
                case HashEntry::Exact:
                    return entryScore;
                case HashEntry::LowerBound:
                    if (entryScore >= beta) {
                        return entryScore;
                    }

                    break;
                case HashEntry::UpperBound:
                    if (entryScore <= alpha) {
                        return entryScore;
                    }

                    break;
                }
            }
        }
    }

    return MISS;
}
