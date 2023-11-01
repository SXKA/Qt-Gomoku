#include "engine.h"

using namespace Gomoku;

aho_corasick::trie Engine::trie = aho_corasick::trie();
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

    for (const auto &shapeScore : shapeScoreTable.keys()) {
        trie.insert(shapeScore);
    }
}

bool Engine::isLegal(const QPoint &point)
{
    return point.x() >= 0 && point.x() < 15 && point.y() >= 0 && point.y() < 15;
}

void Engine::move(const QPoint &point, const Stone &stone)
{
    generator.move(point);
    movesHistory.push(point);
    blackScoresHistory.push(blackScores);
    whiteScoresHistory.push(whiteScores);
    blackTotalScoreHistory.push(blackTotalScore);
    whiteTotalScoreHistory.push(whiteTotalScore);
    transpositionTable.transpose(point, stone);
    board[point.x()][point.y()] = stone;

    updateScore(point);
}

void Engine::undo(const int &step)
{
    for (int i = 0; i < step; ++i) {
        auto point = movesHistory.top();

        generator.undo(point);
        movesHistory.pop();
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

    if (movesHistory.empty() || (movesHistory.size() == 1 && last != QPoint(7, 7)
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
    return movesHistory.empty() ? QPoint(-1, -1) : movesHistory.top();
}

inline void Engine::restoreScore()
{
    blackScores = blackScoresHistory.top();
    whiteScores = whiteScoresHistory.top();
    blackTotalScore = blackTotalScoreHistory.top();
    whiteTotalScore = whiteTotalScoreHistory.top();
    blackScoresHistory.pop();
    whiteScoresHistory.pop();
    blackTotalScoreHistory.pop();
    whiteTotalScoreHistory.pop();
}

void Engine::updateScore(const QPoint &point)
{
    std::array<std::string, 4> blackLines;
    std::array<std::string, 4> whiteLines;
    auto insertToLine = [this, &blackLines, &whiteLines](const auto & i, const auto & p) {
        switch (checkStone(p)) {
        case Empty:
            blackLines[i].push_back('0');
            whiteLines[i].push_back('0');

            break;
        case Black:
            blackLines[i].push_back('1');
            whiteLines[i].push_back(' ');

            break;
        case White:
            blackLines[i].push_back(' ');
            whiteLines[i].push_back('1');

            break;
        }
    };
    const auto &x = point.x();
    const auto &y = point.y();

    for (int i = 0; i < 15; ++i) {
        insertToLine(0, QPoint(i, y));
        insertToLine(1, QPoint(x, i));
    }

    auto base = qMin(x, y);

    for (int i = x - base, j = y - base; i < 15 && j < 15; ++i, ++j) {
        insertToLine(2, QPoint(i, j));
    }

    base = qMin(y, 14 - x);

    for (int i = x + base, j = y - base; i >= 0 && j < 15; --i, ++j) {
        insertToLine(3, QPoint(i, j));
    }

    std::array<int, 4> blackLineScores{};
    std::array<int, 4> whiteLineScores{};

    for (int i = 0; i < 4; ++i) {
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

    auto offsetIndex = x + 15;
    auto update = [this](const auto & index, const auto & blackLineScore, const auto & whiteLineScore) {
        blackTotalScore -= blackScores[index];
        whiteTotalScore -= whiteScores[index];
        blackScores[index] = blackLineScore;
        whiteScores[index] = whiteLineScore;
        blackTotalScore += blackScores[index];
        whiteTotalScore += whiteScores[index];
    };

    update(y, blackLineScores[0], whiteLineScores[0]);
    update(offsetIndex, blackLineScores[1], whiteLineScores[1]);

    offsetIndex = y - x + 40;

    if (qAbs(y - x) <= 10) {
        update(offsetIndex, blackLineScores[2], whiteLineScores[2]);
    }

    offsetIndex = x + y + 47;

    if (x + y >= 4 && x + y <= 24) {
        update(offsetIndex, blackLineScores[3], whiteLineScores[3]);
    }
}

int Engine::evaluatePoint(const QPoint &point) const
{
    int score = 0;
    constexpr std::array<int, 4> dx = {1, 0, 1, 1};
    constexpr std::array<int, 4> dy = {0, 1, 1, -1};

    for (int i = 0; i < 4; ++i) {
        score += lineScore(point, dx[i], dy[i]);
    }

    return score;
}

int Engine::lineScore(const QPoint &point, const int &dx, const int &dy) const
{
    bool isolated = true;
    std::array<Stone, 5> stones{Empty, Empty, Empty, Empty, Empty};

    for (int i = -2; i <= 2; ++i) {
        const auto &neighborhood = QPoint(point.x() + dx * i, point.y() + dy * i);

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

    std::string blackLine;
    std::string whiteLine;

    for (int i = -4; i <= 4; ++i) {
        const auto &neighborhood = QPoint(point.x() + dx * i, point.y() + dy * i);

        if (!i) {
            blackLine.push_back('1');
            whiteLine.push_back('1');

            continue;
        }

        if (!isLegal(neighborhood)) {
            continue;
        }

        switch (checkStone(neighborhood)) {
        case Empty:
            blackLine.push_back('0');
            whiteLine.push_back('0');

            break;
        case Black:
            blackLine.push_back('1');
            whiteLine.push_back(' ');

            break;
        case White:
            blackLine.push_back(' ');
            whiteLine.push_back('1');

            break;
        }
    }

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

    if (nodeType != PVNode && nullOk) {
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

    moves.remove(goodMovePair.first);
    moves.remove(goodMovePair.second);

    for (const auto &move : moves) {
        candidates.emplace_back(evaluatePoint(move), move);
    }

    std::sort(candidates.begin(), candidates.end(), std::greater());

    if (goodMovePair.second != QPoint(-1, -1) && goodMovePair.second != goodMovePair.first) {
        candidates.emplaceFront(evaluatePoint(goodMovePair.second), goodMovePair.second);
    }

    if (goodMovePair.first != QPoint(-1, -1)) {
        candidates.emplaceFront(evaluatePoint(goodMovePair.first), goodMovePair.first);
    }

    if (candidates.size() > LIMIT_WIDTH) {
        candidates.resize(LIMIT_WIDTH);
    }

    bool extend = false;

    if (candidates.front().first >= Five) {
        extend = true;
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
        extend = false;

        if (score >= Five) {
            extend = true;
        }

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
