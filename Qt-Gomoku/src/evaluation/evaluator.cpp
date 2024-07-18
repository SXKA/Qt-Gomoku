#include "evaluator.h"

#include <QCache>
#include <QHash>
#include <QList>
#include <QtGlobal>

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

#ifdef emit
#undef emit
#include "../algorithm/aho_corasick.hpp"
#endif

using namespace Evaluation;

namespace {
aho_corasick::trie trie;
aho_corasick::trie fourTrie;
QCache<std::string, bool> fourCache = QCache<std::string, bool>{1 << 16};
QCache<std::string, int> scoreCache = QCache<std::string, int>{1 << 23};
const QHash<std::string, Score> shapeScoreTable = {{"00100", One},
                                                   {"01010", Two},
                                                   {"00110", Two},
                                                   {"01100", Two},
                                                   {"01110", Three},
                                                   {"010110", Three},
                                                   {"011010", Three},
                                                   {"11110", Four},
                                                   {"01111", Four},
                                                   {"10111", Four},
                                                   {"11011", Four},
                                                   {"11101", Four},
                                                   {"011110", OpenFour},
                                                   {"11111", Five}};
} // namespace

Evaluator::Evaluator(std::array<std::string, 72> *blackShapes,
                     std::array<std::string, 72> *whiteShapes)
    : blackShapes(blackShapes)
    , whiteShapes(whiteShapes)
    , blackScores({})
    , whiteScores({})
    , blackTotalScore(0)
    , whiteTotalScore(0)
{
    trie.only_whole_words();
    fourTrie.only_whole_words();

    for (auto it = shapeScoreTable.cbegin(); it != shapeScoreTable.cend(); ++it) {
        trie.insert(it.key());
    }

    fourTrie.insert("11110");
    fourTrie.insert("01111");
    fourTrie.insert("10111");
    fourTrie.insert("11011");
    fourTrie.insert("11101");
}

void Evaluator::restore()
{
    blackScores = history.blackScores.top();
    whiteScores = history.whiteScores.top();
    blackTotalScore = history.blackTotalScores.top();
    whiteTotalScore = history.whiteTotalScores.top();
    history.blackScores.pop();
    history.whiteScores.pop();
    history.blackTotalScores.pop();
    history.whiteTotalScores.pop();
}

