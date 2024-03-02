#include "engine.h"

#include <QtGlobal>
#include <QMessageBox>
#include <QTime>

#include <algorithm>
#include <cmath>

using namespace Search;

namespace {
std::array<int, 226> moveCounts;
}

inline bool operator< (const QPoint &lhs, const QPoint &rhs)
{
    return lhs.y() == rhs.y() ? lhs.x() < rhs.x() : lhs.y() < rhs.y();
}

Engine::Engine()
    : evaluator(&blackShapes, &whiteShapes)
    , generator(&evaluator, &board)
    , board({})
, blackShapes({})
, whiteShapes({})
, checkSum(transpositionTable.hash())
, cutNodeCount(0)
, hitNodeCount(0)
, nodeCount(0)
{
    for (size_t i = 0; i < moveCounts.size(); ++i) {
        moveCounts[i] = (3 + static_cast<int>(std::pow(i, 1.33))) >> 1;
    }

    std::fill_n(blackShapes.begin(), 30, std::string(15, '0'));
    std::fill_n(whiteShapes.begin(), 30, std::string(15, '0'));

    for (int i = 5; i <= 15; ++i) {
        blackShapes[25 + i] = std::string(i, '0');
        whiteShapes[25 + i] = std::string(i, '0');
        blackShapes[35 + i] = std::string(20 - i, '0');
        whiteShapes[35 + i] = std::string(20 - i, '0');
        blackShapes[46 + i] = std::string(i, '0');
        whiteShapes[46 + i] = std::string(i, '0');
        blackShapes[56 + i] = std::string(20 - i, '0');
        whiteShapes[56 + i] = std::string(20 - i, '0');
    }
}

bool Engine::isLegal(const QPoint &point)
{
    return point.x() >= 0 && point.x() < 15 && point.y() >= 0 && point.y() < 15;
}

void Engine::move(const QPoint &point, const Stone &stone)
{
    const auto x = point.x();
    const auto y = point.y();
    auto &firstShapes = stone == Black ? blackShapes : whiteShapes;
    auto &secondShapes = stone == Black ? whiteShapes : blackShapes;

    firstShapes[y][x] = '1';
    secondShapes[y][x] = '2';
    firstShapes[x + 15][y] = '1';
    secondShapes[x + 15][y] = '2';

    if (qAbs(y - x) <= 10) {
        firstShapes[y - x + 40][qMin(x, y)] = '1';
        secondShapes[y - x + 40][qMin(x, y)] = '2';
    }

    if (x + y >= 4 && x + y <= 24) {
        firstShapes[x + y + 47][qMin(y, 14 - x)] = '1';
        secondShapes[x + y + 47][qMin(y, 14 - x)] = '2';
    }

    evaluator.update(point);
    generator.move(point);
    transpositionTable.transpose(point, stone);
    moveHistory.push(point);
    board[x][y] = stone;
}

void Engine::undo(const int &step)
{
    for (int i = 0; i < step; ++i) {
        const auto point = moveHistory.top();
        const auto x = point.x();
        const auto y = point.y();

        generator.undo(point);
        transpositionTable.transpose(point, checkStone(point));
        moveHistory.pop();
        board[x][y] = Empty;
        blackShapes[y][x] = '0';
        whiteShapes[y][x] = '0';
        blackShapes[x + 15][y] = '0';
        whiteShapes[x + 15][y] = '0';

        if (qAbs(y - x) <= 10) {
            blackShapes[y - x + 40][qMin(x, y)] = '0';
            whiteShapes[y - x + 40][qMin(x, y)] = '0';
        }

        if (x + y >= 4 && x + y <= 24) {
            blackShapes[x + y + 47][qMin(y, 14 - x)] = '0';
            whiteShapes[x + y + 47][qMin(y, 14 - x)] = '0';
        }

        evaluator.restore();
    }
}

Stone Engine::checkStone(const QPoint &point) const
{
    return board[point.x()][point.y()];
}

State Engine::gameState(const QPoint &point, const Stone &stone) const
{
    constexpr std::array<int, 2> d = {-1, 1};
    constexpr std::array<int, 4> dx = {1, 0, 1, 1};
    constexpr std::array<int, 4> dy = {0, 1, 1, -1};

    for (size_t i = 0; i < 4; ++i) {
        int count = 1;

        for (size_t j = 0; j < 2; ++j) {
            auto neighborhood = point + QPoint(d[j] * dx[i], d[j] * dy[i]);

            while (isLegal(neighborhood) && checkStone(neighborhood) == stone) {
                ++count;

                neighborhood += QPoint(d[j] * dx[i], d[j] * dy[i]);
            }
        }

        if (count >= 5) {
            return Win;
        }
    }

    for (const auto &row : board) {
        if (std::any_of(row.cbegin(), row.cend(), [](const auto & x) {
        return x == Empty;
    })) {
            return Undecided;
        }
    }

    return Draw;
}

