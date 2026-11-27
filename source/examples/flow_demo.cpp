#include "lo02/game.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

using namespace lo02;

class DemoTurns final : public TurnRules {
public:
    void start(const std::vector<Player>& players) override {
        marks_.assign(players.size(), 0);
        awaitingConfirmation_ = false;
    }

    std::vector<ActionOption> availableActions(PlayerId) const override {
        if (awaitingConfirmation_) {
            return {{"confirm", "Confirm the mark"}};
        }
        return {{"mark", "Mark one point"}};
    }

    ActionResult apply(PlayerId player, std::string_view actionId) override {
        if (!awaitingConfirmation_ && actionId == "mark") {
            awaitingConfirmation_ = true;
            return {ActionStatus::AwaitingAction, "Mark selected."};
        }
        if (awaitingConfirmation_ && actionId == "confirm") {
            ++marks_.at(player);
            awaitingConfirmation_ = false;
            return {ActionStatus::TurnComplete, "Mark confirmed."};
        }
        return {ActionStatus::Rejected, "Choose the available action."};
    }

    int marks(PlayerId player) const {
        return marks_.at(player);
    }

private:
    std::vector<int> marks_;
    bool awaitingConfirmation_ = false;
};

class DemoOutcome final : public OutcomeRules {
public:
    explicit DemoOutcome(
        const DemoTurns& turns, std::optional<std::size_t> turnLimit = std::nullopt)
        : turns_(turns), turnLimit_(turnLimit) {}

    std::optional<GameResult> evaluateAfterTurn(
        const std::vector<Player>& players,
        PlayerId,
        std::size_t completedTurns) override {
        if (completedTurns < turnLimit_.value_or(players.size() * 2)) {
            return std::nullopt;
        }

        GameResult result;
        for (const auto& player : players) {
            result.scores.push_back({player.id, turns_.marks(player.id)});
        }

        const auto highest = std::max_element(
            result.scores.begin(), result.scores.end(),
            [](const PlayerScore& left, const PlayerScore& right) {
                return left.points < right.points;
            });
        for (const auto& score : result.scores) {
            if (score.points == highest->points) {
                result.winners.push_back(score.player);
            }
        }
        return result;
    }

private:
    const DemoTurns& turns_;
    std::optional<std::size_t> turnLimit_;
};

class DemoOutcomeError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class FailingOutcome final : public OutcomeRules {
public:
    FailingOutcome(DemoOutcome& outcome, std::size_t failedTurn)
        : outcome_(outcome), failedTurn_(failedTurn) {}

    std::optional<GameResult> evaluateAfterTurn(
        const std::vector<Player>& players,
        PlayerId lastPlayer,
        std::size_t completedTurns) override {
        if (completedTurns == failedTurn_ && failuresRemaining_ > 0) {
            --failuresRemaining_;
            throw DemoOutcomeError("Simulated outcome evaluation failure.");
        }
        return outcome_.evaluateAfterTurn(players, lastPlayer, completedTurns);
    }

private:
    DemoOutcome& outcome_;
    std::size_t failedTurn_;
    int failuresRemaining_ = 2;
};

void check(bool condition, std::string_view message) {
    if (!condition) {
        throw std::logic_error(std::string(message));
    }
}

void checkAvailableAction(const Game& game, std::string_view expectedId) {
    const auto actions = game.availableActions();
    check(actions.size() == 1 && actions.front().id == expectedId,
          "The available action does not match the turn state.");
}

