#ifndef TRANSPOSITIONTABLE_H
#define TRANSPOSITIONTABLE_H

#include "../core/types.h"

#include <QPoint>
#include <QVarLengthArray>

#include <array>

namespace Search {
constexpr auto MISS = INT_MAX;

struct HashEntry
{
    unsigned long long lock{};
    enum Type { Exact, LowerBound, UpperBound } type{};
    QPoint move{};
    int generation{};
    int depth{};
    int score{};
    Stone stone{};
};

class TranspositionTable
{
private:
    QVarLengthArray<std::array<HashEntry, 8>> hashTable;
    std::array<std::array<unsigned long long, 15>, 15> blackRandomTable;
    std::array<std::array<unsigned long long, 15>, 15> whiteRandomTable;
    unsigned long long mask;
    unsigned long long checkSum;
    int generation;

public:
    TranspositionTable();
    TranspositionTable(const size_t &size);
    void insert(const unsigned long long &hashKey,
                const HashEntry::Type &type,
                const QPoint &move,
                const int &depth,
                const int &score,
                const Stone &stone);
    void aging();
    void transpose(const QPoint &move, const Stone &stone);
    [[nodiscard]] unsigned long long hash() const;
    int probe(const unsigned long long &hashKey,
              const int &alpha,
              const int &beta,
              const int &depth,
              const Stone &stone,
              QPoint &move);
};
} // namespace Search

#endif
