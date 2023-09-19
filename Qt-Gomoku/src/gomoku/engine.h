#ifndef GOBANG_H
#define GOBANG_H

#include "../zobrist/translationtable.h"
#include "movesgenerator.h"
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
#include "../algorithm/aho_corasick.hpp"
#endif

namespace Gobang {
constexpr auto r = 2;
constexpr auto limitDepth = 10;

enum Score {
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

enum State { draw = -1, undecided, win };

class Engine
{
public:
    Engine();
    [[nodiscard]] static bool isLegal(const QPoint &point);
    void move(const QPoint &point, const Stone &stone);
    void undo(const int &step);
    [[nodiscard]] bool gameOver(const QPoint &point, const Stone &stone) const;
    [[nodiscard]] bool isInitial(const bool &type, const Stone &stone) const;
    [[nodiscard]] Stone checkStone(const QPoint &point) const;
    [[nodiscard]] State gameState(const QPoint &point, const Stone &stone) const;
    QPoint bestMove(const Stone &stone);
    [[nodiscard]] QPoint lastStone() const;

private:
    static QCache<std::string, int> largeCache;
    static QCache<std::string, int> smallCache;
    static const QHash<std::string, Score> shapeScoreHash;
    MovesGenerator movesGenerator;
    aho_corasick::trie trie;
    zobrist::TranslationTable translationTable;
    QStack<QPoint> record;
    QPoint bestPoint;
    std::array<std::array<Stone, 15>, 15> board;
    std::array<int, 72> blackScores;
    std::array<int, 72> whiteScores;
    int blackTotalScore;
    int whiteTotalScore;
    void updateScore(const QPoint &point);
    int calculateScore(const QPoint &point);
    int dScore(const QPoint &point, const int &dx, const int &dy);
    int evaluate(const Stone &stone) const;
    int negamax(const Stone &stone, const int &depth, int alpha = minScore,
                const int &beta = maxScore - 1);
};
}
#endif