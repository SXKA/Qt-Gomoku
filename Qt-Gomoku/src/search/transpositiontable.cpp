#include "transpositiontable.h"

#include <QtGlobal>

#include <random>

using namespace Search;

TranspositionTable::TranspositionTable()
    : TranspositionTable((1 << 28) / sizeof(std::array<HashEntry, 8>)){};

TranspositionTable::TranspositionTable(const size_t &size)
    : hashTable(QVarLengthArray<std::array<HashEntry, 8>>(size))
    , mask(size - 1)
    , checkSum(0)
    , generation(0)
{
    std::random_device device;
    std::default_random_engine engine(device());
    std::uniform_int_distribution<unsigned long long> distribution;

    for (size_t i = 0; i <= mask; ++i) {
        hashTable[i].fill(HashEntry{0, HashEntry::Exact, {-1, -1}, 0, 0, MISS, Empty});
    }

    for (size_t i = 0; i < 15; ++i) {
        for (size_t j = 0; j < 15; ++j) {
            blackRandomTable[i][j] = distribution(engine);
            whiteRandomTable[i][j] = distribution(engine);
        }
    }
}

void TranspositionTable::insert(const unsigned long long &hashKey,
                                const HashEntry::Type &type,
                                const QPoint &move,
                                const int &depth,
                                const int &score,
                                const Stone &stone)
{
    const auto index = hashKey & mask;
    auto &entries = hashTable[index];
    auto *replacement = &entries.front();

    for (auto &entry : entries) {
        if (entry.lock == hashKey && entry.stone == stone) {
            replacement = &entry;

            break;
        }

        if (entry.depth - (generation - entry.generation)
            < replacement->depth - (generation - replacement->generation)) {
            replacement = &entry;
        }
    }

    if (type != HashEntry::Exact && depth + 2 < replacement->depth) {
        return;
    }

    replacement->lock = hashKey;
    replacement->type = type;
    replacement->move = move == QPoint{-1, -1} ? replacement->move : move;
    replacement->generation = generation;
    replacement->depth = depth;
    replacement->score = score;
    replacement->stone = stone;
}

void TranspositionTable::aging()
{
    ++generation;
}

void TranspositionTable::transpose(const QPoint &move, const Stone &stone)
{
    const auto &[x, y] = move;
    const auto &randomTable = stone == Black ? blackRandomTable : whiteRandomTable;

    checkSum ^= randomTable[x][y];
}

unsigned long long TranspositionTable::hash() const
{
    return checkSum;
}

int TranspositionTable::probe(const unsigned long long &hashKey,
                              const int &alpha,
                              const int &beta,
                              const int &depth,
                              const Stone &stone,
                              QPoint &move)
{
    const auto index = hashKey & mask;
    auto &entries = hashTable[index];

    for (auto &[entryLock, entryType, entryMove, entryAge, entryDepth, entryScore, entryStone] :
         entries) {
        if (entryLock == hashKey && entryStone == stone) {
            move = entryMove;

            entryAge = generation;

            bool mate = false;
            int compensation = 0;

            if (entryScore >= Max - 225) {
                mate = true;
                compensation = 1;
            } else if (entryScore <= Min + 225) {
                mate = true;
                compensation = -1;
            }

            if (entryDepth >= depth || mate) {
                switch (entryType) {
                case HashEntry::Exact:
                    return entryScore + compensation;
                case HashEntry::LowerBound:
                    if (entryScore >= beta) {
                        return entryScore + compensation;
                    }

                    break;
                case HashEntry::UpperBound:
                    if (entryScore <= alpha) {
                        return entryScore + compensation;
                    }

                    break;
                }
            }
        }
    }

    return MISS;
}
