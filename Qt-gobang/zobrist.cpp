#include  "zobrist.h"

using namespace zobrist;

Zobrist::Zobrist() : Zobrist(65536)
{
};

Zobrist::Zobrist(const int &size)
{
    blackRandomTable = std::array<std::array<unsigned long long, 15>, 15>();
    whiteRandomTable = std::array<std::array<unsigned long long, 15>, 15>();

    std::random_device device;
    std::default_random_engine engine(device());
    std::uniform_int_distribution<unsigned long long> distribution;

    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            blackRandomTable[i][j] = distribution(engine);
            whiteRandomTable[i][j] = distribution(engine);
        }
    }

    hashTable = std::vector<hashEntry>(size);
    boardHash = distribution(engine);
}

void Zobrist::insert(const unsigned long long &hashKey, const type &type, const int &depth,
                     const int &score)
{
    auto &entry = hashTable[hashKey & (hashTable.size() - 1)];

    if (entry.type == type::empty && entry.depth <= depth) {
        entry.checkSum = hashKey;
        entry.type = type;
        entry.depth = depth;
        entry.score = score;
    }
}

bool Zobrist::contains(const unsigned long long &hashKey, const int &depth) const
{
    auto &entry = hashTable[hashKey & (hashTable.size() - 1)];

    return entry.type != type::empty && entry.checkSum == hashKey && entry.depth >= depth;
}

unsigned long long Zobrist::hash() const
{
    return boardHash;
}

unsigned long long Zobrist::translate(const QPoint &point, const bool &color)
{
    const auto randomTable = color == false ? blackRandomTable : whiteRandomTable;

    boardHash ^= randomTable[point.x()][point.y()];

    return boardHash;
}

hashEntry Zobrist::at(const unsigned long long &hashKey) const
{
    return hashTable[hashKey & (hashTable.size() - 1)];
}
