#include "CFR/GameState.h"
#include "CFR/Action.h" 
using std::string;

struct BruteAbstraction{    
    
public:

    static string get_InfoSetID(int player, const GameState& state) {
    string output;

    output += state.get_hand(player);
    output += "|";
    output += state.get_board();
    output += "|";

    for (const Action& action : state.get_action_history()) {
        output += action.description();
        output += ",";
    }

    return output;
}

};