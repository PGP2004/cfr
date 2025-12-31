#include "TexasHoldEm/HoldEmGameState.h"
#include "TexasHoldEm/HandEval.h"
#include "Deck.h"
#include <algorithm>
#include <stdexcept>
#include <memory>

using namespace std;

//Reminder abt streets:
// 0: Deal hole cards   
// 1: Preflop betting   
// 2: Deal flop         
// 3: Flop betting      
// 4: Deal turn         
// 5: Turn betting      
// 6: Deal river        
// 7: River betting  
//need to implement


static vector<string> make_standard_deck() {
    constexpr std::string_view ranks = "23456789TJQKA";
    constexpr std::string_view suits = "cdhs";

    std::vector<std::string> cards;
    cards.reserve(52);

    for (char r : ranks) {
        for (char s : suits) {
            cards.push_back(std::string{r, s}); // "Ac"
        }
    }
    return cards;
}

HoldEmGameState::HoldEmGameState()
  : stacks{starting_stack, starting_stack},
    pips({0,0}),
    pot(2),
    deck(make_standard_deck()),
    hands{{ {"", ""}, {"", ""} }},
    street(0),
    active_player(0),
    action_history(){}

HoldEmGameState::HoldEmGameState(array<int, 2> player_stacks,
                               array<int, 2> current_pips,
                               int current_pot,
                               array<array<string,2>,2> player_hands,
                               vector<string> cur_board,
                               Deck game_deck,
                               int current_player,
                               int current_street,
                               vector<Action> current_history, 
                               vector<Action> current_street_history)
  : stacks(player_stacks),
    pips(current_pips),
    pot(current_pot),
    deck(std::move(game_deck)),
    hands(std::move(player_hands)),
    board(std::move(cur_board)),
    street(current_street),
    active_player(current_player),
    action_history(std::move(current_history)),
    street_history(std::move(current_street_history))
{}


bool HoldEmGameState::is_well_formed_action(const Action& action) const {

    const string& t = action.type();
    int amt = action.amount();

    if (t == "CHECK" || t == "FOLD" || t == "CALL") {
        return (amt == 0);

    }

    if (t == "RAISE") {
        return (amt > 0);
    }

    return false;
}

bool HoldEmGameState::is_legal_action(const Action& action) const {
    if (is_terminal() || is_chance_node()) return false;

    const string t = action.type();
    const int amt = action.amount();
    if (amt < 0) return false;

    bool facing_bet = (pips[active_player] < pips[1-active_player]);

    int to_call = pips[1-active_player] - pips[active_player];
    int min_raise_to = max(2, pips[1-active_player] + to_call);
    int max_raise_to = min(pips[0] + stacks[0], pips[1] + stacks[1]);

    if (t == "RAISE") {
        return (amt >= min_raise_to) && (amt <= max_raise_to);
    }

    if (t == "FOLD")  return facing_bet;
    if (t == "CALL")  return facing_bet;
    if (t == "CHECK") return !facing_bet;

    throw logic_error("Unknown action type");
}

bool HoldEmGameState::is_terminal() const {

    if (street == 8) return true;
    if (street_history.size() == 0) return false;
    if (street_history.back().type() == "FOLD") return true;
    return false;
}

unique_ptr<GameState> HoldEmGameState::next_game_state_impl(const Action& action) const {

    if (is_terminal()) {
        throw logic_error("Cannot get next game state from terminal state");
    }
    if (is_chance_node()) {
        throw logic_error("Cannot make an action at a chance node");
    }
    if (street < 0 || street > 7) {
        throw logic_error("Street must be between 0 and 7");
    }

    array<int, 2> new_stacks = stacks;
    array<int, 2> new_pips = pips;
    int new_pot = pot;

    Deck new_deck = deck;
    array<array<string,2>, 2> new_hands = hands;
    vector<string> new_board = board;

    int new_active_player = 1 - active_player;
    int new_street = street;

    vector<Action> new_action_history = action_history;
    vector<Action> new_street_history = street_history; 
    new_action_history.push_back(action);
    new_street_history.push_back(action);

    const string& t = action.type();

    int to_pay = 0;

    if (t == "CALL") {
        int to_pay = pips[1-active_player] - pips[active_player];

        if (to_pay <= 0){
            throw logic_error("The call amount should always be positive");
        }
    }
    else if (t == "RAISE"){
        to_pay = action.amount()-pips[active_player];  
    }

    new_pips[active_player] += to_pay;
    new_stacks[active_player] -= to_pay;
    new_pot += to_pay;

    bool round_ended = false;

    if (new_street_history.size() >= 2) {
        const int n = (int)new_street_history.size();
        const string& a = new_street_history[n - 2].type();
        const string& b = new_street_history[n - 1].type();

        round_ended = (a == "CHECK" && b == "CHECK") || (a == "RAISE" && b == "CALL");
    }

    if (round_ended || t == "FOLD") {
        new_street_history.clear();
        new_pips = {0, 0};
        new_active_player = 0;
        new_street_history.clear();
        new_street += 1;  
    }

    if (t == "FOLD"){
        new_street = 8;
    }

    return make_unique<HoldEmGameState>(
        new_stacks,
        new_pips,
        new_pot,
        new_hands,
        new_board,
        std::move(new_deck),
        new_active_player,
        new_street,
        std::move(new_action_history),
        std::move(new_street_history)
    );
}

