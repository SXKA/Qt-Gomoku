#ifndef TRANSPOSITIONTABLE_H
#define TRANSPOSITIONTABLE_H

#include "../gomoku/stone.h"
#include <QPoint>
#include <QVarLengthArray>
#include <array>
#include <random>
#include <set>

namespace Zobrist {
constexpr auto MISS = INT_MAX;

struct HashEntry {
    unsigned long long lock;
    enum Type { Empty, Exact, LowerBound, UpperBound } type;
    int score;
    int depth;
    QPoint move;
};

class TranspositionTable
{
private:
    QVarLengthArray<HashEntry> innerTable;
    QVarLengthArray<HashEntry> outerTable;
    std::array<std::array<unsigned long long, 15>, 15> blackRandomTable;
    std::array<std::array<unsigned long long, 15>, 15> whiteRandomTable;
    unsigned long long checkSum;
    long long mask;
public:
    TranspositionTable();
    TranspositionTable(const int &size);
    void insert(const unsigned long long &hashKey, const HashEntry::Type &type, const int &depth,
                const int &score, const QPoint &point);
    void transpose(const QPoint &point, const Gomoku::Stone &stone);
    unsigned long long hash() const;
    int probe(const unsigned long long &hashKey, const int &alpha, const int &beta, const int &depth,
              QPair<QPoint, QPoint> &pair) const;
};
}

#endif
