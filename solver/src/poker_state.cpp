#include "poker_state.h"
#include "action.h"
#include "evaluator.h"
#include "dealer.h"

#include <algorithm>
#include <string>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <array>
#include <cstdint>
#include <random>
#include <utility>  

PokerState::PokerState(int stack, int bb, int sb):
    starting_stack(stack), big_blind(bb), small_blind(sb),
    dealer(Dealer{}){
    stage = 0;
    active_player = 0;

    stacks.fill(starting_stack);
    pips.fill(0);
    pot = 0;

    last_action = {-1,-1};
}

 std::vector<uint8_t> PokerState::get_cards(int player) const{
    int street = get_street();
    std::vector<int> board_cards_per_st = {0, 3, 4, 5, 5};
    int n = board_cards_per_st[street];

    std::vector<uint8_t> output;

    for (int i = 0; i < n; ++i){
        output.push_back(dealer.cards[player][i]);
    }
    return output;
}

 double PokerState::get_reward(int player) const{

    if (!is_terminal()){
        throw std::runtime_error("cannot get reward for non-terminal node");
    }

    int opp = 1 - active_player;

    //if someone folded in the game
    if (player_folded()){
        bool won = (player == active_player);
        if (won) return get_payoff(player);
        return -get_payoff(opp);
    }

            // if no one folded in the game.
    if (dealer.winner == -1) return 0.0;
    else if (dealer.winner == player) return get_payoff(player);
    else if (dealer.winner == opp) return - get_payoff(opp);

    throw std::runtime_error("Should not be able to get here");
    return 0.0;
}


std::pair<int,int> PokerState::get_raise_bounds() const{
    int to_call = pips[1-active_player] - pips[active_player];
    bool facing_bet = (to_call > 0);

    int cur_bet = std::max(pips[0], pips[1]);
    int min_raise_to = cur_bet + (facing_bet ? std::max(big_blind, to_call) : big_blind);
    int max_raise_to = std::min(pips[0] + stacks[0], pips[1] + stacks[1]);
    return {min_raise_to, max_raise_to};
}


bool PokerState::is_legal_action(const Action& action) const {
  
    if (is_terminal() || is_chance()) return false;
    if (action.type < 0 || action.type > 3) return false;
         
    int to_call = pips[1-active_player] - pips[active_player];
    bool facing_bet = (to_call > 0);

    if (action.type == 0) return facing_bet && action.amt == 0; // fold
    if (action.type == 2) return facing_bet && action.amt == 0 && to_call <= stacks[active_player]; // call
    if (action.type == 1) return !facing_bet && action.amt == 0;// check

    if (action.type != 3) throw std::logic_error("Shoudl not get here");
    auto [min_raise, max_raise] = get_raise_bounds();

    return (action.amt >= min_raise) && (action.amt <= max_raise);
}

PokerState PokerState::apply_action(const Action& action) {

    if (is_chance() || is_terminal()){
        throw std::logic_error("cant call action on chance or terminal");
    }
    PokerState next = *this;

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

    if (action.type == 0) next.stage = 8;
    else if (round_ended) next.stage += 1;
    return next;
}

PokerState PokerState::apply_chance(std::mt19937 rng) {

    PokerState next = *this;

    if (!is_chance()){ throw std::logic_error("Can only apply chance in a chance node");}

    next.pips = {0, 0};
    next.active_player = 0;
    next.last_action = {-1, -1};

    if (stage == 0) { 
        dealer.deal(rng);
        next.active_player = 1;
        next.pot = small_blind + big_blind;
        next.stacks[0] = starting_stack - big_blind;
        next.stacks[1] = starting_stack - small_blind;
        next.pips[0] = big_blind;
        next.pips[1] = small_blind;
    }

    if (stage == 2) next.active_player = 0;
    if (stage == 4) next.active_player = 0;
    if (stage == 6) next.active_player = 0;

    next.stage += 1;
    return next;
}


