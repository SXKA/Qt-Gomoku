#include "engine.h"

using namespace Gomoku;

QCache<std::string, int> Engine::largeCache = QCache<std::string, int>(16777216);
QCache<std::string, int> Engine::smallCache = QCache<std::string, int>(65536);

const QHash<std::string, Score> Engine::shapeScoreHash = {
    {"001000", One}, {"000100", One},
    {"010100", Two}, {"001010", Two}, {"001100", Two},
    {"011100", Three}, {"001110", Three}, {"010110", Three}, {"011010", Three},
    {"11110", Four}, {"01111", Four}, {"10111", Four}, {"11011", Four}, {"11101", Four},
    {"011110", OpenFours},
    {"11111", Five}
};

aho_corasick::trie Engine::trie = aho_corasick::trie();

bool operator< (const QPoint &lhs, const QPoint &rhs)
{
    return lhs.y() == rhs.y() ? lhs.x() < rhs.x() : rhs.y() < lhs.y();
}

Engine::Engine()
    : movesGenerator(&board)
    , blackTotalScore({0})
, whiteTotalScore({0})
, board({})
{
    trie.only_whole_words();

    for (const auto &shapeScore : shapeScoreHash.keys()) {
        trie.insert(shapeScore);
    }
}

bool Engine::isLegal(const QPoint &point)
{
    return point.x() >= 0 && point.x() < 15 && point.y() >= 0 && point.y() < 15;
}

void Engine::move(const QPoint &point, const Stone &stone)
{
    movesGenerator.move(point);
    record.push(point);
    translationTable.translate(point, stone);
    board[point.x()][point.y()] = stone;

    updateScore(point);
}

void Engine::undo(const int &step)
{
    for (int i = 0; i < step; ++i) {
        auto point = record.top();

        movesGenerator.undo(point);
        record.pop();
        translationTable.translate(point, checkStone(point));
        board[point.x()][point.y()] = Empty;

        restoreScore();
    }
}

