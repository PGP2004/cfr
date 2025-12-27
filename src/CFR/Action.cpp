#include "CFR/Action.h"
#include <string>

using namespace std;

Action::Action(string input_type, int amount)
    : action_type(std::move(input_type)), bet_amount(amount) {

    action_description = action_type;
    if (action_type == "RAISE" || action_type == "CALL") {
        action_description += " " + to_string(bet_amount);
    }
}

const string& Action::type() const {
    return action_type;
}

const string& Action::description() const {
    return action_description;
}

int Action::amount() const {
    return bet_amount;
}

bool Action::operator==(const Action& other) const {
    return action_type == other.action_type && bet_amount == other.bet_amount;
}
