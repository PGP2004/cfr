#pragma once

#include "Action.h"
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>

using std::string;
using std::unique_ptr;
using std::vector;
using std::mt19937;



class GameState {

protected:
    
    virtual unique_ptr<GameState> next_game_state_impl(const Action& action) const = 0; 

public:

    virtual ~GameState() = default; 

    virtual bool is_terminal() const = 0;
    virtual bool is_chance_node() const = 0;

    virtual bool is_legal_action(const Action& action) const = 0;
    virtual bool is_well_formed_action(const Action& action) const = 0;

    unique_ptr<GameState> next_game_state(const Action& action) const {
        if (!is_well_formed_action(action)) {
            throw std::invalid_argument("Not well-formed: " + action.description());
        }
        if (!is_legal_action(action)) {
            throw std::invalid_argument("Illegal action: " + action.description());
        }
        return next_game_state_impl(action);
    }

    virtual unique_ptr<GameState> sample_chance_node(mt19937& rng) const = 0;

    virtual int get_active_player() const = 0;
    virtual string get_hand(int player) const = 0;
    virtual double get_rewards(int cur_player) const = 0;
    virtual vector<Action> get_action_history() const = 0;

};
