#ifndef MOVESGENERATOR_H
#define MOVESGENERATOR_H

#include <QPair>
#include <QPoint>
#include <QSet>
#include <QStack>
#include <array>

namespace Gomoku {
enum Stone { Black = -1, Empty, White };

class MovesGenerator
{
private:
    QStack<QPair<QSet<QPoint>, QPoint>> history;
    QSet<QPoint> moves;
    std::array<std::array<Stone, 15>, 15> *board;
public:
    MovesGenerator() = delete;
    MovesGenerator(std::array<std::array<Stone, 15>, 15> *board);
    void move(const QPoint &point);
    void undo(const QPoint &point);
    bool empty() const;
    QSet<QPoint> generate() const;
};
}
#endif