void checkOutcomeRetry(std::size_t failedTurn) {
    DemoTurns turns;
    DemoOutcome outcome(turns, 3);
    FailingOutcome failingOutcome(outcome, failedTurn);
    Game game({"Alice", "Bob"}, turns, failingOutcome);

    for (std::size_t turn = 1; turn <= 3; ++turn) {
        const PlayerId player = (turn - 1) % 2;
        const int expectedMarks = turns.marks(player) + 1;
        check(game.currentPlayer() == player, "Wrong active player after outcome resolution.");
        check(game.submitAction(player, "mark").status == ActionStatus::AwaitingAction,
              "Mark did not start the turn.");

        if (turn == failedTurn) {
            const auto expectPending = [&](auto attempt) {
                try {
                    attempt();
                } catch (const DemoOutcomeError&) {
                    check(game.outcomePending() && !game.result() && !game.currentPlayer() &&
                              game.availableActions().empty(),
                          "A failed outcome did not pause the match.");
                    check(game.submitAction(player, "mark").status == ActionStatus::Rejected &&
                              game.submitAction(1 - player, "mark").status == ActionStatus::Rejected,
                          "An action was accepted while the outcome was pending.");
                    check(game.completedTurns() == turn && turns.marks(player) == expectedMarks,
                          "The recorded turn changed while the outcome was pending.");
                    return;
                }
                throw std::logic_error("Expected outcome evaluation to fail.");
            };

            expectPending([&] { game.submitAction(player, "confirm"); });
            expectPending([&] { game.resolvePendingOutcome(); });
            game.resolvePendingOutcome();
        } else {
            check(game.submitAction(player, "confirm").status == ActionStatus::TurnComplete,
                  "Confirmation did not complete the turn.");
        }

        check(!game.outcomePending() && game.completedTurns() == turn &&
                  turns.marks(player) == expectedMarks,
              "Outcome resolution altered an already recorded turn.");
        check(game.result().has_value() == (turn == 3), "Wrong time for the final result.");
    }

    check(!game.currentPlayer() && game.availableActions().empty(),
          "The resolved match still presents a turn.");
    check(game.result()->winners == std::vector<PlayerId>{0},
          "Alice should be the only winner.");
    check(turns.marks(0) == 2 && turns.marks(1) == 1,
          "The single-winner scenario did not retain the expected scores.");
}

void runDemo(std::vector<std::string> names) {
    const bool solo = names.size() == 1;
    DemoTurns turns;
    DemoOutcome outcome(turns);
    Game game(names, turns, outcome);
    const auto& players = game.players();
    check(players.size() == names.size() + (solo ? 1 : 0), "Wrong player count.");
    for (PlayerId id = 0; id < names.size(); ++id) {
        check(players[id].id == id && players[id].kind == PlayerKind::Human &&
                  players[id].name == names[id],
              "A human player was not initialized correctly.");
    }
    if (solo) {
        check(players[1].id == 1 && players[1].kind == PlayerKind::Computer,
              "A solo match must include a computer player.");
    }

    std::cout << (solo ? "Solo flow" : "Multiplayer flow") << '\n';
    for (std::size_t turn = 0; turn < players.size() * 2; ++turn) {
        const PlayerId player = turn % players.size();
        const PlayerId nextPlayer = (player + 1) % players.size();
        check(game.currentPlayer() == player, "Wrong active player.");
        check(game.submitAction(nextPlayer, "mark").status == ActionStatus::Rejected,
              "An out-of-turn action was accepted.");
        check(game.submitAction(player, "confirm").status == ActionStatus::Rejected,
              "An out-of-order action was accepted.");
        check(game.currentPlayer() == player && game.completedTurns() == turn,
              "A rejected action advanced the turn.");
        checkAvailableAction(game, "mark");

        check(game.submitAction(player, "mark").status == ActionStatus::AwaitingAction,
              "The first action should require confirmation.");
        check(game.currentPlayer() == player && game.completedTurns() == turn,
              "The turn advanced before confirmation.");
        checkAvailableAction(game, "confirm");

        check(game.submitAction(player, "confirm").status == ActionStatus::TurnComplete,
              "Confirmation did not complete the turn.");
        check(game.completedTurns() == turn + 1, "Wrong completed turn count.");
        std::cout << "Turn " << game.completedTurns() << " - " << players[player].name
                  << " confirmed a mark.\n";
    }

    check(game.result().has_value(), "Missing game result.");
    check(!game.currentPlayer() && game.availableActions().empty(),
          "The finished match still presents a turn.");
    check(game.submitAction(0, "mark").status == ActionStatus::Rejected,
          "An action was accepted after the match ended.");

    const auto& result = *game.result();
    check(result.scores.size() == players.size(), "Missing player scores.");
    check(result.winners.size() == players.size(), "Wrong winner count for this demo tie.");
    for (const auto& player : players) {
        const auto winner = std::find(result.winners.begin(), result.winners.end(), player.id);
        check(winner != result.winners.end(), "Missing winner ID for this demo tie.");
    }
    for (const auto& score : result.scores) {
        check(score.points == 2, "Wrong demo score.");
        std::cout << players.at(score.player).name << ": " << score.points << " points\n";
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        std::vector<std::string> names(argv + 1, argv + argc);
        if (names.empty()) {
            names = {"Alice", "Bob"};
        }
        runDemo(std::move(names));
        checkOutcomeRetry(1);
        checkOutcomeRetry(3);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
