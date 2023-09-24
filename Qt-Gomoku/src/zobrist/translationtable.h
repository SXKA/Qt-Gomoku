#ifndef TRANSLATIONTABLE_H
#define TRANSLATIONTABLE_H

#include "../gomoku/stone.h"
#include <QPoint>
#include <QVarLengthArray>
#include <array>
#include <random>

namespace Zobrist {
struct hashEntry {
    unsigned long long checkSum;
    enum Type { Empty, Exact, LowBound, UpperBound } type;
    int score;
    int depth;
};

class TranslationTable
{
private:
    std::array<std::array<unsigned long long, 15>, 15> blackRandomTable;
    std::array<std::array<unsigned long long, 15>, 15> whiteRandomTable;
    QVarLengthArray<hashEntry> hashTable;
    unsigned long long boardHash;
public:
    TranslationTable();
    TranslationTable(const int &size);
    void insert(const unsigned long long &hashKey, const hashEntry::Type &type, const int &depth,
                const int &score);
    bool contains(const unsigned long long &hashKey, const int &depth) const;
    unsigned long long hash() const;
    unsigned long long translate(const QPoint &point, const Gomoku::Stone &stone);
    hashEntry at(const unsigned long long &hashKey) const;
};
}

#endif