void Evaluator::update(const QPoint &move)
{
    history.blackScores.push(blackScores);
    history.whiteScores.push(whiteScores);
    history.blackTotalScores.push(blackTotalScore);
    history.whiteTotalScores.push(whiteTotalScore);

    std::array<int, 4> blackLineScores{};
    std::array<int, 4> whiteLineScores{};
    const auto &[x, y] = move;
    const std::array<bool, 4> valid = {true, true, qAbs(y - x) <= 10, x + y >= 4 && x + y <= 24};
    const std::reference_wrapper<const std::string> blackLines[]
        = {(*blackShapes)[y],
           (*blackShapes)[x + 15],
           (*blackShapes)[y - x + 40],
           (*blackShapes)[valid[3] ? x + y + 47 : 0]};
    const std::reference_wrapper<const std::string> whiteLines[]
        = {(*whiteShapes)[y],
           (*whiteShapes)[x + 15],
           (*whiteShapes)[y - x + 40],
           (*whiteShapes)[valid[3] ? x + y + 47 : 0]};

    for (size_t i = 0; i < 4; ++i) {
        if (valid[i]) {
            if (const auto &cacheScore = scoreCache[blackLines[i]]) {
                blackLineScores[i] = *cacheScore;
            } else {
                const auto shapes = trie.parse_text(blackLines[i]);

                for (const auto &shape : shapes) {
                    blackLineScores[i] += shapeScoreTable[shape.get_keyword()];
                }

                scoreCache.insert(blackLines[i], new int(blackLineScores[i]));
            }

            if (const auto &cacheScore = scoreCache[whiteLines[i]]) {
                whiteLineScores[i] = *cacheScore;
            } else {
                const auto shapes = trie.parse_text(whiteLines[i]);

                for (const auto &shape : shapes) {
                    whiteLineScores[i] += shapeScoreTable[shape.get_keyword()];
                }

                scoreCache.insert(whiteLines[i], new int(whiteLineScores[i]));
            }
        }
    }

    auto update = [this](const auto &index, const auto &blackLineScore, const auto &whiteLineScore) {
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

bool Evaluator::isFourMove(const QPoint &move, const Stone &stone) const
{
    const auto &[x, y] = move;

    for (int d = 0; d < 4; ++d) {
        if (d == 2 && qAbs(y - x) > 10) {
            continue;
        }

        if (d == 3 && x + y < 4 || x + y > 24) {
            continue;
        }

        auto [line, offset] = lineOffsetPair(move, d);
        std::string stoneLine = stone == Black ? (*blackShapes)[line] : (*whiteShapes)[line];

        stoneLine[offset + 4] = '1';

        size_t count = offset < 0 ? offset + 9 : 9;

        offset = qMax(0, offset);

        if (offset + count > stoneLine.size()) {
            count = stoneLine.size() - offset;
        }

        stoneLine = std::string_view{stoneLine}.substr(offset, count);

        if (const auto &cacheFour = fourCache[stoneLine]; cacheFour) {
            if (*cacheFour) {
                return true;
            }

            continue;
        }

        const auto shapes = fourTrie.parse_text(stoneLine);

        if (!shapes.empty()) {
            fourCache.insert(stoneLine, new bool(true));

            return true;
        }

        fourCache.insert(stoneLine, new bool(false));
    }

    return false;
}

int Evaluator::evaluate(const Stone &stone) const
{
    return stone == Black ? blackTotalScore : whiteTotalScore;
}

QPair<int, int> Evaluator::evaluateMove(const QPoint &move, const int &direction) const
{
    const auto &[x, y] = move;

    if (direction == 2 && qAbs(y - x) > 10) {
        return {0, 0};
    }

    if (direction == 3 && x + y < 4 || x + y > 24) {
        return {0, 0};
    }

    auto [line, offset] = lineOffsetPair(move, direction);
    std::string blackLine = (*blackShapes)[line];
    std::string whiteLine = (*whiteShapes)[line];

    blackLine[offset + 4] = '1';
    whiteLine[offset + 4] = '1';

    size_t count = offset < 0 ? offset + 9 : 9;

    offset = qMax(0, offset);

    if (offset + count > blackLine.size()) {
        count = blackLine.size() - offset;
    }

    blackLine = std::string_view{blackLine}.substr(offset, count);
    whiteLine = std::string_view{whiteLine}.substr(offset, count);

    int blackScore = 0;

    if (const auto &cacheScore = scoreCache[blackLine]) {
        blackScore += *cacheScore;
    } else {
        const auto accumulateScore = new int(0);
        const auto shapes = trie.parse_text(blackLine);

        for (const auto &shape : shapes) {
            *accumulateScore += shapeScoreTable[shape.get_keyword()];
        }

        scoreCache.insert(blackLine, accumulateScore);

        blackScore += *accumulateScore;
    }

    int whiteScore = 0;

    if (const auto &cacheScore = scoreCache[whiteLine]) {
        whiteScore += *cacheScore;
    } else {
        const auto accumulateScore = new int(0);
        const auto shapes = trie.parse_text(whiteLine);

        for (const auto &shape : shapes) {
            *accumulateScore += shapeScoreTable[shape.get_keyword()];
        }

        scoreCache.insert(whiteLine, accumulateScore);

        whiteScore += *accumulateScore;
    }

    return {blackScore, whiteScore};
}

QPair<int, int> Evaluator::lineOffsetPair(const QPoint &move, const int &direction)
{
    int line;
    int offset;
    const auto &[x, y] = move;

    switch (direction) {
    case 0:
        line = y;
        offset = x - 4;

        break;
    case 1:
        line = x + 15;
        offset = y - 4;

        break;
    case 2:
        line = y - x + 40;
        offset = qMin(x, y) - 4;

        break;
    case 3:
        line = x + y + 47;
        offset = qMin(y, 14 - x) - 4;

        break;
    }

    return {line, offset};
}
