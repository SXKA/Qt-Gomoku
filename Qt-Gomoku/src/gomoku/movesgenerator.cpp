#include "movesgenerator.h"

#include "engine.h"

using namespace Gomoku;

MovesGenerator::MovesGenerator(std::array<std::array<Stone, 15>, 15> *board) : board(board) {}

void MovesGenerator::move(const QPoint &point)
{
    auto hist = QSet<QPoint>();
    constexpr std::array<int, 2> d = {-1, 1};
    constexpr std::array<int, 4> dx = {1, 0, 1, 1};
    constexpr std::array<int, 4> dy = {0, 1, 1, -1};

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 4; ++j) {
            auto neighborhood = point + QPoint(d[i] * dx[j], d[i] * dy[j]);

            if (Engine::isLegal(neighborhood) && (*board)[neighborhood.x()][neighborhood.y()] == Stone::Empty) {
                const auto size = moves.count();

                moves.insert(neighborhood);

                if (moves.size() > size) {
                    hist.insert(neighborhood);
                }
            }
        }
    }

    auto removedPoint = QPoint();

    if (moves.remove(point)) {
        removedPoint = point;
    }

    history.emplaceBack(hist, removedPoint);
}

void Gomoku::MovesGenerator::undo(const QPoint &point)
{
    const auto &hist = history.top();

    for (const auto &p : hist.first) {
        moves.remove(p);
    }

    if (!hist.second.isNull()) {
        moves.insert(hist.second);
    }

    history.pop();
}

bool Gomoku::MovesGenerator::empty() const
{
    return moves.empty();
}

QSet<QPoint> Gomoku::MovesGenerator::generate() const
{
    return moves;
}
