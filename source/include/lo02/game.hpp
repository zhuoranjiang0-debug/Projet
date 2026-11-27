#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lo02 {

using PlayerId = std::size_t;

enum class PlayerKind { Human, Computer };

struct Player {
    PlayerId id;
    std::string name;
    PlayerKind kind;
};

struct ActionOption {
    std::string id;
    std::string label;
};

enum class ActionStatus { Rejected, AwaitingAction, TurnComplete };

struct ActionResult {
    ActionStatus status;
    std::string message;
};

struct PlayerScore {
    PlayerId player;
    int points;
};

struct GameResult {
    std::vector<PlayerScore> scores;
    std::vector<PlayerId> winners;
};

class TurnRules {
public:
    virtual ~TurnRules() = default;
    virtual void start(const std::vector<Player>& players) = 0;
    virtual std::vector<ActionOption> availableActions(PlayerId player) const = 0;

    // Rejected actions must leave the match state unchanged.
    virtual ActionResult apply(PlayerId player, std::string_view actionId) = 0;
};

class OutcomeRules {
public:
    virtual ~OutcomeRules() = default;

    // Return no result while the match should continue.
    virtual std::optional<GameResult> evaluateAfterTurn(
        const std::vector<Player>& players,
        PlayerId lastPlayer,
        std::size_t completedTurns) = 0;
};

class Game {
public:
    // Both rule objects must outlive this match.
    Game(
        std::vector<std::string> humanNames,
        TurnRules& turnRules,
        OutcomeRules& outcomeRules);

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    const std::vector<Player>& players() const noexcept;
    std::optional<PlayerId> currentPlayer() const noexcept;
    std::vector<ActionOption> availableActions() const;
    std::size_t completedTurns() const noexcept;
    const std::optional<GameResult>& result() const noexcept;

    ActionResult submitAction(PlayerId player, std::string_view actionId);

private:
    std::vector<Player> players_;
    TurnRules& turnRules_;
    OutcomeRules& outcomeRules_;
    PlayerId currentPlayer_ = 0;
    std::size_t completedTurns_ = 0;
    std::optional<GameResult> result_;
};

}  // namespace lo02
