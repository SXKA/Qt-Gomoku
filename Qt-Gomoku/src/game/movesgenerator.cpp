#include "movesgenerator.h"
#include "../search/engine.h"

#include <numeric>

using namespace Game;

MovesGenerator::MovesGenerator(Evaluation::Evaluator *evaluator,
                               std::array<std::array<Stone, 15>, 15> *board)
    : evaluator(evaluator)
    , board(board)
{}

void MovesGenerator::move(const QPoint &point)
{
    history.push(moves);

    for (int i = -3; i <= 3; ++i) {
        for (int j = -3; j <= 3; ++j) {
            if (const auto neighborhood = point + QPoint(i, j);
                Search::Engine::isLegal(neighborhood)
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
                if (const auto neighborhood = point + k * d[i] * QPoint{dx[j], dy[j]};
                    moves.contains(neighborhood)) {
                    auto &moveBlackScore = moves[neighborhood].first[j];
                    auto &moveWhiteScore = moves[neighborhood].second[j];
                    const auto [blackScore, whiteScore] = evaluator->evaluateMove(neighborhood, j);

                    moveBlackScore = blackScore;
                    moveWhiteScore = whiteScore;
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
        const auto [blackScores, whiteScores] = it.value();

        m.insert(it.key(),
                 {std::reduce(blackScores.cbegin(), blackScores.cend()),
                  std::reduce(whiteScores.cbegin(), whiteScores.cend())});
    }

    return m;
}
