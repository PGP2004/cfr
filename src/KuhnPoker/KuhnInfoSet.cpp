#include "KuhnPoker/KuhnInfoSet.h"
#include <stdexcept>

using namespace std;

const vector<string> KuhnInfoSet::possible_actions = {"CHECK", "CALL", "FOLD", "RAISE"};

vector<pair<string, Action>> KuhnInfoSet::propose_actions(const GameState& state) const {
    vector<pair<string, Action>> out;
    out.reserve(possible_actions.size());

    auto make_action = [&](const string& s) -> Action {
        if (s == "RAISE") return Action("RAISE", 1);
        if (s == "CALL")  return Action("CALL", 1);
        if (s == "FOLD")  return Action("FOLD");
        if (s == "CHECK") return Action("CHECK");
        throw invalid_argument("Unknown action string: " + s);
    };

    for (const string& s : possible_actions) {
        Action a = make_action(s);
        if (state.is_legal_action(a)) out.push_back({s, a});
    }
    return out;
}

KuhnInfoSet::KuhnInfoSet(const GameState& state){
    init(state);
}
