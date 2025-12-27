#pragma once
#include "CFR/InfoSet.h"

class RPSInfoSet : public InfoSet {
private:
    static const vector<string> possible_actions;

protected:
    vector<pair<string, Action>> propose_actions(const GameState& state) const override;

public:

    explicit RPSInfoSet(const GameState& state);
};
