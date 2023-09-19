#include "engine.h"

using namespace Gobang;

QCache<std::string, int> Engine::largeCache = QCache<std::string, int>(16777216);
QCache<std::string, int> Engine::smallCache = QCache<std::string, int>(65536);

const QHash<std::string, Score> Engine::shapeScoreHash = {
    {"001000", one}, {"000100", one},
    {"010100", two}, {"001010", two}, {"001100", two},
    {"011100", three}, {"001110", three}, {"010110", three}, {"011010", three},
    {"11110", four}, {"01111", four}, {"10111", four}, {"11011", four}, {"11101", four},
    {"011110", livingFour},
    {"11111", five}
};

inline bool operator< (const QPoint &lhs, const QPoint &rhs)
{
    return lhs.y() == rhs.y() ? lhs.x() < rhs.x() : rhs.y() < lhs.y();
}

Engine::Engine() : movesGenerator(&board), blackScores({}), whiteScores({}), blackTotalScore(0),
       whiteTotalScore(0)
{
    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            board[i][j] = empty;
        }
    }

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
        board[point.x()][point.y()] = empty;

        updateScore(point);
    }
}

bool Engine::gameOver(const QPoint &point, const Stone &stone) const
{
    QString winner = stone == black ? "Black" : "White";

    winner.append(" win!");

    switch (gameState(point, stone)) {
    case draw:
        QMessageBox::information(nullptr, "Result", "Draw!", QMessageBox::Ok, QMessageBox::NoButton);

        return true;
    case undecided:
        break;
    case win:
        QMessageBox::information(nullptr, "Result", winner, QMessageBox::Ok, QMessageBox::NoButton);

        return true;
    }

    return false;
}

bool Engine::isInitial(const bool &type, const Stone &stone) const
{
    if (!type) {
        if (record.empty()) {
            return true;
        }

        if (record.size() == 1) {
            return checkStone(record.top()) != stone;
        }
    }

    return record.empty();
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
            return win;
        }
    }

    for (const auto &row : board) {
        if (std::any_of(row.cbegin(), row.cend(), [](const auto & x) {
        return x == empty;
    })) {
            return undecided;
        }
    }

    return draw;
}

QPoint Engine::bestMove(const Stone &stone)
{
    const auto score = negamax(stone, limitDepth);

    qInfo() << "Score: " << score;

    return bestPoint;
}

QPoint Engine::lastStone() const
{
    return record.empty() ? QPoint() : record.top();
}

