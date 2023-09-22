#ifndef GOBANG_H
#define GOBANG_H

#include "../zobrist/translationtable.h"
#include "movesgenerator.h"
#include <QCache>
#include <QList>
#include <QMessageBox>
#include <QPair>
#include <QPoint>
#include <QSet>
#include <QStack>
#include <QString>
#include <array>
#include <algorithm>
#include <functional>
#include <string>

#ifdef emit
#undef emit
#include "../algorithm/aho_corasick.hpp"
#endif

namespace Gomoku {
constexpr auto R = 3;
constexpr auto LimitDepth = 12;

enum NodeType {
    AllNode = -1, PVNode, CutNode
};

enum Score {
    One = 20,
    Two = 60,
    Three = 720,
    Four = 720,
    OpenFours = 4320,
    Five = 50000,
    Max = 10000000,
    Min = -Max
};

enum State { Draw = -1, Undecided, Win };

class Engine
{
private:
    static QCache<std::string, int> largeCache;
    static QCache<std::string, int> smallCache;
    static const QHash<std::string, Score> shapeScoreHash;
    static aho_corasick::trie trie;
    MovesGenerator movesGenerator;
    Zobrist::TranslationTable translationTable;
    QStack<QPoint> movesHistory;
    QStack<std::array<int, 72>> blackScoresHistory;
    QStack<std::array<int, 72>> whiteScoresHistory;
    QStack<int> blackTotalScoreHistory;
    QStack<int> whiteTotalScoreHistory;
    QPoint bestPoint;
    std::array<std::array<Stone, 15>, 15> board;
    std::array<int, 72> blackScores;
    std::array<int, 72> whiteScores;
    int blackTotalScore;
    int whiteTotalScore;
public:
    Engine();
    [[nodiscard]] static bool isLegal(const QPoint &point);
    void move(const QPoint &point, const Stone &stone);
    void undo(const int &step);
    [[nodiscard]] Stone checkStone(const QPoint &point) const;
    [[nodiscard]] State gameState(const QPoint &point, const Stone &stone) const;
    QPoint bestMove(const Stone &stone);
    [[nodiscard]] QPoint lastStone() const;

private:
    void restoreScore();
    void updateScore(const QPoint &point);
    int evaluatePoint(const QPoint &point) const;
    int lineScore(const QPoint &point, const int &dx, const int &dy) const;
    int evaluate(const Stone &stone) const;
    int pvs(const Stone &stone, int alpha, const int &beta, const int &depth, const NodeType &nodeType);
};
}
#endif
