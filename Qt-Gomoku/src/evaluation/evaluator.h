#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "../core/types.h"

#include <QPoint>
#include <QStack>

#include <array>
#include <string>

namespace Evaluation {
struct History
{
    QStack<std::array<int, 72>> blackScores;
    QStack<std::array<int, 72>> whiteScores;
    QStack<int> blackTotalScores;
    QStack<int> whiteTotalScores;
};

class Evaluator
{
private:
    History history;
    std::array<std::string, 72> *blackShapes;
    std::array<std::string, 72> *whiteShapes;
    std::array<int, 72> blackScores;
    std::array<int, 72> whiteScores;
    int blackTotalScore;
    int whiteTotalScore;

public:
    Evaluator() = delete;
    Evaluator(std::array<std::string, 72> *blackShapes, std::array<std::string, 72> *whiteShapes);
    void restore();
    void update(const QPoint &move);
    [[nodiscard]] bool isFourMove(const QPoint &move, const Stone &stone) const;
    [[nodiscard]] int evaluate(const Stone &stone) const;
    [[nodiscard]] QPair<int, int> evaluateMove(const QPoint &move, const int &direction) const;
    static QPair<int, int> lineOffsetPair(const QPoint &move, const int &direction);
};
} // namespace Evaluation
#endif