void Engine::updateScore(const QPoint &point)
{
    std::array<std::string, 4> blackLines;
    std::array<std::string, 4> whiteLines;
    auto insertToLine = [this, &blackLines, &whiteLines](const auto & i, const auto & p) {
        switch (checkStone(p)) {
        case empty:
            blackLines[i].push_back('0');
            whiteLines[i].push_back('0');

            break;
        case black:
            blackLines[i].push_back('1');
            whiteLines[i].push_back(' ');

            break;
        case white:
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

    if (std::abs(y - x) <= 10) {
        update(offsetIndex, blackLineScores[2], whiteLineScores[2]);
    }

    offsetIndex = x + y + 47;

    if (x + y >= 4 && x + y <= 24) {
        update(offsetIndex, blackLineScores[3], whiteLineScores[3]);
    }
}

int Engine::calculateScore(const QPoint &point)
{
    int score = 0;
    constexpr std::array<int, 4> dx = {1, 0, 1, 1};
    constexpr std::array<int, 4> dy = {0, 1, 1, -1};

    for (int i = 0; i < 4; ++i) {
        score += dScore(point, dx[i], dy[i]);
    }

    return score;
}

int Engine::dScore(const QPoint &point, const int &dx, const int &dy)
{
    std::string blackLine;
    std::string whiteLine;
    int score = 0;

    for (int i = -5; i <= 5; ++i) {
        auto neighborhood = QPoint(point.x() + dx * i, point.y() + dy * i);

        if (!isLegal(neighborhood)) {
            blackLine.push_back(' ');
            whiteLine.push_back(' ');

            continue;
        }

        if (neighborhood == point) {
            blackLine.push_back('1');
            whiteLine.push_back('1');

            continue;
        }

        switch (checkStone(neighborhood)) {
        case empty:
            blackLine.push_back('0');
            whiteLine.push_back('0');

            break;
        case black:
            blackLine.push_back('1');
            whiteLine.push_back(' ');

            break;
        case white:
            blackLine.push_back(' ');
            whiteLine.push_back('1');

            break;
        }
    }

    if (const auto &cacheScore = smallCache[blackLine]) {
        score += *cacheScore;
    } else {
        auto *accumulateScore = new int(0);
        const auto shapes = trie.parse_text(blackLine);

        for (const auto &shape : shapes) {
            *accumulateScore += std::max(*accumulateScore, static_cast<int>(shapeScoreHash[shape.get_keyword()]));
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
            *accumulateScore += std::max(*accumulateScore, static_cast<int>(shapeScoreHash[shape.get_keyword()]));
        }

        smallCache.insert(whiteLine, accumulateScore);

        score += *accumulateScore;
    }

    return score;
}


int Engine::evaluate(const Stone &stone) const
{
    return stone == black ? blackTotalScore : whiteTotalScore;
}

int Engine::negamax(const Stone &stone, const int &depth, int alpha, const int &beta)
{
    if (depth != limitDepth && translationTable.contains(translationTable.hash(), depth)) {
        const auto &entry = translationTable.at(translationTable.hash());

        switch (entry.type) {
        case zobrist::hashEntry::empty:
            qWarning() << "Hashing error!";

            break;
        case zobrist::hashEntry::exact:
            return entry.score;
        case zobrist::hashEntry::beta:
            if (entry.score >= beta) {
                return beta;
            }

            break;
        case zobrist::hashEntry::alpha:
            if (entry.score <= alpha) {
                return alpha;
            }

            break;
        }
    }

    const auto &firstScore = evaluate(stone);
    const auto &secondScore = evaluate(static_cast<const Stone>(!stone));

    if (firstScore >= five) {
        return maxScore - 1000 - (limitDepth - depth);
    }

    if (secondScore >= five) {
        return minScore + 1000 + (limitDepth - depth);
    }

    if (depth <= 0 || movesGenerator.empty()) {
        const auto score = firstScore - secondScore;

        translationTable.insert(translationTable.hash(), zobrist::hashEntry::exact, depth, score);

        return score;
    }

    const auto vacancyScore = -negamax(static_cast<const Stone>(!stone), depth - 1 - r, -beta,
                                       -beta + 1);

    if (vacancyScore >= beta) {
        translationTable.insert(translationTable.hash(), zobrist::hashEntry::beta, depth, beta);

        return beta;
    }

    QVector<QPair<int, QPoint>> candidates;
    const auto &moves = movesGenerator.generate();

    for (const auto &move : moves) {
        candidates.emplace_back(calculateScore(move), move);
    }

    std::sort(candidates.begin(), candidates.end(), std::greater<>());

    if (const int limit = 12 - (((limitDepth - depth) >> 1) << 1); candidates.size() > limit) {
        candidates.resize(limit);
    }

    auto valueType = zobrist::hashEntry::alpha;

    for (const auto& [score, candidate] : candidates) {
        move(candidate, stone);

        const auto candidateScore = -negamax(static_cast<const Stone>(!stone), depth - 1, -beta, -alpha);

        undo(1);

        if (candidateScore >= beta) {
            translationTable.insert(translationTable.hash(), zobrist::hashEntry::beta, depth, beta);

            return beta;
        }

        if (candidateScore > alpha) {
            if (depth == limitDepth) {
                bestPoint = candidate;
            }

            alpha = candidateScore;

            valueType = zobrist::hashEntry::exact;
        }
    }

    translationTable.insert(translationTable.hash(), valueType, depth, alpha);

    return alpha;
}