bool Engine::gameOver(const QPoint &point, const Stone &stone) const
{
    QString winner = stone == Black ? "Black" : "White";

    winner.append(" win!");

    switch (gameState(point, stone)) {
    case Draw:
        QMessageBox::information(nullptr, "Result", "Draw!", QMessageBox::Ok, QMessageBox::NoButton);

        return true;
    case Undecided:
        break;
    case Win:
        QMessageBox::information(nullptr, "Result", winner, QMessageBox::Ok, QMessageBox::NoButton);

        return true;
    }

    return false;
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

        if (count == 5) {
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
    const auto score = pvs(stone, Min, Max - 1, LimitDepth, PVNode);

    qInfo() << "Score: " << score;

    return bestPoint;
}

QPoint Engine::lastStone() const
{
    return record.empty() ? QPoint() : record.top();
}

void Engine::restoreScore()
{
    blackTotalScore.pop_back();
    whiteTotalScore.pop_back();
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
    const auto x = point.x();
    const auto y = point.y();

    for (int i = 0; i < 15; ++i) {
        insertToLine(0, QPoint(i, y));
        insertToLine(1, QPoint(x, i));
    }

    auto base = std::min(x, y);

    for (int i = x - base, j = y - base; i < 15 && j < 15; ++i, ++j) {
        insertToLine(2, QPoint(i, j));
    }

    base = std::min(y, 14 - x);

    for (int i = x + base, j = y - base; i >= 0 && j < 15; --i, ++j) {
        insertToLine(3, QPoint(i, j));
    }

    std::array<int, 4> blackLineScores{};
    std::array<int, 4> whiteLineScores{};

    for (int i = 0; i < 4; ++i) {
        if (blackLines[i].size() < 15) {
            blackLines[i].append(15 - blackLines[i].size(), ' ');
        }

        if (whiteLines[i].size() < 15) {
            whiteLines[i].append(15 - whiteLines[i].size(), ' ');
        }

        if (const auto cacheScore = largeCache[blackLines[i]]) {
            blackLineScores[i] += *cacheScore;
        } else {
            auto *accumulateScore = new int(blackLineScores[i]);
            const auto shapes = trie.parse_text(blackLines[i]);

            for (const auto &shape : shapes) {
                blackLineScores[i] += shapeScoreHash[shape.get_keyword()];
            }

            *accumulateScore = blackLineScores[i] - *accumulateScore;

            largeCache.insert(blackLines[i], accumulateScore);
        }

        if (const auto cacheScore = largeCache[whiteLines[i]]) {
            whiteLineScores[i] += *cacheScore;
        } else {
            auto *accumulateScore = new int(whiteLineScores[i]);
            const auto shapes = trie.parse_text(whiteLines[i]);

            for (const auto &shape : shapes) {
                whiteLineScores[i] += shapeScoreHash[shape.get_keyword()];
            }

            *accumulateScore = whiteLineScores[i] - *accumulateScore;

            largeCache.insert(whiteLines[i], accumulateScore);
        }
    }

    blackTotalScore.push_back(blackTotalScore.back());
    whiteTotalScore.push_back(whiteTotalScore.back());

    auto update = [this](const auto & blackLineScore, const auto & whiteLineScore) {
        blackTotalScore.back() += blackLineScore;
        whiteTotalScore.back() += whiteLineScore;
    };

    update(blackLineScores[0], whiteLineScores[0]);
    update(blackLineScores[1], whiteLineScores[1]);

    if (std::abs(y - x) <= 10) {
        update(blackLineScores[2], whiteLineScores[2]);
    }

    if (x + y >= 4 && x + y <= 24) {
        update(blackLineScores[3], whiteLineScores[3]);
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
    std::string blackLine(10, ' ');
    std::string whiteLine(10, ' ');
    int score = 0;

    for (int i = -5; i <= 5; ++i) {
        auto neighborhood = QPoint(point.x() + dx * i, point.y() + dy * i);

        if (!isLegal(neighborhood)) {
            continue;
        }

        if (neighborhood == point) {
            blackLine[i + 5] = '1';
            whiteLine[i + 5] = '1';

            continue;
        }

        switch (checkStone(neighborhood)) {
        case Empty:
            blackLine[i + 5] = '0';
            whiteLine[i + 5] = '0';

            break;
        case Black:
            blackLine[i + 5] = '1';

            break;
        case White:
            whiteLine[i + 5] = '1';

            break;
        }
    }

    if (const auto &cacheScore = smallCache[blackLine]) {
        score += *cacheScore;
    } else {
        auto *accumulateScore = new int(0);
        const auto shapes = trie.parse_text(blackLine);

        for (const auto &shape : shapes) {
            *accumulateScore += std::max(*accumulateScore,
                                         static_cast<int>(shapeScoreHash[shape.get_keyword()]));
        }

        smallCache.insert(blackLine, accumulateScore);

        score += *accumulateScore;
    }

    if (const auto &cacheScore = smallCache[whiteLine]) {
        score += *cacheScore;
    } else {
        auto *accumulateScore = new int(0);
        const auto shapes = trie.parse_text(whiteLine);

        for (const auto &shape : shapes) {
            *accumulateScore += std::max(*accumulateScore,
                                         static_cast<int>(shapeScoreHash[shape.get_keyword()]));
        }

        smallCache.insert(whiteLine, accumulateScore);

        score += *accumulateScore;
    }

    return score;
}


inline int Engine::evaluate(const Stone &stone) const
{
    return stone == Black ? blackTotalScore.back() : whiteTotalScore.back();
}

int Engine::pvs(const Stone &stone, int alpha, const int &beta, const int &depth,
                const NodeType &nodeType)
{
    if (depth != LimitDepth && translationTable.contains(translationTable.hash(), depth)) {
        const auto &entry = translationTable.at(translationTable.hash());

        switch (entry.type) {
        case Zobrist::hashEntry::Empty:
            qWarning() << "Hashing error!";

            break;
        case Zobrist::hashEntry::Exact:
            return entry.score;
        case Zobrist::hashEntry::UpperBound:
            if (entry.score >= beta) {
                return beta;
            }

            break;
        case Zobrist::hashEntry::LowBound:
            if (entry.score <= alpha) {
                return alpha;
            }

            break;
        }
    }

    const auto &firstScore = evaluate(stone);
    const auto &secondScore = evaluate(static_cast<const Stone>(-stone));

    if (firstScore >= Five) {
        return Max - 1000 - (LimitDepth - depth);
    }

    if (secondScore >= Five) {
        return Min + 1000 + (LimitDepth - depth);
    }

    if (depth <= 0 || movesGenerator.empty()) {
        const auto score = firstScore - secondScore;

        translationTable.insert(translationTable.hash(), Zobrist::hashEntry::Exact, depth, score);

        return score;
    }

    if (nodeType != PVNode) {
        const auto score = -pvs(static_cast<const Stone>(-stone), -beta, -beta + 1, depth - R - 1,
                                nodeType);

        if (score >= beta) {
            translationTable.insert(translationTable.hash(), Zobrist::hashEntry::UpperBound, depth, score);

            return score;
        }
    }

    QList<QPair<int, QPoint>> candidates;
    const auto &moves = movesGenerator.generate();

    for (const auto &move : moves) {
        candidates.emplace_back(evaluatePoint(move), move);
    }

    std::sort(candidates.begin(), candidates.end(), std::greater<>());

    if (candidates.size() > 8) {
        candidates.resize(8);
    }

    if (depth == LimitDepth) {
        bestPoint = candidates.front().second;
    }

    move(candidates.front().second, stone);

    auto bestScore = -pvs(static_cast<const Stone>(-stone), -beta, -alpha, depth - 1,
                          static_cast<const NodeType>(-nodeType));

    undo(1);

    if (bestScore >= beta) {
        translationTable.insert(translationTable.hash(), Zobrist::hashEntry::UpperBound, depth, bestScore);

        return bestScore;
    }

    candidates.pop_front();

    auto valueType = Zobrist::hashEntry::LowBound;

    for (const auto& [score, candidate] : candidates) {
        alpha = std::max(alpha, bestScore);

        move(candidate, stone);

        auto candidateScore = -pvs(static_cast<const Stone>(-stone), -alpha - 1, -alpha, depth - 1,
                                   nodeType == CutNode ? AllNode : CutNode);

        undo(1);

        if (candidateScore > alpha && candidateScore < beta || candidateScore == beta && beta == alpha + 1
                && nodeType == PVNode) {
            if (candidateScore == alpha + 1) {
                candidateScore = alpha;
            }

            move(candidate, stone);

            candidateScore = -pvs(static_cast<const Stone>(-stone), -beta, -candidateScore, depth - 1,
                                  nodeType);

            undo(1);
        }

        if (candidateScore > bestScore) {
            bestScore = candidateScore;

            if (bestScore >= beta) {
                translationTable.insert(translationTable.hash(), Zobrist::hashEntry::UpperBound, depth, bestScore);

                return bestScore;
            }

            if (depth == LimitDepth) {
                bestPoint = candidate;
            }
            valueType = Zobrist::hashEntry::Exact;
        }
    }

    if (nodeType == CutNode && bestScore == alpha) {
        return bestScore;
    }

    translationTable.insert(translationTable.hash(), valueType, depth, bestScore);

    return bestScore;
}
