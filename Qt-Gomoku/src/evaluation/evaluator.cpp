#include "evaluator.h"

#include <QtGlobal>
#include <QCache>
#include <QHash>
#include <QList>

#include <algorithm>
#include <functional>
#include <string>

#ifdef emit
#undef emit
#include "../algorithm/aho_corasick.hpp"
#endif

using namespace Evaluation;

namespace {
aho_corasick::trie trie;
QCache<std::string, int> shapeScore = QCache<std::string, int>(1 << 23);
const QHash<std::string, Score> shapeScoreTable = {
    {"00100", One},
    {"01010", Two}, {"001100", Two},
    {"01110", Three}, {"010110", Three}, {"011010", Three},
    {"11110", Four}, {"01111", Four}, {"10111", Four}, {"11011", Four}, {"11101", Four},
    {"011110", OpenFours},
    {"11111", Five}
};
}

Evaluator::Evaluator(std::array<std::string, 72> *blackShapes,
                     std::array<std::string, 72> *whiteShapes) : blackShapes(blackShapes), whiteShapes(whiteShapes),
    blackScores({}), whiteScores({}), blackTotalScore(0), whiteTotalScore(0)
{
    trie.only_whole_words();

    for (auto it = shapeScoreTable.cbegin(); it != shapeScoreTable.cend(); ++it) {
        trie.insert(it.key());
    }
}

void Evaluator::restore()
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

void Evaluator::update(const QPoint &point)
{
    history.blackScores.push(blackScores);
    history.whiteScores.push(whiteScores);
    history.blackTotalScore.push(blackTotalScore);
    history.whiteTotalScore.push(whiteTotalScore);

    std::array<int, 4> blackLineScores{};
    std::array<int, 4> whiteLineScores{};
    const auto x = point.x();
    const auto y = point.y();
    const std::array<bool, 4> valid = {true, true, qAbs(y - x) <= 10, x + y >= 4 && x + y <= 24};
    const std::reference_wrapper<const std::string> blackLines[] = {(*blackShapes)[y], (*blackShapes)[x + 15], (*blackShapes)[y - x + 40], (*blackShapes)[valid[3] ? x + y + 47 : 0]};
    const std::reference_wrapper<const std::string> whiteLines[] = {(*whiteShapes)[y], (*whiteShapes)[x + 15], (*whiteShapes)[y - x + 40], (*whiteShapes)[valid[3] ? x + y + 47 : 0]};

    for (size_t i = 0; i < 4; ++i) {
        if (valid[i]) {
            if (const auto &cacheScore = shapeScore[blackLines[i]]) {
                blackLineScores[i] = *cacheScore;
            } else {
                const auto shapes = trie.parse_text(blackLines[i]);

                for (const auto &shape : shapes) {
                    blackLineScores[i] += shapeScoreTable[shape.get_keyword()];
                }

                shapeScore.insert(blackLines[i], new int(blackLineScores[i]));
            }

            if (const auto &cacheScore = shapeScore[whiteLines[i]]) {
                whiteLineScores[i] = *cacheScore;
            } else {
                const auto shapes = trie.parse_text(whiteLines[i]);

                for (const auto &shape : shapes) {
                    whiteLineScores[i] += shapeScoreTable[shape.get_keyword()];
                }

                shapeScore.insert(whiteLines[i], new int(whiteLineScores[i]));
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

int Evaluator::evaluate(const Stone &stone) const
{
    return stone == Black ? blackTotalScore : whiteTotalScore;
}

QPair<int, int> Evaluator::evaluatePoint(const QPoint &point, const int &direction) const
{
    const auto x = point.x();
    const auto y = point.y();

    if (direction == 2 && qAbs(y - x) > 10) {
        return {0, 0};
    }

    if (direction == 3 && x + y < 4 || x + y > 24) {
        return {0, 0};
    }

    int line;
    int offset;

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

    std::string blackLine = (*blackShapes)[line];
    std::string whiteLine = (*whiteShapes)[line];

    blackLine[offset + 4] = '1';
    whiteLine[offset + 4] = '1';

    int count = offset < 0 ? offset + 9 : 9;

    offset = qMax(0, offset);

    if (offset + count > blackLine.size()) {
        count = blackLine.size() - offset;
    }

    blackLine = blackLine.substr(offset, count);
    whiteLine = whiteLine.substr(offset, count);

    int blackScore = 0;

    if (const auto &cacheScore = shapeScore[blackLine]) {
        blackScore += *cacheScore;
    } else {
        const auto accumulateScore = new int(0);
        const auto shapes = trie.parse_text(blackLine);

        for (const auto &shape : shapes) {
            *accumulateScore += shapeScoreTable[shape.get_keyword()];
        }

        shapeScore.insert(blackLine, accumulateScore);

        blackScore += *accumulateScore;
    }

    int whiteScore = 0;

    if (const auto &cacheScore = shapeScore[whiteLine]) {
        whiteScore += *cacheScore;
    } else {
        const auto accumulateScore = new int(0);
        const auto shapes = trie.parse_text(whiteLine);

        for (const auto &shape : shapes) {
            *accumulateScore += shapeScoreTable[shape.get_keyword()];
        }

        shapeScore.insert(whiteLine, accumulateScore);

        whiteScore += *accumulateScore;
    }

    return {blackScore, whiteScore};
}
