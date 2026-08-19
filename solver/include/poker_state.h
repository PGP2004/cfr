#pragma once
#include "action.h"
#include "indexer.h"
#include "dealer.h"

#include <array>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

class PokerState {

public:

    int starting_stack;
    int big_blind;
    int small_blind;
    std::array<int, 2> stacks;
    std::array<int, 2> pips;
    Dealer dealer;

    int pot;
    int stage;
    int active_player;
    Action last_action;

    PokerState(int stack, int bb, int sb);

    PokerState apply_action(const Action& action);
    PokerState apply_chance(std::mt19937 rng);

    bool is_legal_action(const Action& action) const;

    inline int get_street() const { return stage/2; }

    inline bool is_terminal() const { return stage == 8; }

    inline bool is_chance() const { return (stage%2 == 0) && stage != 8; }

    inline bool player_folded() const {
        Action fold_action = {0,0};
        return (last_action == fold_action);
    }

    inline const Dealer& get_dealer() const {return dealer;}

    std::vector<uint8_t> get_cards(int player) const;
    double get_reward(int player) const;

    std::pair<int,int> get_raise_bounds() const;


    //payoff for the player if the game ended right now
    inline double get_payoff(int player) const {
        double output = (stacks[player] - starting_stack) + static_cast<double>(pot);
        return output;
    };

};