QPoint Engine::bestMove(const Stone &stone)
{
    if (const auto last = lastMove(); moveHistory.empty() || (moveHistory.size() == 1
                                                              && last != QPoint(7, 7)
                                                              && checkStone(last) != stone)) {
        return {7, 7};
    }

    transpositionTable.aging();
    checkSum = transpositionTable.hash();

    const QTime time = QTime::currentTime();
    const auto score = pvs(stone, Min, Max, LIMIT_DEPTH, PVNode);
    const auto elapsedTime = time.msecsTo(QTime::currentTime());

    qInfo() << bestPoint;
    qInfo() << "Score: " << score;
    qInfo() << "Node numbers: " << nodeCount;
    qInfo() << "Cut node numbers: " << cutNodeCount << " (" << 100 * cutNodeCount / nodeCount << "%)";
    qInfo() << "Hit node numbers: " << hitNodeCount << " (" << 100 * hitNodeCount / nodeCount << "%)";
    qInfo() << "Elapsed time: " << 0.001 * elapsedTime << 's';
    qInfo() << "Node per second: " << nodeCount / (0.001 * elapsedTime);
    qInfo() << "Time per node: " << 1000.0 * elapsedTime / nodeCount << "us";

    cutNodeCount = 0;
    hitNodeCount = 0;
    nodeCount = 0;

    return bestPoint;
}

QPoint Engine::lastMove() const
{
    return moveHistory.empty() ? QPoint(-1, -1) : moveHistory.top();
}

bool Engine::inThreat(const Stone &stone, QHash<QPoint, QPair<int, int>> &moves)
{
    auto blackMaxMove = moves.cbegin();
    auto whiteMaxMove = moves.cbegin();

    for (auto it = moves.cbegin(); it != moves.cend(); ++it) {
        if (it.value().first > blackMaxMove.value().first) {
            blackMaxMove = it;
        }

        if (it.value().second > whiteMaxMove.value().second) {
            whiteMaxMove = it;
        }
    }

    const auto &secondMaxMove = stone == Black ? whiteMaxMove : blackMaxMove;
    const auto &firstMaxScore = stone == Black ? blackMaxMove.value().first :
                                whiteMaxMove.value().second;
    const auto &secondMaxScore = stone == Black ? whiteMaxMove.value().second :
                                 blackMaxMove.value().first;

    if (firstMaxScore >= Five) {
        return false;
    }

    if (secondMaxScore >= Five) {
        const auto point = secondMaxMove.key();
        const auto scores = secondMaxMove.value();

        moves.clear();
        moves.insert(point, scores);

        return true;
    }

    return false;
}

