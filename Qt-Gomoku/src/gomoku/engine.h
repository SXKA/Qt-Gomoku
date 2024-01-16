#ifndef ENGINE_H
#define ENGINE_H

#include "../zobrist/transpositiontable.h"
#include "movesgenerator.h"
#include "stone.h"
#include <QtGlobal>
#include <QCache>
#include <QList>
#include <QMessageBox>
#include <QPair>
#include <QPoint>
#include <QSet>
#include <QStack>
#include <QString>
#include <QTime>
#include <array>
#include <algorithm>
#include <functional>
#include <string>

#ifdef emit
#undef emit
#include "../algorithm/aho_corasick.hpp"
#endif

namespace Gomoku {
inline auto R = 3;
constexpr auto LIMIT_DEPTH = 10;
constexpr auto LIMIT_WIDTH = 10;

enum NodeType {
    AllNode = -1, PVNode, CutNode
};

enum Score {
    One = 20,
    Two = 120,
    Three = 720,
    Four = 720,
    OpenFours = 4320,
    Five = 50000,
    Max = 10000000,
    Min = -Max
};

enum State { Draw = -1, Undecided, Win };

struct History {
    QStack<QPoint> moves;
    QStack<std::array<int, 72>> blackScores;
    QStack<std::array<int, 72>> whiteScores;
    QStack<int> blackTotalScore;
    QStack<int> whiteTotalScore;
};

class Engine
{
private:
    static aho_corasick::trie trie;
    static aho_corasick::trie checkTrie;
    static QCache<std::string, int> largeCache;
    static QCache<std::string, int> smallCache;
    static const QHash<std::string, Score> shapeScoreTable;
    History history;
    MovesGenerator generator;
    Zobrist::TranspositionTable transpositionTable;
    QSet<QPoint> escapes;
    QPoint bestPoint;
    std::array<std::array<Stone, 15>, 15> board;
    std::array<std::string, 72> blackShapes;
    std::array<std::string, 72> whiteShapes;
    std::array<int, 72> blackScores;
    std::array<int, 72> whiteScores;
    unsigned long long checkSum;
    unsigned long long cutNodeCount;
    unsigned long long hitNodeCount;
    unsigned long long nodeCount;
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
    [[nodiscard]] QPoint lastMove() const;

private:
    void restoreScore();
    void updateScore(const QPoint &point);
    bool inCheck(const Stone &stone);
    int evaluatePoint(const QPoint &point) const;
    int lineScore(const QPoint &point, const int &direction) const;
    int evaluate(const Stone &stone) const;
    int pvs(const Stone &stone, int alpha, const int &beta, const int &depth, const NodeType &nodeType,
            const bool &nullOk = true);
};
}
#endif
