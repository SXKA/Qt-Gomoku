#include "engine.h"

using namespace Gomoku;

aho_corasick::trie Engine::trie = aho_corasick::trie();
aho_corasick::trie Engine::checkTrie = aho_corasick::trie();
QCache<std::string, int> Engine::largeCache = QCache<std::string, int>(16777216);
QCache<std::string, int> Engine::smallCache = QCache<std::string, int>(65536);
const QHash<std::string, Score> Engine::shapeScoreTable = {
    {"00100", One},
    {"01010", Two}, {"001100", Two},
    {"01110", Three}, {"010110", Three}, {"011010", Three},
    {"11110", Four}, {"01111", Four}, {"10111", Four}, {"11011", Four}, {"11101", Four},
    {"011110", OpenFours},
    {"11111", Five}
};

inline bool operator< (const QPoint &lhs, const QPoint &rhs)
{
    return lhs.y() == rhs.y() ? lhs.x() < rhs.x() : rhs.y() < lhs.y();
}

Engine::Engine()
    : generator(&board)
    , board({})
, blackShapes({})
, whiteShapes({})
, blackScores({})
, whiteScores({})
, checkSum(transpositionTable.hash())
, cutNodeCount(0)
, hitNodeCount(0)
, nodeCount(0)
, blackTotalScore(0)
, whiteTotalScore(0)
{
    trie.only_whole_words();
    checkTrie.only_whole_words();

    for (auto it = shapeScoreTable.cbegin(); it != shapeScoreTable.cend(); ++it) {
        trie.insert(it.key());

        if (it.value() >= Three) {
            checkTrie.insert(it.key());
        }
    }

    blackShapes.fill(std::string(15, '0'));
    whiteShapes.fill(std::string(15, '0'));
}

bool Engine::isLegal(const QPoint &point)
{
    return point.x() >= 0 && point.x() < 15 && point.y() >= 0 && point.y() < 15;
}

void Engine::move(const QPoint &point, const Stone &stone)
{
    const auto &x = point.x();
    const auto &y = point.y();

    generator.move(point);
    history.moves.push(point);
    history.blackScores.push(blackScores);
    history.whiteScores.push(whiteScores);
    history.blackTotalScore.push(blackTotalScore);
    history.whiteTotalScore.push(whiteTotalScore);
    transpositionTable.transpose(point, stone);
    board[x][y] = stone;

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

    updateScore(point);
}

