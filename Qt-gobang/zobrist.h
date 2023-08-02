#pragma once

#include <array>
#include <random>
#include <QPoint>

namespace zobrist {
enum type { empty, exact, lowerBound, upperBound };

struct hashEntry {
    unsigned long long checkSum;
    type type;
    int score;
    int depth;
};

class Zobrist
{
public:
    Zobrist();
    Zobrist(const int &size);
    void insert(const unsigned long long &hashKey, const type &type, const int &depth,
                const int &score);
    bool contains(const unsigned long long &hashKey, const int &depth) const;
    unsigned long long hash() const;
    unsigned long long translate(const QPoint &point, const bool &color);
    hashEntry at(const unsigned long long &hashKey) const;

private:
    std::array<std::array<unsigned long long, 15>, 15> blackRandomTable;
    std::array<std::array<unsigned long long, 15>, 15> whiteRandomTable;
    std::vector<hashEntry> hashTable;
    unsigned long long boardHash;
};
}