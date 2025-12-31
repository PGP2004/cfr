#include "TexasHoldEm/HoldEmInfoSet.h"
#include "CFR/GameState.h"
#include <stdexcept>
#include <array>


using namespace std;

vector<pair<string, Action>> HoldEmInfoSet::propose_actions(const GameState& state) const {

    int pot = state.get_pot();
    int half_pot_raise = pot/2;
    int full_pot_raise  = pot;

    //at some point modify so it changes pot and half pot in case its not legal to be the max bet?

    vector<pair<string, Action>> candidates = {
    {"check",    Action("CHECK")},
    {"call",     Action("CALL")},          
    {"fold",     Action("FOLD")},
    {"half_pot", Action("RAISE", half_pot_raise)},
    {"full_pot", Action("RAISE", full_pot_raise)},
    };


    vector<pair<string, Action>> output;

    for (const pair<string, Action>& name_act_pair: candidates) {

        if (state.is_legal_action(name_act_pair.second)) {
            output.push_back(name_act_pair);
        }
    }

    return output;
}

HoldEmInfoSet::HoldEmInfoSet(const GameState& state){
    init(state);
}