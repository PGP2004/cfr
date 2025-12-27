#include "CFR/InfoSet.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace std;

void InfoSet::init(const GameState& state) {

    vector<pair<string, Action>> proposed_actions = propose_actions(state);

    abstract_to_concrete.clear();
    concrete_to_abstract.clear();
    regret_sum.clear();
    strategy_sum.clear();

    for (const auto& [abs, act] : proposed_actions) {

        if (!state.is_legal_action(act)) continue;

        abstract_to_concrete.emplace(abs, act);
        concrete_to_abstract.emplace(act.description(), abs);
        regret_sum[abs] = 0.0;
        strategy_sum[abs] = 0.0;
    }

    if (regret_sum.empty()) {
        throw logic_error("InfoSet::init: 0 legal actions");
    }
}

unordered_map<string, double> InfoSet::get_regret_strategy() const {
    double total_sum = 0.0;
    unordered_map<string, double> output;
    
    for (const auto& [key, val] : regret_sum) {
        total_sum += max(val, 0.0);
    }
    
    double num_actions = static_cast<double>(regret_sum.size());
    for (const auto& [key, val] : regret_sum) {
        if (total_sum == 0.0) {
            output[key] = 1.0 / num_actions;
        } else {
            output[key] = max(0.0, val) / total_sum;
        }
    }
    
    return output;
}

unordered_map<string, double> InfoSet::get_average_strategy() const {
    double total = 0.0;
    for (const auto& [key, val] : strategy_sum) {
        total += val;
    }
    
    unordered_map<string, double> output;
    double num_actions = static_cast<double>(strategy_sum.size());
    
    if (num_actions == 0) {
        return output;
    }
    
    if (total <= 1e-12) {
        for (const auto& [key, val] : strategy_sum) {
            output[key] = 1.0 / num_actions;
        }
    } else {
        for (const auto& [key, val] : strategy_sum) {
            output[key] = val / total;
        }
    }
    
    return output;
}

vector<pair<Action, double>> InfoSet::get_actions_with_probs() const {
    vector<pair<Action, double>> output;
    unordered_map<string, double> regret_strat = get_regret_strategy();

    for (const auto& [abstract, concrete] : abstract_to_concrete) {
        output.push_back({concrete, regret_strat.at(abstract)});
    }
    return output;
}

void InfoSet::update_regret(const Action& action, double delta) {

    if (concrete_to_abstract.count(action.description()) == 0){
        throw invalid_argument("Can only update when its taking in a valid infoset atcion");
    }

    string abstract = concrete_to_abstract.at(action.description());
    
    if (regret_sum.count(abstract) == 0) {
        throw invalid_argument("Cannot update regret for illegal action");
    }
    
    regret_sum[abstract] += delta;
}

void InfoSet::update_average_strategy(double reach_prob) {
    if (reach_prob < 0.0 || reach_prob > 1.0) {
        throw invalid_argument("Reach probability must be between 0 and 1");
    }
    
    unordered_map<string, double> current_strategy = get_regret_strategy();
    for (const auto& [abstract, prob] : current_strategy) {
        strategy_sum[abstract] += reach_prob * prob;
    }
}

pair<Action, double> InfoSet::sample_regret_action(mt19937& rng) const {
    unordered_map<string, double> strategy = get_regret_strategy();
    
    vector<string> abstract_actions;
    vector<double> probabilities;
    
    for (const auto& [abstract, prob] : strategy) {
        abstract_actions.push_back(abstract);
        probabilities.push_back(prob);
    }
    
    discrete_distribution<int> dist(probabilities.begin(), probabilities.end());
    int sampled_idx = dist(rng);
    
    string sampled_abstract = abstract_actions[sampled_idx];
    Action concrete_action = abstract_to_concrete.at(sampled_abstract);
    double probability = probabilities[sampled_idx];
    
    return {concrete_action, probability};
}