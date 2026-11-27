#include "lo02/game.hpp"

#include <stdexcept>
#include <utility>

namespace lo02 {

Game::Game(
    std::vector<std::string> humanNames,
    TurnRules& turnRules,
    OutcomeRules& outcomeRules)
    : turnRules_(turnRules), outcomeRules_(outcomeRules) {
    if (humanNames.empty() || humanNames.size() > 4) {
        throw std::invalid_argument("Choose between one and four human players.");
    }

    for (auto& name : humanNames) {
        if (name.find_first_not_of(" \t\r\n\f\v") == std::string::npos) {
            throw std::invalid_argument("Player names must not be blank.");
        }
        players_.push_back({players_.size(), std::move(name), PlayerKind::Human});
    }

    if (players_.size() == 1) {
        players_.push_back({players_.size(), "Computer", PlayerKind::Computer});
    }

    turnRules_.start(players_);
}

const std::vector<Player>& Game::players() const noexcept {
    return players_;
}

std::optional<PlayerId> Game::currentPlayer() const noexcept {
    if (result_) {
        return std::nullopt;
    }
    return currentPlayer_;
}

std::vector<ActionOption> Game::availableActions() const {
    if (result_) {
        return {};
    }
    return turnRules_.availableActions(currentPlayer_);
}

std::size_t Game::completedTurns() const noexcept {
    return completedTurns_;
}

const std::optional<GameResult>& Game::result() const noexcept {
    return result_;
}

ActionResult Game::submitAction(PlayerId player, std::string_view actionId) {
    if (result_) {
        return {ActionStatus::Rejected, "The match has already ended."};
    }
    if (player != currentPlayer_) {
        return {ActionStatus::Rejected, "It is not that player's turn."};
    }

    ActionResult action = turnRules_.apply(player, actionId);
    if (action.status == ActionStatus::TurnComplete) {
        ++completedTurns_;
        result_ = outcomeRules_.evaluateAfterTurn(players_, player, completedTurns_);
        if (!result_) {
            currentPlayer_ = (currentPlayer_ + 1) % players_.size();
        }
    }
    return action;
}

}  // namespace lo02
