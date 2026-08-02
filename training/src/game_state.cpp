#include "game_state.h"
#include "action_and_undo.h"
#include "evaluator.h"

#include <algorithm>
#include <string>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <array>
#include <cstdint>
#include <random>
#include <utility>  

using namespace std;

using std::mt19937;

extern "C" {
#define _Bool bool
#include "hand_index.h"
#undef _Bool
}
double get_p1_winshare(const array<array<uint8_t, 7>, 2> hands){
    uint8_t r0[7], s0[7];
    uint8_t r1[7], s1[7];

    for (int i = 0; i < 7; ++i) {
        uint8_t c0 = hands[0][i];
        uint8_t c1 = hands[1][i];
        r0[i] = card_rank(c0);  s0[i] = card_suit(c0);
        r1[i] = card_rank(c1);  s1[i] = card_suit(c1);
    }

    uint32_t score0 = evaluate_raw(r0, s0, 7);
    uint32_t score1 = evaluate_raw(r1, s1, 7);

    if (score1 > score0) return 1.0;
    if (score1 < score0) return 0.0;
    return 0.5;
}

static double deal_and_get_winshare(mt19937& rng, array<array<int, 4>, 2>& hand_ids) {
    //deck object created just once
    static array<int, 52> deck = []{
        array<int,52> d{};
        for (int i = 0; i < 52; i++) d[i] = i;
        return d;}();

    static const array<uint8_t, 1> preflop_counts = {2};
    static const array<uint8_t, 2> flop_counts = {2, 3};
    static const array<uint8_t, 2> turn_counts = {2, 4};
    static const array<uint8_t, 2> river_counts = {2, 5};
    
    static Indexer preflop_indexer(preflop_counts.size(), preflop_counts.data());
    static Indexer flop_indexer(flop_counts.size(), flop_counts.data());
    static Indexer turn_indexer(turn_counts.size(), turn_counts.data());
    static Indexer river_indexer(river_counts.size(), river_counts.data());
    static array<array<uint8_t,7>,2> hands;
    
    //shuffle deck
    for (int i = 0; i < 9; ++i) {
        uniform_int_distribution<int> dist(i, 51);
        int j = dist(rng);
        swap(deck[i], deck[j]);
    }

    hands[0][0] = deck[0]; hands[0][1] = deck[1];
    hands[1][0] = deck[2]; hands[1][1] = deck[3];

    size_t count = 2;

    for (int i = 4; i < 9; ++i){
        hands[0][count] = deck[i];
        hands[1][count] = deck[i];
        count += 1;
    }

    for (size_t p = 0; p < 2; ++p){
        hand_ids[p][0] = static_cast<int>(hand_index_last(&preflop_indexer.h, hands[p].data()));
        hand_ids[p][1] = static_cast<int>(hand_index_last(&flop_indexer.h, hands[p].data()));
        hand_ids[p][2] = static_cast<int>(hand_index_last(&turn_indexer.h, hands[p].data()));
        hand_ids[p][3] = static_cast<int>(hand_index_last(&river_indexer.h, hands[p].data()));
    }

    double p1_winshare = get_p1_winshare(hands);
    return p1_winshare;
}


GameState::GameState(): hand_ids{}, stacks{starting_stack, starting_stack}, 
pips{0,0}, p1_win_share(-1), pot(0), street(0), active_player(0), 
last_action{-1,-1}{
    hand_ids[0].fill(-1);
    hand_ids[1].fill(-1);
}

double GameState::get_reward(int player) const {

    if (!is_terminal_node()) throw logic_error("Cannot assign rewards from non-terminal state");
    if (player != 0 && player != 1) throw logic_error("The player index must be one of 1 or 0");

    double win_share = 0.0;

    if (last_action.type == 0){
        if (player == active_player) win_share = 1.0;
    }

    else{
        if (player == 1) win_share = p1_win_share;
        else win_share = 1.0 - p1_win_share;
    }

    double reward = (stacks[player] - starting_stack) + win_share * static_cast<double>(pot);
    return reward;
}


