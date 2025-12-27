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

using std::pair;
using std::string;
using std::vector;
using std::unordered_map;
using std::unique_ptr;
using std::mt19937;



class KuhnGameState : public GameState {
private:

    pair<int,int> stacks;
    int pot;
    Deck deck{ vector<string>{"A", "K", "Q"} };
    pair<string, string> hands;
    bool cards_dealt;
    int active_player;
    vector<Action> action_history;
    vector<Action> action_set;
    static const unordered_map<string, int> card_to_rank;
    vector<Action> make_action_set() const;

public:

    KuhnGameState();

    KuhnGameState(pair<int,int> player_stacks, int current_pot,
                  pair<string,string> player_hands, Deck game_deck,
                  int current_player, vector<Action> history);

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

  
};