void Engine::undo(const int &step)
{
    for (int i = 0; i < step; ++i) {
        auto &point = history.moves.top();
        auto &firstShapes = checkStone(point) == Black ? blackShapes : whiteShapes;
        auto &secondShapes = checkStone(point) == Black ? whiteShapes : blackShapes;
        const auto &x = point.x();
        const auto &y = point.y();

        firstShapes[y][x] = '0';
        secondShapes[y][x] = '0';
        firstShapes[x + 15][y] = '0';
        secondShapes[x + 15][y] = '0';

        if (qAbs(y - x) <= 10) {
            firstShapes[y - x + 40][qMin(x, y)] = '0';
            secondShapes[y - x + 40][qMin(x, y)] = '0';
        }

        if (x + y >= 4 && x + y <= 24) {
            firstShapes[x + y + 47][qMin(y, 14 - x)] = '0';
            secondShapes[x + y + 47][qMin(y, 14 - x)] = '0';
        }

        generator.undo(point);
        history.moves.pop();
        transpositionTable.transpose(point, checkStone(point));
        board[point.x()][point.y()] = Empty;

        restoreScore();
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

    for (int i = 0; i < 4; ++i) {
        int count = 1;

        for (int j = 0; j < 2; ++j) {
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
    const auto &last = lastMove();

    if (history.moves.empty() || (history.moves.size() == 1 && last != QPoint(7, 7)
                                  && checkStone(last) != stone)) {
        return {7, 7};
    }

    checkSum = transpositionTable.hash();

    const QTime &time = QTime::currentTime();
    const auto &score = pvs(stone, Min, Max, LIMIT_DEPTH, PVNode);
    const auto &elapsedTime = time.msecsTo(QTime::currentTime());

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
    return history.moves.empty() ? QPoint(-1, -1) : history.moves.top();
}

void Engine::restoreScore()
{
    blackScores = history.blackScores.top();
    whiteScores = history.whiteScores.top();
    blackTotalScore = history.blackTotalScore.top();
    whiteTotalScore = history.whiteTotalScore.top();
    history.blackScores.pop();
    history.whiteScores.pop();
    history.blackTotalScore.pop();
    history.whiteTotalScore.pop();
}

void Engine::updateScore(const QPoint &point)
{

    std::array<int, 4> blackLineScores{};
    std::array<int, 4> whiteLineScores{};
    const auto &x = point.x();
    const auto &y = point.y();
    const std::array<bool, 4> valid = {true, true, qAbs(y - x) <= 10, x + y >= 4 && x + y <= 24};
    const std::reference_wrapper<std::string> blackLines[] = {blackShapes[y], blackShapes[x + 15], blackShapes[y - x + 40], blackShapes[valid[3] ? x + y + 47 : 0]};
    const std::reference_wrapper<std::string> whiteLines[] = {whiteShapes[y], whiteShapes[x + 15], whiteShapes[y - x + 40], whiteShapes[valid[3] ? x + y + 47 : 0]};

    for (int i = 0; i < 4; ++i) {
        if (valid[i]) {
	        if (const auto &cacheScore = largeCache[blackLines[i]]) {
	            blackLineScores[i] = *cacheScore;
	        } else {
	            const auto &shapes = trie.parse_text(blackLines[i]);

	            for (const auto &shape : shapes) {
	                blackLineScores[i] += shapeScoreTable[shape.get_keyword()];
	            }

	            largeCache.insert(blackLines[i], new int(blackLineScores[i]));
	        }

	        if (const auto &cacheScore = largeCache[whiteLines[i]]) {
	            whiteLineScores[i] = *cacheScore;
	        } else {
	            const auto &shapes = trie.parse_text(whiteLines[i]);

	            for (const auto &shape : shapes) {
	                whiteLineScores[i] += shapeScoreTable[shape.get_keyword()];
	            }

	            largeCache.insert(whiteLines[i], new int(whiteLineScores[i]));
	        }
        }
    }

    auto update = [this](const auto & index, const auto & blackLineScore, const auto & whiteLineScore) {
        blackTotalScore -= blackScores[index];
        whiteTotalScore -= whiteScores[index];
        blackScores[index] = blackLineScore;
        whiteScores[index] = whiteLineScore;
        blackTotalScore += blackScores[index];
        whiteTotalScore += whiteScores[index];
    };

    update(y, blackLineScores[0], whiteLineScores[0]);
    update(x + 15, blackLineScores[1], whiteLineScores[1]);

    if (valid[2]) {
        update(y - x + 40, blackLineScores[2], whiteLineScores[2]);
    }

    if (valid[3]) {
        update(x + y + 47, blackLineScores[3], whiteLineScores[3]);
    }
}

bool Engine::inCheck(const Stone &stone)
{   
    int firstCount = 0;
    const auto &firstShapes = stone == Black ? blackShapes : whiteShapes;
    const auto &secondShapes = stone == Black ? whiteShapes : blackShapes;
    const auto &firstScores = stone == Black ? blackScores : whiteScores;
    const auto &secondScores = stone == Black ? whiteScores : blackScores;

    for (int i = 0; i < 72; ++i) {
        if (firstScores[i] >= OpenFours) {
            return false;
        }

        if (firstScores[i] >= Three) {
            const auto &shapes = checkTrie.parse_text(firstShapes[i]);

            for (const auto &shape : shapes) {
                const auto &keyword = shape.get_keyword();
                const auto &count = std::count(keyword.cbegin(), keyword.cend(), '1');

                firstCount = qMax(firstCount, count);
            }
        }
    }

    if (firstCount == 4) {
        return false;
    }

    int line;
    int start;
    int end;
    int secondCount = 0;

    for (int i = 0; i < 72; ++i) {
        if (secondScores[i] >= Three) {
    		const auto &shapes = checkTrie.parse_text(secondShapes[i]);

    		for (const auto &shape : shapes) {
    			const auto &keyword = shape.get_keyword();
    			const auto &count = std::count(keyword.cbegin(), keyword.cend(), '1');

    			if (count > secondCount) {
    				line = i;
    				start = shape.get_start();
    				end = shape.get_end();
    				secondCount = count;
    			}
    		}
        }
    }

    if (secondCount == 4) {
        QList<int> indices;

        for (int i = start; i <= end; ++i) {
            if (secondShapes[line][i] == '0') {
                indices.push_back(i);
            }
        }

        escapes.clear();

        for (const auto &index : indices) {
            QPoint point;

            if (line < 15) {
                point.setX(index);
                point.setY(line);
            } else if (line < 30) {
                point.setX(line - 15);
                point.setY(index);
            } else if (line < 51) {
                point.setX(qMax(0, 40 - line) + index);
                point.setY(qMax(0, line - 40) + index);
            } else {
                point.setX(qMin(14, line - 47) - index);
                point.setY(qMax(0, line - 61) + index);
            }

            if (isLegal(point)) {
                escapes.insert(point);
            } else {
                return false;
            }
        }

        return true;
    }

    return false;
}

int Engine::evaluatePoint(const QPoint &point) const
{
    int score = 0;

    for (int i = 0; i < 4; ++i) {
        score += lineScore(point, i);
    }

    return score;
}

int Engine::lineScore(const QPoint &point, const int &direction) const
{
    bool isolated = true;
    constexpr std::array<int, 4> dx = {1, 0, 1, 1};
    constexpr std::array<int, 4> dy = {0, 1, 1, -1};
    std::array<Stone, 5> stones{Empty, Empty, Empty, Empty, Empty};

    for (int i = -2; i <= 2; ++i) {
        const auto &neighborhood = QPoint(point.x() + dx[direction] * i, point.y() + dy[direction] * i);

        if (!isLegal(neighborhood)) {
            continue;
        }

        const auto &stone = checkStone(neighborhood);

        if (stone != Empty) {
            isolated = false;
            stones[i + 2] = stone;
        }
    }

    if (isolated) {
        return 0;
    }

    if (!(stones[0] + stones[1] || stones[3] + stones[4]) && stones[0] != stones[1]
            && stones[3] != stones[4]) {
        return 0;
    }

    int offset;
    std::string blackLine;
    std::string whiteLine;
    const auto &x = point.x();
    const auto &y = point.y();

    switch (direction) {
    case 0:
        offset = qMax(0, x - 4);
        blackLine = blackShapes[y];
        whiteLine = whiteShapes[y];
        blackLine[x] = '1';
        whiteLine[x] = '1';

        break;
    case 1:
        offset = qMax(0, y - 4);
        blackLine = blackShapes[x + 15];
        whiteLine = whiteShapes[x + 15];
        blackLine[y] = '1';
        whiteLine[y] = '1';

        break;
    case 2:
        if (qAbs(y - x) > 10) {
            return 0;
        }

        offset = qMax(0, qMin(x, y) - 4);
        blackLine = blackShapes[y - x + 40];
        whiteLine = whiteShapes[y - x + 40];
        blackLine[qMin(x, y)] = '1';
        whiteLine[qMin(x, y)] = '1';

        break;
    case 3:
        if (x + y < 4 || x + y > 24) {
            return 0;
        }

        offset = qMax(0, qMin(y, 14 - x) - 4);
        blackLine = blackShapes[x + y + 47];
        whiteLine = whiteShapes[x + y + 47];
        blackLine[qMin(y, 14 - x)] = '1';
        whiteLine[qMin(y, 14 - x)] = '1';

        break;
    default:
        break;
    }

    blackLine = blackLine.substr(offset, qMin(9, 15 - offset));
    whiteLine = whiteLine.substr(offset, qMin(9, 15 - offset));

    int score = 0;

    if (const auto &cacheScore = smallCache[blackLine]) {
        score += *cacheScore;
    } else {
        const auto accumulateScore = new int(0);
        const auto &shapes = trie.parse_text(blackLine);

        for (const auto &shape : shapes) {
            *accumulateScore += shapeScoreTable[shape.get_keyword()];
        }

        smallCache.insert(blackLine, accumulateScore);

        score += *accumulateScore;
    }

    if (const auto &cacheScore = smallCache[whiteLine]) {
        score += *cacheScore;
    } else {
        const auto accumulateScore = new int(0);
        const auto &shapes = trie.parse_text(whiteLine);

        for (const auto &shape : shapes) {
            *accumulateScore += shapeScoreTable[shape.get_keyword()];
        }

        smallCache.insert(whiteLine, accumulateScore);

        score += *accumulateScore;
    }

    return score;
}

inline int Engine::evaluate(const Stone &stone) const
{
    return stone == Black ? blackTotalScore : whiteTotalScore;
}

int Engine::pvs(const Stone &stone, int alpha, const int &beta, const int &depth,
                const NodeType &nodeType, const bool &nullOk)
{
    ++nodeCount;

    QPair<QPoint, QPoint> goodMovePair{{-1, -1}, {-1, -1}};

    if (transpositionTable.hash() != checkSum) {
        const auto &probeValue = transpositionTable.probe(transpositionTable.hash(), alpha, beta, depth,
                                                          goodMovePair);

        if (probeValue != Zobrist::MISS) {
            ++hitNodeCount;

            return probeValue;
        }
    }

    const auto &firstScore = evaluate(stone);
    const auto &secondScore = evaluate(static_cast<const Stone>(-stone));

    if (firstScore >= Five) {
        ++cutNodeCount;

        return Max - (LIMIT_DEPTH - depth) - 1;
    }

    if (secondScore >= Five) {
        ++cutNodeCount;

        return Min + (LIMIT_DEPTH - depth) + 1;
    }

    if (depth <= 0 || generator.empty()) {
        return firstScore - secondScore;
    }

    const bool &extend = inCheck(stone);

    if (nodeType != PVNode && nullOk && !extend) {
        R = depth >= 6 ? 3 : 2;

        const auto &score = -pvs(static_cast<const Stone>(-stone), -beta, -beta + 1, depth - R - 1,
                                 nodeType, false);

        if (score >= beta) {
            ++cutNodeCount;

            return beta;
        }
    }

    QList<QPair<int, QPoint>> candidates;
    auto moves = generator.generate();

    if (extend) {
        moves = escapes;
    } else {
        moves.remove(goodMovePair.first);
        moves.remove(goodMovePair.second);
    }

    for (const auto &move : moves) {
        candidates.emplace_back(evaluatePoint(move), move);
    }

    std::sort(candidates.begin(), candidates.end(), std::greater());

    if (!extend && goodMovePair.second != QPoint(-1, -1) && goodMovePair.second != goodMovePair.first) {
        candidates.emplaceFront(evaluatePoint(goodMovePair.second), goodMovePair.second);
    }

    if (!extend && goodMovePair.first != QPoint(-1, -1)) {
        candidates.emplaceFront(evaluatePoint(goodMovePair.first), goodMovePair.first);
    }

    if (candidates.size() > LIMIT_WIDTH) {
        candidates.resize(LIMIT_WIDTH);
    }

    move(candidates.front().second, stone);

    auto bestScore = -pvs(static_cast<const Stone>(-stone), -beta, -alpha, depth + extend - 1,
                          static_cast<const NodeType>(-nodeType));

    undo(1);

    if (bestScore >= beta) {
        if (transpositionTable.hash() != checkSum) {
            transpositionTable.insert(transpositionTable.hash(), Zobrist::HashEntry::LowerBound, depth,
                                      bestScore, candidates.front().second);
        }

        ++cutNodeCount;

        return bestScore;
    }

    QPoint pvNode{-1, -1};
    auto valueType = Zobrist::HashEntry::UpperBound;

    if (bestScore > alpha) {
        alpha = bestScore;
        pvNode = candidates.front().second;
        valueType = Zobrist::HashEntry::Exact;

        if (transpositionTable.hash() == checkSum) {
            bestPoint = candidates.front().second;
        }
    }

    candidates.pop_front();

    for (const auto& [score, candidate] : candidates) {
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
                    transpositionTable.insert(transpositionTable.hash(), Zobrist::HashEntry::LowerBound, depth,
                                              bestScore, candidate);
                }

                ++cutNodeCount;

                return bestScore;
            }

            if (bestScore > alpha) {
                alpha = bestScore;
                pvNode = candidate;
                valueType = Zobrist::HashEntry::Exact;

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
        transpositionTable.insert(transpositionTable.hash(), valueType, depth, bestScore, pvNode);
    }

    return bestScore;
}
