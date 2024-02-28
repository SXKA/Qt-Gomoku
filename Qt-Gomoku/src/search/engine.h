#ifndef ENGINE_H
#define ENGINE_H

#include "../core/types.h"
#include "../evaluation/evaluator.h"
#include "../game/movesgenerator.h"
#include "transpositiontable.h"

#include <QPair>
#include <QPoint>
#include <QStack>

#include <array>
#include <string>

namespace Search {
inline int IID_DEPTH = 10;
inline int IID_R = 2;
inline int LIMIT_DEPTH = 12;
inline int MCA_C = 2;
inline int MCA_M = 10;
inline int MCA_R = 2;
inline int R = 3;

enum NodeType {
    AllNode = -1, PVNode, CutNode
};

class Engine
{
private:
    Evaluation::Evaluator evaluator;
    Game::MovesGenerator generator;
    TranspositionTable transpositionTable;
    QHash<QPoint, QPair<int, int>> escapes;
    QStack<QPoint> moveHistory;
    QPoint bestPoint;
    std::array<std::array<Stone, 15>, 15> board;
    std::array<std::string, 72> blackShapes;
    std::array<std::string, 72> whiteShapes;
    unsigned long long checkSum;
    unsigned long long cutNodeCount;
    unsigned long long hitNodeCount;
    unsigned long long nodeCount;
public:
    Engine();
    [[nodiscard]] static bool isLegal(const QPoint &point);
    void move(const QPoint &point, const Stone &stone);
    void undo(const int &step);
    [[nodiscard]] Stone checkStone(const QPoint &point) const;
    [[nodiscard]] State gameState(const QPoint &point, const Stone &stone) const;
    [[nodiscard]] QPoint bestMove(const Stone &stone);
    [[nodiscard]] QPoint lastMove() const;

private:
    bool inThreat(const Stone &stone);
    int pvs(const Stone &stone, int alpha, const int &beta, const int &depth, const NodeType &nodeType,
            const bool &nullOk = true);
};
}
#endif
