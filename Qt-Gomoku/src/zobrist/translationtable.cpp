#include  "TranslationTable.h"

using namespace zobrist;

TranslationTable::TranslationTable() : TranslationTable(65536)
{
};

TranslationTable::TranslationTable(const int &size) : hashTable(QVarLengthArray<hashEntry>(size))
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

void TranslationTable::insert(const unsigned long long &hashKey, const hashEntry::Type &type,
                              const int &depth,
                              const int &score)
{
    auto &entry = hashTable[hashKey & (hashTable.size() - 1)];

    if (entry.type == hashEntry::empty && entry.depth <= depth) {
        entry.checkSum = hashKey;
        entry.type = type;
        entry.depth = depth;
        entry.score = score;
    }
}

bool TranslationTable::contains(const unsigned long long &hashKey, const int &depth) const
{
    auto &entry = hashTable[hashKey & (hashTable.size() - 1)];

    return entry.type != hashEntry::empty && entry.checkSum == hashKey && entry.depth >= depth;
}

unsigned long long TranslationTable::hash() const
{
    return boardHash;
}

unsigned long long TranslationTable::translate(const QPoint &point, const bool &color)
{
    const auto randomTable = color == false ? blackRandomTable : whiteRandomTable;

    boardHash ^= randomTable[point.x()][point.y()];

    return boardHash;
}

hashEntry TranslationTable::at(const unsigned long long &hashKey) const
{
    return hashTable[hashKey & (hashTable.size() - 1)];
}
