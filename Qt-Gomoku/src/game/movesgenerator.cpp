#include "../search/engine.h"
#include "movesgenerator.h"

#include <numeric>

using namespace Game;

MovesGenerator::MovesGenerator(Evaluation::Evaluator *evaluator,
                               std::array<std::array<Stone, 15>, 15> *board) : evaluator(evaluator), board(board) {}

void MovesGenerator::move(const QPoint &point)
{
    const int r = moves.empty() ? 1 : 3;

    history.push(moves);

    for (int i = -r; i <= r; ++i) {
        for (int j = -r; j <= r; ++j) {
            if (const auto neighborhood = point + QPoint(i, j); Search::Engine::isLegal(neighborhood)
                    && (*board)[neighborhood.x()][neighborhood.y()] == Empty) {
                if (!moves.contains(neighborhood)) {
                    moves.insert(neighborhood, {{0, 0, 0, 0}, {0, 0, 0, 0}});
                }
            }
        }
    }

    constexpr std::array<int, 2> d = {-1, 1};
    constexpr std::array<int, 4> dx = {1, 0, 1, 1};
    constexpr std::array<int, 4> dy = {0, 1, 1, -1};

    for (size_t i = 0; i < 2; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 1; k <= 4; ++k) {
                if (const auto neighborhood = point + QPoint(d[i] * dx[j] * k, d[i] * dy[j] * k);
                        Search::Engine::isLegal(neighborhood) && (*board)[neighborhood.x()][neighborhood.y()] == Empty) {
                    if (moves.contains(neighborhood)) {
                        const auto [blackScore, whiteScore] = evaluator->evaluatePoint(neighborhood, j);

                        moves[neighborhood].first[j] = blackScore;
                        moves[neighborhood].second[j] = whiteScore;
                    }
                }
            }
        }
    }

    moves.remove(point);
}

void MovesGenerator::undo(const QPoint &point)
{
    moves = history.top();

    history.pop();
}

bool MovesGenerator::empty() const
{
    return moves.empty();
}

QHash<QPoint, QPair<int, int>> MovesGenerator::generate() const
{
    QHash<QPoint, QPair<int, int>> m;

    m.reserve(moves.size());

    for (auto it = moves.cbegin(); it != moves.cend(); ++it) {
        m.insert(it.key(), {std::reduce(it.value().first.cbegin(), it.value().first.cend()), std::reduce(it.value().second.cbegin(), it.value().second.cend())});
    }

    return m;
}
