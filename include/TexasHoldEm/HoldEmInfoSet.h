#pragma once
#include "CFR/InfoSet.h"

class HoldEmInfoSet : public InfoSet {

protected:
    vector<pair<string, Action>> propose_actions(const GameState& state) const override;

public:
    explicit HoldEmInfoSet(const GameState& state);
};