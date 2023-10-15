#include  "transpositiontable.h"

using namespace Zobrist;

TranspositionTable::TranspositionTable() : TranspositionTable(1048576)
{
};

TranspositionTable::TranspositionTable(const int &size)
    : hashTable(QVarLengthArray<HashEntry>(size))
{
    std::random_device device;
    std::default_random_engine engine(device());
    std::uniform_int_distribution<unsigned long long> distribution;

    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            blackRandomTable[i][j] = distribution(engine);
            whiteRandomTable[i][j] = distribution(engine);
        }
    }

    boardHash = distribution(engine);
}

void TranspositionTable::insert(const unsigned long long &hashKey, const HashEntry::Type &type,
                                const int &depth,
                                const int &score)
{
    auto &entry = hashTable[hashKey & (hashTable.size() - 1)];

    if (entry.type == HashEntry::Empty && entry.depth <= depth) {
        entry.checkSum = hashKey;
        entry.type = type;
        entry.depth = depth;
        entry.score = score;
    }
}

bool TranspositionTable::contains(const unsigned long long &hashKey, const int &depth) const
{
    auto &entry = hashTable[hashKey & (hashTable.size() - 1)];

    return entry.checkSum == hashKey && entry.depth >= depth;
}

unsigned long long TranspositionTable::hash() const
{
    return boardHash;
}

unsigned long long TranspositionTable::transpose(const QPoint &point, const Gomoku::Stone &stone)
{
    const auto randomTable = stone == Gomoku::Black ? blackRandomTable : whiteRandomTable;

    boardHash ^= randomTable[point.x()][point.y()];

    return boardHash;
}

HashEntry TranspositionTable::at(const unsigned long long &hashKey) const
{
    return hashTable[hashKey & (hashTable.size() - 1)];
}
