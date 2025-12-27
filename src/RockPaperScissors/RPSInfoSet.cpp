#include "RockPaperScissors/RPSInfoSet.h"
#include <stdexcept>

using namespace std;

const vector<string> RPSInfoSet::possible_actions = {"ROCK", "PAPER", "SCISSORS"};

vector<pair<string, Action>>

RPSInfoSet::propose_actions(const GameState& state) const {
    vector<pair<string, Action>> out;

    for (const string& s : possible_actions) {
        Action a(s); 
        if (state.is_legal_action(a)) out.push_back({s, a});
    }

    return out;
}

RPSInfoSet::RPSInfoSet(const GameState& state) {
    init(state);
}