int Engine::pvs(const Stone &stone, int alpha, const int &beta, const int &depth,
                const NodeType &nodeType, const bool &nullOk)
{
    ++nodeCount;

    QPoint goodMove{-1, -1};

    if (nodeType != PVNode) {
        if (const auto probeScore = transpositionTable.probe(transpositionTable.hash(), alpha, beta, depth,
                                                             goodMove); probeScore != MISS) {
            ++hitNodeCount;

            return probeScore >= beta ? beta : probeScore;
        }
    }

    const auto firstScore = evaluator.evaluate(stone);
    const auto secondScore = evaluator.evaluate(static_cast<const Stone>(-stone));

    if (firstScore >= Five) {
        return Max - (LIMIT_DEPTH - depth) - 1;
    }

    if (secondScore >= Five) {
        return Min + (LIMIT_DEPTH - depth) + 1;
    }

    if (generator.empty()) {
        return firstScore - secondScore;
    }

    auto moves = generator.generate();
    const bool extend = inThreat(stone, moves);

    if (depth <= 0 && !extend) {
        return firstScore - secondScore;
    }

    if (nodeType != PVNode && !extend && nullOk) {
        R = depth >= 6 ? 3 : 2;

        if (const auto &score = -pvs(static_cast<const Stone>(-stone), -beta, -beta + 1, depth - R - 1,
                                     nodeType, false); score >= beta) {
            ++cutNodeCount;

            return beta;
        }
    }

    bool heuristic = false;
    QList<QPair<int, QPoint>> candidates;

    if (!extend) {
        heuristic = moves.remove(goodMove);
    }

    auto blackMaxMove = moves.cbegin();
    auto whiteMaxMove = moves.cbegin();

    for (auto it = moves.cbegin(); it != moves.cend(); ++it) {
        const auto &[blackScore, whiteScore] = it.value();

        if (blackScore > blackMaxMove.value().first) {
            blackMaxMove = it;
        }

        if (whiteScore > whiteMaxMove.value().second) {
            whiteMaxMove = it;
        }

        candidates.emplace_back(blackScore + whiteScore, it.key());
    }

    if (!extend && !moves.empty()) {
        const auto &firstMaxMove = stone == Black ? blackMaxMove : whiteMaxMove;
        const auto &firstMaxScore = stone == Black ? blackMaxMove.value().first :
                                    whiteMaxMove.value().second;
        const auto &secondMaxScore = stone == Black ? whiteMaxMove.value().second :
                                     blackMaxMove.value().first;

        if (firstMaxScore >= OpenFours) {
            candidates.clear();
            candidates.emplace_back(firstMaxMove.value().first + firstMaxMove.value().second,
                                    firstMaxMove.key());
        } else if (secondMaxScore >= OpenFours) {
            auto it = candidates.cbegin();

            while (it != candidates.cend()) {
                const auto &firstMoveScore = stone == Black ? moves[it->second].first : moves[it->second].second;
                const auto &secondMoveScore = stone == Black ? moves[it->second].second : moves[it->second].first;

                if (firstMoveScore < Four && secondMoveScore < OpenFours) {
                    it = candidates.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(), std::greater());

    if (heuristic) {
        candidates.emplaceFront(INT_MAX, goodMove);
    }

    if (nodeType == CutNode && depth > MC_R && candidates.size() >= MC_M) {
        int c = 0;

        for (int m = 0; m < MC_M; ++m) {
            move(candidates[m].second, stone);

            const auto score = -pvs(static_cast<const Stone>(-stone), -beta, -alpha, depth - MC_R - 1,
                                    static_cast<const NodeType>(-nodeType));

            undo(1);

            if (score >= beta) {
                if (score >= Five) {
                    ++cutNodeCount;

                    return score;
                }

                if (++c >= MC_C) {
                    ++cutNodeCount;

                    return beta;
                }
            }
        }
    }

    if (!extend && candidates.size() > moveCounts[depth]) {
        candidates.resize(moveCounts[depth]);
    }

    move(candidates.front().second, stone);

    auto bestScore = -pvs(static_cast<const Stone>(-stone), -beta, -alpha, depth + extend - 1,
                          static_cast<const NodeType>(-nodeType));

    undo(1);

    if (bestScore >= beta) {
        if (transpositionTable.hash() != checkSum) {
            transpositionTable.insert(transpositionTable.hash(), HashEntry::LowerBound,
                                      candidates.front().second, depth, bestScore);
        }

        ++cutNodeCount;

        return bestScore;
    }

    QPoint pvNode{-1, -1};
    auto valueType = HashEntry::UpperBound;

    if (bestScore > alpha) {
        alpha = bestScore;
        pvNode = candidates.front().second;
        valueType = HashEntry::Exact;

        if (transpositionTable.hash() == checkSum) {
            bestPoint = candidates.front().second;
        }
    }

    candidates.pop_front();

    for (const auto&[_, candidate] : candidates) {
        move(candidate, stone);

        auto candidateScore = -pvs(static_cast<const Stone>(-stone), -alpha - 1, -alpha, depth + extend - 1,
                                   nodeType == CutNode ? AllNode : CutNode);

        undo(1);

        if (candidateScore > alpha && candidateScore < beta || candidateScore == beta && beta == alpha + 1
                && nodeType == PVNode) {
            if (candidateScore == alpha + 1) {
                candidateScore = alpha;
            }

            move(candidate, stone);

            candidateScore = -pvs(static_cast<const Stone>(-stone), -beta, -candidateScore, depth + extend - 1,
                                  nodeType);

            undo(1);
        }

        if (candidateScore > bestScore) {
            bestScore = candidateScore;

            if (bestScore >= beta) {
                if (transpositionTable.hash() != checkSum) {
                    transpositionTable.insert(transpositionTable.hash(), HashEntry::LowerBound, candidate, depth,
                                              bestScore);
                }

                ++cutNodeCount;

                return bestScore;
            }

            if (bestScore > alpha) {
                alpha = bestScore;
                pvNode = candidate;
                valueType = HashEntry::Exact;

                if (transpositionTable.hash() == checkSum) {
                    bestPoint = candidate;
                }
            }
        }
    }

    if (nodeType == CutNode && bestScore == alpha) {
        return bestScore;
    }

    if (transpositionTable.hash() != checkSum) {
        transpositionTable.insert(transpositionTable.hash(), valueType, pvNode, depth, bestScore);
    }

    return bestScore;
}
