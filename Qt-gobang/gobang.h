#pragma once

#include "zobrist.h"
#include <array>
#include <algorithm>
#include <functional>
#include <string>
#include <QCache>
#include <QMessageBox>
#include <QPair>
#include <QPoint>
#include <QSet>
#include <QStack>
#include <QString>
#include <QVector>

#ifdef emit
#undef emit
#include "aho_corasick.hpp"
#endif

namespace gobang {
constexpr auto maxDepth = 10;

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
    static QCache<std::string, int> largeCache;
    static QCache<std::string, int> smallCache;
    static const QHash<std::string, score> shapeScoreHash;
    aho_corasick::trie trie;
	zobrist::Zobrist zobrist;
    QSet<QPoint> vacancies;
    QStack<QPoint> record;
    QPoint bestPoint;
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