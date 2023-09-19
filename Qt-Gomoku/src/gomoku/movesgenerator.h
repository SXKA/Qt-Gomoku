#ifndef MOVESGENERATOR_H
#define MOVESGENERATOR_H

#include <array>
#include <QPair>
#include <QPoint>
#include <QSet>
#include <QStack>

namespace Gobang {
enum Stone { empty = -1, black, white };

class MovesGenerator
{
public:
    MovesGenerator() = delete;
    MovesGenerator(std::array<std::array<Stone, 15>, 15> *board);
    void move(const QPoint &point);
    void undo(const QPoint &point);
    bool empty() const;
    QSet<QPoint> generate() const;
private:
    QStack<QPair<QSet<QPoint>, QPoint>> history;
    QSet<QPoint> moves;
    std::array<std::array<Stone, 15>, 15> *board;
};
}
#endif
