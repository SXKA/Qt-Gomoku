#pragma once

#include "zobrist.h"
#include <array>
#include <set>
#include <unordered_map>
#include <QMessageBox>
#include <QPoint>
#include <QSet>
#include <QStack>

#ifdef emit
#undef emit
#include "aho_corasick.hpp"
#endif

namespace gobang {
constexpr auto maxDepth = 8;

enum score {
    one = 20,
    two = 60,
    three = 720,
    livingThree = 720,
    four = 720,
    livingFour = 4320,
    five = 50000,
    maxScore = 10000000,
    minScore = -maxScore
};

enum stone { empty = -1, black, white };

enum state { draw = -1, undecided, win };

class Gobang
{
public:
    Gobang();
    [[nodiscard]] static bool isLegal(const QPoint &point);
    void back(const int &step);
    void play(const QPoint &point, const stone &stone);
    [[nodiscard]] bool gameOver(const QPoint &point, const stone &stone) const;
    [[nodiscard]] bool isInitial(const bool &type, const stone &stone) const;
    [[nodiscard]] stone checkStone(const QPoint &point) const;
    [[nodiscard]] state gameState(const QPoint &point, const stone &stone) const;
    QPoint ai(const stone &stone);
    [[nodiscard]] QPoint lastStone() const;

private:
    static const std::unordered_map<std::string, score> shapeScoreMap;
    QSet<QPoint> vacancies;
    QStack<QPoint> record;
    QPoint bestPoint;
    zobrist::Zobrist zobrist;
    aho_corasick::trie trie;
    std::array<std::array<stone, 15>, 15> board;
    std::array<int, 72> blackScores;
    std::array<int, 72> whiteScores;
    int blackTotalScore;
    int whiteTotalScore;
    void updateScore(const QPoint &point);
    [[nodiscard]] bool isIsolated(const QPoint &point) const;
    int alphaBetaPrune(const stone &stone, const int &depth, int alpha = minScore,
                       const int &beta = maxScore - 1);
    int calculateScore(const QPoint &point);
    int dScore(const QPoint &point, const int &dx, const int &dy);
    int evaluate(const stone &stone) const;
};
}