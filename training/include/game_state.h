#pragma once
#include "action_and_undo.h"
#include "indexer.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using std::array;
using std::string;
using std::vector;
using std::pair;
using std::uint64_t;
using std::unique_ptr;
using std::mt19937;
using std::logic_error;


extern "C" {
#define _Bool bool
#include "hand_index.h"
#undef _Bool
}

class GameState {

private:

    static constexpr int starting_stack = 200;

    array<array<int, 4>, 2> hand_ids;
    array<int, 2> stacks;
    array<int, 2> pips;
    
    double p1_win_share;

    int pot;
    int street;
    int active_player;
    Action last_action;

public:

    GameState();
    GameState(const GameState&) = default;
    GameState& operator=(const GameState&) = default;

    double get_reward(int player) const;
    bool is_legal_action(const Action& action) const;

    void write_action_undo(const Action& action, ActionUndo& undo) const;
    void apply_action(const Action& action);
    void undo_action(const ActionUndo& undo_info);

    void write_chance_undo(ChanceUndo& undo) const;
    void apply_chance(mt19937& rng);
    void undo_chance(const ChanceUndo& undo);

    //below here is boilerplate

    inline size_t get_street() const { return street/2; }
    inline bool is_terminal_node() const { return street == 8; }
    inline bool is_chance_node() const { return (street%2 == 0) && street != 8; }

    inline int get_active_player() const { return active_player; }
    inline int get_pot() const { return pot; }

    inline int get_hand_id() const {return hand_ids[active_player][street/2]; }

    inline int get_pip(int player) const {
        if (player != 0 && player != 1) throw logic_error("The player index must be one of 1 or 0");
        return pips[player];
    }

};