bool GameState::is_legal_action(const Action& action) const {
  
    if (is_terminal_node() || is_chance_node()) return false;
    if (action.type < 0 || action.type > 3) return false;
         
    int to_call = pips[1-active_player] - pips[active_player];
    bool facing_bet = (to_call > 0);

    if (action.type == 0) return facing_bet && action.amt == 0; // fold
    if (action.type == 2) return facing_bet && action.amt == 0 && to_call <= stacks[active_player]; // call
    if (action.type == 1) return !facing_bet && action.amt == 0;// check

    if (action.type != 3) throw logic_error("Shoudl not get here");

    int cur_bet = max(pips[0], pips[1]);
    int min_raise_to = cur_bet + (facing_bet ? max(2, to_call) : 2);
    int max_raise_to = min(pips[0] + stacks[0], pips[1] + stacks[1]);

    return (action.amt >= min_raise_to) && (action.amt <= max_raise_to);
}


void GameState::write_action_undo(const Action& action, ActionUndo& undo) const {
    if (is_chance_node() || is_terminal_node())
        throw logic_error("cant call action on chance or terminal");

    int to_pay = 0;
    if (action.type == 2) { // call
        to_pay = pips[1 - active_player] - pips[active_player];
    } else if (action.type == 3) { 
        to_pay = action.amt - pips[active_player];
    }

    undo.old_to_pay = to_pay;
    undo.old_last_action = last_action;
    undo.old_street = street;
    undo.old_active_player = active_player;
}

void GameState::apply_action(const Action& action) {

    if (is_chance_node() || is_terminal_node()){ throw logic_error("cant call action on chance or terminal");}

    int to_pay = 0;
    if (action.type == 2) to_pay = pips[1 - active_player] - pips[active_player];
    else if (action.type == 3) to_pay = action.amt - pips[active_player];

    pips[active_player] += to_pay;
    pot += to_pay;
    stacks[active_player] += -to_pay;
    
    bool round_ended = false;

    if (last_action.type == 3 && action.type == 2) round_ended = true; //raise then call
    if (last_action.type == 1 && action.type == 1) round_ended = true; //check then check
    if (last_action.type == 2 && action.type == 1) round_ended = true; //limp then check


    last_action = action;
    active_player = 1 - active_player; 

    if (action.type == 0) { 
        street = 8;
    }

    else if (round_ended){
        street += 1;
    }
}

void GameState::undo_action(const ActionUndo& undo) {

    last_action = undo.old_last_action;
    street = undo.old_street; 
    active_player = undo.old_active_player;

    int to_pay = undo.old_to_pay;
    pips[active_player] += -to_pay; 
    pot += -to_pay;
    stacks[active_player] += to_pay;
}

void GameState::write_chance_undo(ChanceUndo& undo) const{
    if (!(is_chance_node())){ throw logic_error("cant call chance on non chance node");}
    
    undo.old_pips = pips;
    undo.old_stacks = stacks;
    undo.old_pot = pot;
    undo.old_active_player = active_player;
    undo.old_street = street;
    undo.old_last_action = last_action;
}

void GameState::apply_chance(mt19937& rng) {

    if (!is_chance_node()){ throw logic_error("Can only apply chance in a chance node");}

    pips = {0, 0};
    active_player = 0;
    last_action = {-1, -1};

    if (street == 0) { 
        p1_win_share = deal_and_get_winshare(rng, hand_ids);
        active_player = 1;
        pot = 3;
        stacks[0] = starting_stack - 2;
        stacks[1] = starting_stack - 1;
        pips[0] = 2;
        pips[1] = 1;
    }

    if (street == 2) { 
        active_player = 0;
    }

    if (street == 4) { 
        active_player = 0;
    }

    if (street == 6) { 
        active_player = 0;
    }

    street += 1;
}

void GameState::undo_chance(const ChanceUndo& undo) {

    if (undo.old_street == 0){
        hand_ids[0].fill(-1);
        hand_ids[1].fill(-1);
        p1_win_share = -1;
    }

    pips = undo.old_pips;
    stacks = undo.old_stacks;
    pot = undo.old_pot;
    active_player = undo.old_active_player;
    street = undo.old_street;
    last_action = undo.old_last_action;
}
