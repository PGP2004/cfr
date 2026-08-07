#pragma once
#include "action.h"
#include "indexer.h"

#include <array>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

class GameState {

private:

    static constexpr int starting_stack = 200;
    std::array<std::array<uint8_t, 7>, 2> hands;
    std::array<int, 2> stacks;
    std::array<int, 2> pips;

    int pot;
    int street;
    int active_player;
    Action last_action;

public:

    GameState();

    GameState(const GameState&) = default;

    GameState& operator=(const GameState&) = default;

    GameState  apply_action(const Action& action);

    GameState apply_chance(std::mt19937& rng);

    double get_reward(int player) const;

    bool is_legal_action(const Action& action) const;

    //this is jank, should fix!
    inline int get_street() const { return street/2; }

    inline bool is_terminal_node() const { return street == 8; }

    inline bool is_chance_node() const { return (street%2 == 0) && street != 8; }

    inline int get_active_player() const { return active_player; }
    
    inline int get_pot() const { return pot; }

    inline int get_pip(int player) const {
        if (player != 0 && player != 1) throw std::logic_error("The player index must be one of 1 or 0");
        return pips[player];
    }

    inline bool player_folded() const {
        Action fold_action = {0,0};
        return (last_action == fold_action);
    }

    //payoff for the player if the game ended right now
    inline double get_payoff(int player) const {
        double output = (stacks[player] - starting_stack) + static_cast<double>(pot);
        return output;
    };

};
