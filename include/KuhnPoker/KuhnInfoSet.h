#pragma once
#include "CFR/InfoSet.h"

class KuhnInfoSet : public InfoSet {
private:
    static const vector<string> possible_actions; // one shared list for Kuhn

protected:

    vector<pair<string, Action>> propose_actions(const GameState& state) const override;

public:

    explicit KuhnInfoSet(const GameState& state);
};
