#ifndef GOBANG_H
#define GOBANG_H

#include "../zobrist/translationtable.h"
#include "movesgenerator.h"
#include <array>
#include <algorithm>
#include <functional>
#include <string>
#include <QCache>
#include <QList>
#include <QMessageBox>
#include <QPair>
#include <QPoint>
#include <QSet>
#include <QStack>
#include <QString>

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
    OpenThrees = 720,
    Four = 720,
    OpenFours = 4320,
    Five = 50000,
    Max = 10000000,
    Min = -Max
};

enum State { Draw = -1, Undecided, Win };

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
    Zobrist::TranslationTable translationTable;
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
    int pvs(const Stone &stone, int alpha, const int &beta, const int &depth, const NodeType &nodeType);
};
}
#endif
