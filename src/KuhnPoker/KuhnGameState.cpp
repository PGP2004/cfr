#include "KuhnPoker/KuhnGameState.h"
#include "Deck.h"
#include <algorithm>
#include <stdexcept>
#include <memory>


//todo: can consolidate some stuff between kuhnGameState and RPS gamestate
using namespace std;

const unordered_map<string, int> KuhnGameState::card_to_rank = {{"A", 3}, {"K", 2}, {"Q", 1}};

KuhnGameState::KuhnGameState()
  : stacks{1,1},
    pot(2),
    hands({"", ""}),
    cards_dealt(false),
    active_player(0),
    action_history({})
{
   
    action_set = make_action_set();
}

KuhnGameState::KuhnGameState(pair<int,int> player_stacks, int current_pot,
                             pair<string, string> player_hands, Deck game_deck,
                             int current_player, vector<Action> current_history)
  : stacks(player_stacks),
    pot(current_pot),
    deck(std::move(game_deck)),
    hands(std::move(player_hands)),
    cards_dealt(true),                
    active_player(current_player),
    action_history(std::move(current_history))
{
    action_set = make_action_set();
}


vector<Action> KuhnGameState::make_action_set() const {

    if (is_chance_node() || is_terminal()) return {};

    vector<Action> output;
    int n = (int)action_history.size();

    if (n == 0) {
        output.emplace_back(Action("RAISE", 1));
        output.emplace_back(Action("CHECK"));
    } else if (action_history.back().type() == "RAISE") {
        output.emplace_back(Action("CALL", 1));
        output.emplace_back(Action("FOLD"));
    }

    return output;
}

vector<Action> KuhnGameState::get_action_set() const {
    return action_set;
}

bool KuhnGameState::is_well_formed_action(const Action& action) const {
    const string& t = action.type();
    int amt = action.amount();

    if (t == "CHECK" || t == "FOLD") return amt == 0;
    if (t == "CALL"  || t == "RAISE") return amt > 0;
    return false;
}

bool KuhnGameState::is_legal_action(const Action& action) const {
    return find(action_set.begin(), action_set.end(), action) != action_set.end();
}

bool KuhnGameState::is_terminal() const {

    int n = (int)action_history.size();
    if (n == 0) return false;

    if (action_history.back().type() == "FOLD" || action_history.back().type() == "CHECK") return true;

    if (n >= 2) {
        string a = action_history[n-2].type();
        string b = action_history[n-1].type();
        if (a == "CHECK" && b == "CHECK") return true;
        if (a == "RAISE" && b == "CALL") return true;
    }
    return false;
}


unique_ptr<GameState> KuhnGameState::next_game_state_impl(const Action& action) const {

    if (is_terminal()) {
        throw logic_error("Cannot get next info set from terminal state");
    }

    if (is_chance_node()) {
        throw logic_error("Cannot make an action at a chance node");
    }

    pair<string, string> new_hands = hands;
    int new_active_player = 1 - active_player;
    Deck new_deck = deck;

    pair<int,int> new_stacks = stacks;
    int new_pot = pot;

    if (active_player == 0) {
        new_stacks.first -= action.amount();
    }
    else {
        new_stacks.second -= action.amount();
    }

    new_pot += action.amount();

    vector<Action> new_history = action_history;
    new_history.push_back(action);

    return make_unique<KuhnGameState>(new_stacks, new_pot, new_hands, new_deck, new_active_player, new_history);
}

//TODO: Somethings wrong i need to run at some point and figure it out
unique_ptr<GameState> KuhnGameState::sample_chance_node(mt19937& rng) const {
    if (!is_chance_node()) {
        throw logic_error("Cannot sample from non-chance node");
    }

    Deck new_deck = deck;
    new_deck.shuffle_remaining(rng);
    
    string p0_card = new_deck.deal();
    string p1_card = new_deck.deal();
    
    pair<string, string> dealt_cards = {p0_card, p1_card};
    
    unique_ptr<GameState> next_state = make_unique<KuhnGameState>(
        stacks,
        pot,
        dealt_cards,
        new_deck,
        0,  
        action_history
    );
    
    return next_state;
}


double KuhnGameState::get_rewards(int player) const {

    if (!is_terminal()){
         throw logic_error("Cannot assign rewards from non-terminal state");
    }
    if (player != 0 && player != 1){
        throw logic_error("player must be 0 or 1");
    }

    double win_share = 0.0;

    bool is_fold = (!action_history.empty() && action_history.back().type() == "FOLD");

    if (is_fold) {

        if (player == active_player) win_share = 1.0;
        else win_share = 0.0;
    }

    else {
        string my_card;
        string opp_card;

        if (player == 0) {
            my_card = hands.first;
            opp_card = hands.second;
        } else {
            my_card = hands.second;
            opp_card = hands.first;
        }

        int my_rank = card_to_rank.at(my_card);
        int opp_rank = card_to_rank.at(opp_card);

        if (my_rank > opp_rank) win_share = 1.0;
        else if (my_rank == opp_rank) win_share = 0.5;
        else win_share = 0.0;
    }

    double chips_at_start = (pot + stacks.first + stacks.second) / 2.0;
    double my_stack;
    
    if (player == 0) my_stack = static_cast<double>(stacks.first);
    else my_stack = static_cast<double>(stacks.second);

    return my_stack + win_share * static_cast<double>(pot) - chips_at_start;
}


int KuhnGameState::get_active_player() const {
    return active_player;
}

vector<Action> KuhnGameState::get_action_history() const {
    return action_history;
}

bool KuhnGameState::is_chance_node() const {
    return !cards_dealt;
}

int KuhnGameState::get_pot() const{
    return pot;
}

string KuhnGameState::get_board() const {
    return "";
}

string KuhnGameState::get_hand(int player) const {

    if (player != 0 && player!= 1){
        throw logic_error("The player index must be one of 1 or 0");
    }

    if (player == 0){
        return hands.first;
    }

    return hands.second;
}
