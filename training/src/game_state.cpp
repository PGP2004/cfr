#include "game_state.h"
#include "action.h"
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


static void deal_hands(std::mt19937& rng, std::array<std::array<uint8_t, 7>, 2>& hands) {
    //deck object created just once
    static std::array<int, 52> deck = []{
        std::array<int,52> d{};
        for (int i = 0; i < 52; i++) d[i] = i;
        return d;}();

    for (int i = 0; i < 9; ++i) {
        std::uniform_int_distribution<int> dist(i, 51);
        int j = dist(rng);
        std::swap(deck[i], deck[j]);
    }

    hands[0][0] = deck[0]; hands[0][1] = deck[1];
    hands[1][0] = deck[2]; hands[1][1] = deck[3];

    size_t count = 2;

    for (int i = 4; i < 9; ++i){
        hands[0][count] = deck[i];
        hands[1][count] = deck[i];
        count += 1;
    }
}

GameState::GameState(){
    street = 0;
    active_player = 0;

    stacks.fill(starting_stack);
    pips.fill(0);
    pot = 0;

    last_action = {-1,-1};

    hands[0].fill(-1);
    hands[1].fill(-1);
}

bool GameState::is_legal_action(const Action& action) const {
  
    if (is_terminal_node() || is_chance_node()) return false;
    if (action.type < 0 || action.type > 3) return false;
         
    int to_call = pips[1-active_player] - pips[active_player];
    bool facing_bet = (to_call > 0);

    if (action.type == 0) return facing_bet && action.amt == 0; // fold
    if (action.type == 2) return facing_bet && action.amt == 0 && to_call <= stacks[active_player]; // call
    if (action.type == 1) return !facing_bet && action.amt == 0;// check

    if (action.type != 3) throw std::logic_error("Shoudl not get here");

    int cur_bet = std::max(pips[0], pips[1]);
    int min_raise_to = cur_bet + (facing_bet ? std::max(2, to_call) : 2);
    int max_raise_to = std::min(pips[0] + stacks[0], pips[1] + stacks[1]);

    return (action.amt >= min_raise_to) && (action.amt <= max_raise_to);
}

GameState GameState::apply_action(const Action& action) {

    if (is_chance_node() || is_terminal_node()){
        throw std::logic_error("cant call action on chance or terminal");
    }
    GameState next = *this;

    int to_pay = 0;
    if (action.type == 2) to_pay = pips[1 - active_player] - pips[active_player];
    else if (action.type == 3) to_pay = action.amt - pips[active_player];

    next.pips[active_player] += to_pay;
    next.pot += to_pay;
    next.stacks[active_player] += -to_pay;
    
    bool round_ended = false;

    if (last_action.type == 3 && action.type == 2) round_ended = true; //raise then call
    if (last_action.type == 1 && action.type == 1) round_ended = true; //check then check
    if (last_action.type == 2 && action.type == 1) round_ended = true; //limp then check

    next.last_action = action;
    next.active_player = 1 - active_player; 

    if (action.type == 0) next.street = 8;
    else if (round_ended) next.street += 1;
    return next;
}

GameState GameState::apply_chance(std::mt19937& rng) {

    GameState next = *this;

    if (!is_chance_node()){ throw std::logic_error("Can only apply chance in a chance node");}

    next.pips = {0, 0};
    next.active_player = 0;
    next.last_action = {-1, -1};

    if (street == 0) { 
        deal_hands(rng, next.hands);
        next.active_player = 1;
        next.pot = 3;
        next.stacks[0] = starting_stack - 2;
        next.stacks[1] = starting_stack - 1;
        next.pips[0] = 2;
        next.pips[1] = 1;
    }

    if (street == 2) next.active_player = 0;
    if (street == 4) next.active_player = 0;
    if (street == 6) next.active_player = 0;

    next.street += 1;
    return next;
}