unique_ptr<GameState> HoldEmGameState::sample_chance_node(mt19937& rng) const {
    if (!is_chance_node()) {
        throw logic_error("sample_chance_node called when not at a chance node");
    }

    array<int, 2> new_stacks = stacks;
    array<int, 2> new_pips = pips;
    int new_pot = pot;

    Deck new_deck = deck;
    array<array<string,2>,2> new_hands = hands;
    vector<string> new_board = board;

    vector<Action> new_action_history = action_history;
    vector<Action> new_street_history = street_history;

    new_deck.shuffle_remaining(rng);

    if (street == 0) {  
        new_hands[0][0] = new_deck.deal(); 
        new_hands[0][1] = new_deck.deal();

        new_hands[1][0] = new_deck.deal(); 
        new_hands[1][1] = new_deck.deal(); 
    } 
        
    else if (street == 2) {
           
        if (new_board.size() != 0) throw logic_error("Flop chance node hit but board is not empty");
        new_board.push_back(new_deck.deal());
        new_board.push_back(new_deck.deal());
        new_board.push_back(new_deck.deal());
    } 
        
    else if (street == 4) {
        if (new_board.size() != 3) throw logic_error("Turn chance node hit but board size != 3");

        new_board.push_back(new_deck.deal());
    } 
    
    else if (street == 6) {
        if (new_board.size() != 4) throw logic_error("River chance node hit but board size != 4");
        new_board.push_back(new_deck.deal());
    }

    else {
        throw logic_error("Unsupported street value in sample_chance_node");
    }

    return make_unique<HoldEmGameState>(
        new_stacks,
        new_pips,
        new_pot,
        new_hands,                 // 4th = hands
        new_board,                 // 5th = board
        std::move(new_deck),       // 6th = deck
        0, //active_player
        street+1,
        std::move(new_action_history),
        std::move(new_street_history)   
    );
}

double HoldEmGameState::get_rewards(int player) const {

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
    }

    else {
        if (board.size() != 5) throw logic_error("Showdown requires 5 board cards");

        vector<string> player_cards;
        player_cards.reserve(7);
        player_cards.push_back(hands[player][0]);
        player_cards.push_back(hands[player][1]);

        vector<string> opp_cards;
        opp_cards.reserve(7);
        opp_cards.push_back(hands[1 - player][0]);
        opp_cards.push_back(hands[1 - player][1]);

        for (const string& card : board) {
            player_cards.push_back(card);
            opp_cards.push_back(card);
        }

        uint32_t player_score = eval_cards(player_cards);
        uint32_t opp_score = eval_cards(opp_cards);

        if (player_score > opp_score) win_share = 1.0;
        else if (player_score == opp_score) win_share = 0.5;
        else win_share = 0.0;
    }

    return stacks[player] - starting_stack + win_share * static_cast<double>(pot) ;
}


int HoldEmGameState::get_active_player() const {
    return active_player;
}

vector<Action> HoldEmGameState::get_action_history() const {
    return action_history;
}

bool HoldEmGameState::is_chance_node() const {
    return (street%2 == 0 && street != 8);
}

array<int, 2> HoldEmGameState::get_raise_bounds() const{
    int to_call = pips[1-active_player] - pips[active_player];
    int min_raise_to = max(2, pips[1-active_player] + to_call);
    int max_raise_to = min(pips[0] + stacks[0], pips[1] + stacks[1]);

    array<int, 2> output = {min_raise_to, max_raise_to};
    return output;
}

int HoldEmGameState::get_pot() const{
    return pot;
}

string HoldEmGameState::get_board() const {
    string output;

    for (const string& card : board){
        output += card;
    }

    return output;
}


string HoldEmGameState::get_hand(int player) const {

    if (player != 0 && player!= 1){
        throw logic_error("The player index must be one of 1 or 0");
    }

    return hands[player][0] + hands[player][1];
}



