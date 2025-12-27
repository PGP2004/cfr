#pragma once
#include <random>
#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include <random>
#include "Action.h"
#include "GameState.h"

using std::string;
using std::vector;
using std::unordered_map;
using std::pair;
using std::mt19937;

class InfoSet {
    
protected: //TODO: action.description() -> string rep in infoset
    virtual vector<pair<string, Action>> propose_actions(const GameState& state) const = 0;

    InfoSet() = default;
    void init(const GameState& state);

    unordered_map<string, Action> abstract_to_concrete;
    unordered_map<string, string> concrete_to_abstract;

    unordered_map<string, double> regret_sum; //TODO: move back to be private
    unordered_map<string, double> strategy_sum; //TODO: move back to be private

    
public:

    virtual ~InfoSet() = default;
    
    unordered_map<string, double> get_regret_strategy() const;
    unordered_map<string, double> get_average_strategy() const;

    pair<Action, double> sample_regret_action(mt19937& rng) const;
    vector<pair<Action, double>> get_actions_with_probs() const;

    void update_regret(const Action& action, double delta);
    void update_average_strategy(double reach_prob);
};