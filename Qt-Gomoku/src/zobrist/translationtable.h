#ifndef TRANSLATIONTABLE_H
#define TRANSLATIONTABLE_H

#include <array>
#include <random>
#include <QPoint>
#include <QVarLengthArray>

namespace zobrist {
struct hashEntry {
    unsigned long long checkSum;
    enum Type { empty, exact, alpha, beta } type;
    int score;
    int depth;
};

class TranslationTable
{
public:
    TranslationTable();
    TranslationTable(const int &size);
    void insert(const unsigned long long &hashKey, const hashEntry::Type &type, const int &depth,
                const int &score);
    bool contains(const unsigned long long &hashKey, const int &depth) const;
    unsigned long long hash() const;
    unsigned long long translate(const QPoint &point, const bool &color);
    hashEntry at(const unsigned long long &hashKey) const;

private:
    std::array<std::array<unsigned long long, 15>, 15> blackRandomTable;
    std::array<std::array<unsigned long long, 15>, 15> whiteRandomTable;
    QVarLengthArray<hashEntry> hashTable;
    unsigned long long boardHash;
};
}

#endif