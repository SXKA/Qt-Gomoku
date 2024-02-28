#ifndef MOVESGENERATOR_H
#define MOVESGENERATOR_H

#include "../core/types.h"

#include <QHash>
#include <QPair>
#include <QPoint>
#include <QStack>

#include <array>

namespace Evaluation {
class Evaluator;
};

namespace Game {
class MovesGenerator
{
private:
    Evaluation::Evaluator *evaluator;
    QStack<QHash<QPoint, QPair<std::array<int, 4>, std::array<int, 4>>>> history;
    QHash<QPoint, QPair<std::array<int, 4>, std::array<int, 4>>> moves;
    std::array<std::array<Stone, 15>, 15> *board;
public:
    MovesGenerator() = delete;
    MovesGenerator(Evaluation::Evaluator *evaluator, std::array<std::array<Stone, 15>, 15> *board);
    void move(const QPoint &point);
    void undo(const QPoint &point);
    [[nodiscard]] bool empty() const;
    [[nodiscard]] QHash<QPoint, QPair<int, int>> generate() const;
};
}
#endif
