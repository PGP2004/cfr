#include "RockPaperScissors/RPSGameState.h"
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <iostream>

using namespace std;

RPSGameState::RPSGameState(): hands(), active_player(0){};

RPSGameState::RPSGameState(pair<string,string> player_hands,
                           int current_player): 
      hands(std::move(player_hands)),
      active_player(current_player){

    if (active_player != 0 && active_player != 1) {
        throw invalid_argument("active_player must be 0 or 1");
    }
}

vector<Action> RPSGameState::get_action_history() const {
    return vector<Action>{};
}

bool RPSGameState::is_well_formed_action(const Action& action) const {
    const string& t = action.type();
    return (t == "ROCK" || t == "PAPER" || t == "SCISSORS") && action.amount() == 0;
}

bool RPSGameState::is_legal_action(const Action& action) const {


    if (find(action_set.begin(), action_set.end(), action) == action_set.end()){
        return false;
    }
    return true;
};

bool RPSGameState::is_terminal() const {
    if (hands.first.size() > 0 && hands.second.size() > 0){
        return true;
    }

    return false;
};

bool RPSGameState::is_chance_node() const {
    return false;
}

unique_ptr<GameState> RPSGameState::sample_chance_node(mt19937& /*rng*/) const{
    throw logic_error("Should never hit chance node in rock paper scissors");
    return nullptr;
}

unique_ptr<GameState> RPSGameState::next_game_state_impl(const Action& action) const {

    if (is_chance_node() || is_terminal()){
        throw invalid_argument("Cant do an action on a terminal/chance state");
    }

    int new_active_player = 1 - active_player;
    pair<string, string> new_hands = hands;

    if (active_player == 0){
        new_hands.first = action.description();
    }
    else if(active_player == 1){
        new_hands.second = action.description();
    }
    return make_unique<RPSGameState>(new_hands, new_active_player);
}

int RPSGameState::get_active_player() const {
    return active_player;
}

string RPSGameState::get_hand(int player) const {

    if (player == 0){
        return hands.first;
    }

    return hands.second;
}

int RPSGameState::get_pot() const{
    return 0;
}

string RPSGameState::get_board() const {
    return "";
}

double RPSGameState::get_rewards(int player) const{

    if (player != 0 && player != 1) throw logic_error("Cannot get reward for a player not equal to 1");

    if (!is_terminal()) throw logic_error("Cannot get rewards from a non terminals state");
    
    string my_hand;
    string opp_hand;

    if (player == 0){
        my_hand = hands.first;
        opp_hand = hands.second;
    }
    else{
        my_hand = hands.second;
        opp_hand = hands.first;
    }

    //tie case
    if (my_hand == opp_hand){
        return 1.0/2.0;
    }

    //win cases
    else if (my_hand == "SCISSORS" && opp_hand == "PAPER") return 1.0;
    else if (my_hand == "PAPER" && opp_hand == "ROCK") return 1.0;
    else if (my_hand == "ROCK" && opp_hand == "SCISSORS") return 1.0;

    return 0.0;
}


   
   