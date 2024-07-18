#include "engine.h"

#include <QTime>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

using namespace Search;

namespace {
std::array<int, 226> moveCounts;
}

inline bool operator<(const QPoint &lhs, const QPoint &rhs)
{
    const auto &[lhsX, lhsY] = lhs;
    const auto &[rhsX, rhsY] = rhs;
    const auto lhsD = qAbs(lhsX - 7) + qAbs(lhsY - 7);
    const auto rhsD = qAbs(rhsX - 7) + qAbs(rhsY - 7);

    if (lhsD != rhsD) {
        return lhsD < rhsD;
    }

    return lhsY == rhsY ? lhsX < rhsX : lhsY < rhsY;
}

Engine::Engine()
    : evaluator(&blackShapes, &whiteShapes)
    , generator(&evaluator, &board)
    , board({})
    , blackShapes({})
    , whiteShapes({})
    , cutNodeCount(0)
    , hitNodeCount(0)
    , nodeCount(0)
    , ply(0)
{
    for (size_t i = 0; i < moveCounts.size(); ++i) {
        moveCounts[i] = static_cast<int>(std::pow(i, 1.33) + 3) / 2;
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

bool Engine::isLegal(const QPoint &move)
{
    return move.x() >= 0 && move.x() < 15 && move.y() >= 0 && move.y() < 15;
}

void Engine::move(const QPoint &point, const Stone &stone)
{
    const auto &[x, y] = point;
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
    pvsTT.transpose(point, stone);
    vcfTT.transpose(point, stone);
    moveHistory.push(point);
    board[x][y] = stone;
}

void Engine::undo(const int &step)
{
    for (int i = 0; i < step; ++i) {
        const auto move = moveHistory.top();
        const auto &[x, y] = move;

        generator.undo(move);
        pvsTT.transpose(move, checkStone(move));
        vcfTT.transpose(move, checkStone(move));
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
    const auto &[x, y] = point;

    return board[x][y];
}

Status Engine::gameStatus(const QPoint &move, const Stone &stone) const
{
    constexpr std::array<int, 2> d = {-1, 1};
    constexpr std::array<int, 4> dx = {1, 0, 1, 1};
    constexpr std::array<int, 4> dy = {0, 1, 1, -1};

    for (size_t i = 0; i < 4; ++i) {
        int count = 1;

        for (size_t j = 0; j < 2; ++j) {
            auto neighborhood = move + d[j] * QPoint{dx[i], dy[i]};

            while (isLegal(neighborhood) && checkStone(neighborhood) == stone) {
                ++count;

                neighborhood += d[j] * QPoint{dx[i], dy[i]};
            }
        }

        if (count >= 5) {
            return Win;
        }
    }

    for (const auto &row : board) {
        if (std::any_of(row.cbegin(), row.cend(), [](const auto &x) { return x == Empty; })) {
            return Undecided;
        }
    }

    return Draw;
}

QPoint Engine::bestMove(const Stone &stone)
{
    if (const auto last = lastMove();
        moveHistory.empty()
        || (moveHistory.size() == 1 && last != QPoint(7, 7) && checkStone(last) != stone)) {
        return {7, 7};
    }

    pvsTT.aging();
    vcfTT.aging();
    ply = static_cast<const int>(moveHistory.size());

    const QTime time = QTime::currentTime();
    const auto score = pvs<PVNode>(stone, Min, Max, LIMIT_DEPTH);
    const auto elapsedTime = time.msecsTo(QTime::currentTime());

    qInfo() << "Best move: " << bestPoint;
    qInfo() << "Score: " << score;
    qInfo() << "Node numbers: " << nodeCount;
    qInfo() << "Cut node numbers: " << cutNodeCount << " (" << 100 * cutNodeCount / nodeCount
            << "%)";
    qInfo() << "Hit node numbers: " << hitNodeCount << " (" << 100 * hitNodeCount / nodeCount
            << "%)";
    qInfo() << "Elapsed time: " << 0.001 * elapsedTime << 's';
    qInfo() << "Node per second: " << static_cast<const double>(nodeCount) / (0.001 * elapsedTime);
    qInfo() << "Time per node: " << 1000.0 * elapsedTime / static_cast<const double>(nodeCount)
            << "us";

    cutNodeCount = 0;
    hitNodeCount = 0;
    nodeCount = 0;

    return bestPoint;
}

QPoint Engine::lastMove() const
{
    return moveHistory.empty() ? QPoint(-1, -1) : moveHistory.top();
}

bool Engine::inMated(const Stone &stone, QHash<QPoint, QPair<int, int>> &moves)
{
    auto blackMaxMove = moves.cbegin();
    auto whiteMaxMove = moves.cbegin();

    for (auto it = moves.cbegin(); it != moves.cend(); ++it) {
        const auto [blackScore, whiteScore] = it.value();
        const auto &blackMaxScore = blackMaxMove.value().first;
        const auto &whiteMaxScore = whiteMaxMove.value().second;

        if (blackScore > blackMaxScore) {
            blackMaxMove = it;
        }

        if (whiteScore > whiteMaxScore) {
            whiteMaxMove = it;
        }
    }

    const auto &blackMaxScore = blackMaxMove.value().first;
    const auto &whiteMaxScore = whiteMaxMove.value().second;
    const auto &firstMaxScore = stone == Black ? blackMaxScore : whiteMaxScore;

    if (firstMaxScore >= Five) {
        return false;
    }

    const auto &secondMaxMove = stone == Black ? whiteMaxMove : blackMaxMove;
    const auto &secondMaxScore = stone == Black ? whiteMaxScore : blackMaxScore;

    if (secondMaxScore >= Five) {
        const auto move = secondMaxMove.key();
        const auto scores = secondMaxMove.value();

        moves.clear();
        moves.insert(move, scores);

        return true;
    }

    return false;
}

template<NodeType NT>
int Engine::pvs(const Stone &stone, int alpha, const int &beta, const int &depth, const bool &nullOk)
{
    ++nodeCount;

    const int distance = static_cast<const int>(moveHistory.size()) - ply;
    const auto firstScore = evaluator.evaluate(stone);
    const auto secondScore = evaluator.evaluate(static_cast<const Stone>(-stone));

    if (firstScore >= Five) {
        return Max;
    }

    if (secondScore >= Five) {
        return Min;
    }

    if (generator.empty()) {
        return 0;
    }

    if (depth <= 0) {
        return vcfSearch<NT>(stone, alpha, beta, VCF_DEPTH);
    }

    QPoint heuristicMove{-1, -1};
    auto moves = generator.generate();
    const auto extension = inMated(stone, moves);
    auto probeScore = pvsTT.probe(pvsTT.hash(), alpha, beta, depth, stone, heuristicMove);

    if (!distance && moves.size() == 1) {
        bestPoint = moves.cbegin().key();

        return vcfSearch<PVNode>(stone, alpha, beta, VCF_DEPTH);
    }

    if (NT != PVNode) {
        if (probeScore != MISS) {
            ++hitNodeCount;

            return probeScore;
        }

        const auto eval = firstScore - secondScore;

        if (depth < 3 && eval + Two * depth < alpha) {
            return vcfSearch<NT>(stone, alpha, alpha + 1, VCF_DEPTH);
        }

        if (eval - Two * depth >= beta) {
            ++cutNodeCount;

            return beta;
        }

        if (!extension && nullOk) {
            R = depth >= 6 ? 3 : 2;

            auto score = -pvs<NT>(static_cast<const Stone>(-stone),
                                  -beta,
                                  -beta + 1,
                                  depth - R - 1,
                                  false);

            if (score >= Max - 225) {
                --score;
            } else if (score <= Min + 225) {
                ++score;
            }

            if (score >= beta) {
                ++cutNodeCount;

                return beta;
            }
        }
    }

    QList<QPair<int, QPoint>> candidates;
    auto blackMaxMove = moves.cbegin();
    auto whiteMaxMove = moves.cbegin();

    candidates.reserve(moves.size());

    for (auto it = moves.cbegin(); it != moves.cend(); ++it) {
        const auto [blackScore, whiteScore] = it.value();
        const auto &blackMaxScore = blackMaxMove.value().first;
        const auto &whiteMaxScore = whiteMaxMove.value().second;

        if (blackScore > blackMaxScore) {
            blackMaxMove = it;
        }

        if (whiteScore > whiteMaxScore) {
            whiteMaxMove = it;
        }

        candidates.emplace_back(blackScore + whiteScore, it.key());
    }

    bool mated = false;

    if (!extension) {
        const auto &firstMaxMove = stone == Black ? blackMaxMove : whiteMaxMove;
        const auto &secondMaxMove = stone == Black ? whiteMaxMove : blackMaxMove;
        const auto &blackMaxScore = blackMaxMove.value().first;
        const auto &whiteMaxScore = whiteMaxMove.value().second;
        const auto &firstMaxScore = stone == Black ? blackMaxScore : whiteMaxScore;
        const auto &secondMaxScore = stone == Black ? whiteMaxScore : blackMaxScore;

        if (firstMaxScore >= OpenFour) {
            candidates.clear();
            candidates.emplace_back(firstMaxMove.value().first + firstMaxMove.value().second,
                                    firstMaxMove.key());
        } else if (secondMaxScore >= OpenFour) {
            mated = true;

            int d;

            for (d = 0; d < 4; ++d) {
                const auto [blackScore, whiteScore] = evaluator.evaluateMove(secondMaxMove.key(), d);

                if (const auto &secondMoveScore = stone == Black ? whiteScore : blackScore;
                    secondMoveScore >= OpenFour) {
                    break;
                }
            }

            const auto line = Evaluation::Evaluator::lineOffsetPair(secondMaxMove.key(), d).first;
            auto it = candidates.cbegin();

            while (it != candidates.cend()) {
                const auto &[blackScore, whiteScore] = moves[it->second];
                const auto &firstMoveScore = stone == Black ? blackScore : whiteScore;
                const auto [x, y] = secondMaxMove.key() - it->second;
                const auto offset = qAbs(qMax(x, y));

                if (offset <= 5 && Evaluation::Evaluator::lineOffsetPair(it->second, d).first == line
                    || firstMoveScore >= Four && evaluator.isFourMove(it->second, stone)) {
                    ++it;
                } else {
                    it = candidates.erase(it);
                }
            }
        }

        if (const auto it = std::find_if(candidates.cbegin(),
                                         candidates.cend(),
                                         [heuristicMove](const auto &candidate) {
                                             return candidate.second == heuristicMove;
                                         });
            it != candidates.cend()) {
            candidates.erase(it);
            candidates.emplaceFront(INT_MAX, heuristicMove);
        }
    }

    std::sort(candidates.begin(), candidates.end(), std::greater());

    if (NT == CutNode && depth > MC_R && candidates.size() >= MC_M) {
        int c = 0;
        int m = 0;
        auto it = candidates.cbegin();
        QList<QPair<int, QPoint>> cutoffs;

        while (m < MC_M) {
            move(it->second, stone);

            auto score = -pvs<static_cast<const NodeType>(-NT)>(static_cast<const Stone>(-stone),
                                                                -beta,
                                                                -alpha,
                                                                depth - MC_R - 1);

            undo(1);

            if (score >= Max - 225) {
                --score;
            } else if (score <= Min + 225) {
                ++score;
            }

            if (score >= beta) {
                if (score >= Five) {
                    ++cutNodeCount;

                    return score;
                }

                if (++c >= MC_C) {
                    ++cutNodeCount;

                    return beta;
                }

                cutoffs.push_back(*it);

                it = candidates.erase(it);
            } else {
                ++it;
            }

            ++m;
        }

        if (!cutoffs.empty()) {
            candidates = cutoffs + candidates;
        }
    }

    if (!mated && candidates.size() > moveCounts[depth]) {
        candidates.resize(moveCounts[depth]);
    }

    move(candidates.front().second, stone);

    auto bestScore = -pvs<static_cast<const NodeType>(-NT)>(static_cast<const Stone>(-stone),
                                                            -beta,
                                                            -alpha,
                                                            depth + extension - 1);

    undo(1);

    if (bestScore >= Max - 225) {
        --bestScore;
    } else if (bestScore <= Min + 225) {
        ++bestScore;
    }

    if (bestScore >= beta) {
        pvsTT.insert(pvsTT.hash(),
                     HashEntry::LowerBound,
                     candidates.front().second,
                     depth,
                     bestScore,
                     stone);
        ++cutNodeCount;

        return bestScore;
    }

    QPoint pvNode{-1, -1};
    auto valueType = HashEntry::UpperBound;

    if (bestScore > alpha) {
        alpha = bestScore;
        pvNode = candidates.front().second;
        valueType = HashEntry::Exact;

        if (!distance) {
            bestPoint = candidates.front().second;
        }
    }

    candidates.pop_front();

    for (const auto [_, candidate] : candidates) {
        move(candidate, stone);

        auto candidateScore = -pvs < NT == CutNode ? AllNode
                                                   : CutNode > (static_cast<const Stone>(-stone),
                                                                -alpha - 1,
                                                                -alpha,
                                                                depth + extension - 1);

        undo(1);

        if (candidateScore >= Max - 225) {
            --candidateScore;
        } else if (candidateScore <= Min + 225) {
            ++candidateScore;
        }

        if (candidateScore > alpha && candidateScore < beta
            || (candidateScore == beta && beta == alpha + 1 && NT == PVNode)) {
            if (candidateScore == alpha + 1) {
                candidateScore = alpha;
            }

            move(candidate, stone);

            candidateScore = -pvs<NT>(static_cast<const Stone>(-stone),
                                      -beta,
                                      -candidateScore,
                                      depth + extension - 1);

            undo(1);

            if (candidateScore >= Max - 225) {
                --candidateScore;
            } else if (candidateScore <= Min + 225) {
                ++candidateScore;
            }
        }

        if (candidateScore > bestScore) {
            bestScore = candidateScore;

            if (bestScore >= beta) {
                pvsTT.insert(pvsTT.hash(), HashEntry::LowerBound, candidate, depth, bestScore, stone);
                ++cutNodeCount;

                return bestScore;
            }

            if (bestScore > alpha) {
                alpha = bestScore;
                pvNode = candidate;
                valueType = HashEntry::Exact;

                if (!distance) {
                    bestPoint = candidate;
                }
            }
        }
    }

    if (NT == CutNode && bestScore == alpha) {
        return bestScore;
    }

    pvsTT.insert(pvsTT.hash(), valueType, pvNode, depth, bestScore, stone);

    return bestScore;
}

template<NodeType NT>
int Engine::vcfSearch(const Stone &stone, int alpha, const int &beta, const int &depth)
{
    ++nodeCount;

    const auto firstScore = evaluator.evaluate(stone);
    const auto secondScore = evaluator.evaluate(static_cast<const Stone>(-stone));

    if (firstScore >= Five) {
        return Max;
    }

    if (secondScore >= Five) {
        return Min;
    }

    if (generator.empty()) {
        return 0;
    }

    const auto eval = firstScore - secondScore;

    if (!depth) {
        return eval;
    }

    QPoint heuristicMove{-1, -1};
    const auto probeScore = vcfTT.probe(vcfTT.hash(), alpha, beta, depth, stone, heuristicMove);

    if (NT != PVNode && probeScore != MISS) {
        ++hitNodeCount;

        return probeScore;
    }

    if (eval >= beta) {
        vcfTT.insert(vcfTT.hash(), HashEntry::LowerBound, {-1, -1}, depth, eval, stone);
        ++cutNodeCount;

        return eval;
    }

    auto moves = generator.generate();
    const auto extension = inMated(stone, moves);
    QList<QPair<int, QPoint>> candidates;

    candidates.reserve(moves.size());

    if (extension) {
        const auto it = moves.cbegin();
        const auto [blackScore, whiteScore] = it.value();

        candidates.emplace_back(blackScore + whiteScore, it.key());
    } else {
        bool mate = false;

        for (auto it = moves.cbegin(); it != moves.cend(); ++it) {
            const auto [blackScore, whiteScore] = it.value();
            const auto firstMoveScore = stone == Black ? blackScore : whiteScore;

            if (firstMoveScore >= Five) {
                candidates.clear();
                candidates.emplace_back(blackScore + whiteScore, it.key());

                break;
            }

            if (firstMoveScore >= OpenFour) {
                mate = true;

                candidates.clear();
                candidates.emplace_back(blackScore + whiteScore, it.key());
            } else if (!mate && firstMoveScore >= Four && evaluator.isFourMove(it.key(), stone)) {
                candidates.emplace_back(blackScore + whiteScore, it.key());
            }
        }

        if (const auto it = std::find_if(candidates.cbegin(),
                                         candidates.cend(),
                                         [heuristicMove](const auto &candidate) {
                                             return candidate.second == heuristicMove;
                                         });
            it != candidates.cend()) {
            candidates.erase(it);
            candidates.emplaceFront(INT_MAX, heuristicMove);
        }

        if (candidates.empty()) {
            return eval;
        }
    }

    std::sort(candidates.begin(), candidates.end(), std::greater());

    move(candidates.front().second, stone);

    auto bestScore = -vcfSearch<static_cast<const NodeType>(-NT)>(static_cast<const Stone>(-stone),
                                                                  -beta,
                                                                  -alpha,
                                                                  depth - 1);

    undo(1);

    if (bestScore >= Max - 225) {
        --bestScore;
    } else if (bestScore <= Min + 225) {
        ++bestScore;
    }

    QPoint pvNode{-1, -1};
    auto valueType = HashEntry::UpperBound;

    if (bestScore >= beta) {
        vcfTT.insert(vcfTT.hash(),
                     HashEntry::LowerBound,
                     candidates.front().second,
                     depth,
                     bestScore,
                     stone);
        ++cutNodeCount;

        return bestScore;
    }

    if (bestScore > alpha) {
        alpha = bestScore;
        pvNode = candidates.front().second;
        valueType = HashEntry::Exact;
    }

    candidates.pop_front();

    for (const auto [_, candidate] : candidates) {
        move(candidate, stone);

        auto candidateScore = -vcfSearch < NT == CutNode
                                  ? AllNode
                                  : CutNode > (static_cast<const Stone>(-stone),
                                               -alpha - 1,
                                               -alpha,
                                               depth - 1);

        undo(1);

        if (candidateScore >= Max - 225) {
            --candidateScore;
        } else if (candidateScore <= Min + 225) {
            ++candidateScore;
        }

        if (candidateScore > alpha && candidateScore < beta
            || (candidateScore == beta && beta == alpha + 1 && NT == PVNode)) {
            move(candidate, stone);

            candidateScore = -vcfSearch<NT>(static_cast<const Stone>(-stone),
                                            -beta,
                                            -candidateScore,
                                            depth - 1);

            undo(1);

            if (candidateScore >= Max - 225) {
                --candidateScore;
            } else if (candidateScore <= Min + 225) {
                ++candidateScore;
            }
        }

        if (candidateScore > bestScore) {
            bestScore = candidateScore;

            if (bestScore >= beta) {
                vcfTT.insert(vcfTT.hash(), HashEntry::LowerBound, candidate, depth, bestScore, stone);
                ++cutNodeCount;

                return bestScore;
            }

            if (bestScore > alpha) {
                alpha = bestScore;
                pvNode = candidate;
                valueType = HashEntry::Exact;
            }
        }
    }

    if (NT == CutNode && bestScore == alpha) {
        return bestScore;
    }

    vcfTT.insert(vcfTT.hash(), valueType, pvNode, depth, bestScore, stone);

    return bestScore;
}
