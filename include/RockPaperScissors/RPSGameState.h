#pragma once
#include "CFR/GameState.h"
#include "CFR/Action.h"

#include <utility>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <random>

using std::pair;
using std::string;
using std::vector;
using std::unordered_map;
using std::unique_ptr;
using std::mt19937;

class RPSGameState : public GameState {
    
private:

    pair<string, string> hands;
    int active_player;
    vector<Action> action_set = {Action("ROCK"), Action("PAPER"), Action("SCISSORS")};

public:

    RPSGameState();

    RPSGameState(pair<string,string> player_hands, 
                int current_player);

    bool is_legal_action(const Action& action) const override; 
    bool is_well_formed_action(const Action& action) const override;
    bool is_terminal() const override;
    bool is_chance_node() const override;

    unique_ptr<GameState> sample_chance_node(mt19937& rng) const override;
    unique_ptr<GameState> next_game_state_impl(const Action& action) const override;

    double get_rewards(int cur_player) const override;
    int get_active_player() const override;
    string get_hand(int player) const override;
    vector<Action> get_action_history() const  override;
    int get_pot() const override;
    string get_board() const override;

};
