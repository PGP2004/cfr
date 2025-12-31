#pragma once
#include "CFR/GameState.h"
#include "Deck.h"
#include "CFR/Action.h"

#include <utility>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <random>
#include <array>

using std::pair;
using std::string;
using std::vector;
using std::unordered_map;
using std::unique_ptr;
using std::mt19937;
using std::array;

//this implementation is quite readable but dog in terms of like effieciency
//should make a faster one at some point


class HoldEmGameState : public GameState {
private:

    const static int starting_stack = 400; 
    const static unordered_map<string, int> card_id;

    array<int, 2> stacks;
    array<int, 2> pips;
    int pot;

    Deck deck{ vector<string>{"A", "K", "Q"} }; // fix this

    array<array<string,2>,2> hands;
    vector<string> board;

    int street;
    int active_player;

    vector<Action> action_history;
    vector<Action> street_history;

public:

    HoldEmGameState();

    HoldEmGameState(array<int, 2> player_stacks,
               array<int, 2> current_pips,
               int current_pot,
               array<array<string,2>,2> player_hands,
               vector<string> cur_board,
               Deck game_deck,
               int current_player,
               int current_street,
               vector<Action> current_history,
               vector<Action> current_street_history);

    vector<Action> get_action_set() const;

    bool is_well_formed_action(const Action& action) const override;
    bool is_legal_action(const Action& action) const override;
    bool is_terminal() const override;
    bool is_chance_node() const override;

    unique_ptr<GameState> sample_chance_node(mt19937& rng) const override;
    unique_ptr<GameState> next_game_state_impl(const Action& action) const override;
    
    double get_rewards(int cur_player) const override;
    int get_active_player() const override;
    string get_hand(int player) const override;

    vector<Action> get_action_history() const  override;
    array<int, 2> get_raise_bounds() const;
    int get_pot() const override;
    string get_board() const override;
};