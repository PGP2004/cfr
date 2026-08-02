#include "info_sets.h"
#include "action_tree.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cmath>

InfoSets::InfoSets(const ActionTree& action_tree, const std::vector<size_t> cluster_counts) {

    size_t cum_total = 0;

    for (const PublicState& pub_state : action_tree.pub_states){

        int st = pub_state.street_idx;
        int num_actions = pub_state.edge_labels.size();
        size_t num_clusters = cluster_counts[st];

        offsets.push_back(cum_total);
        cum_total += num_actions*num_clusters;
    }

    regret_sum.assign(cum_total, 0.0);
    strategy_sum.assign(cum_total, 0.0);
}


void InfoSets::update_regret(const InfoKey& ikey, const std::vector<double>& action_deltas) {

    size_t offset = get_offset(ikey);
    size_t n = ikey.get_num_actions();

    if (n != action_deltas.size()) {
        throw std::invalid_argument("Action_deltas and regret_sum must have the same size");
    }

    for (size_t i = 0; i < n; i++) {
        regret_sum[offset+i] += action_deltas[i];  
    }
}

void InfoSets::update_strategy(const InfoKey& ikey , std::vector<double>& cur_strat) {

    size_t offset = get_offset(ikey);
    size_t n = ikey.get_num_actions();

    if (n != cur_strat.size()) throw std::logic_error("size mismatch");

    for (size_t i = 0; i < n; i++) {
        strategy_sum[offset+i] += cur_strat[i];
    }
}

void InfoSets::get_regret_strategy(const InfoKey& ikey, std::vector<double>& output) const{

    size_t offset = get_offset(ikey);
    size_t n = ikey.get_num_actions();
    
    output.resize(n);

    double total_sum = 0.0;

    for (size_t i = 0; i < n; ++i) total_sum += std::max(regret_sum[offset+i], 0.0);
        
    if (total_sum > 0.0) {
        for (size_t i = 0; i < n; i++) output[i] = std::max(0.0, regret_sum[offset+i]) / total_sum;
    }

    else {
        double uniform = 1.0 / n;
        for (size_t i = 0; i < n; i++) output[i] = uniform;
    }

}

size_t InfoSets::sample_regret(const InfoKey& ikey, std::mt19937& rng, std::vector<double>& probs) const {

    get_regret_strategy(ikey ,probs);

    std::uniform_real_distribution<double> unif(0.0, 1.0);
    double r = unif(rng);
    double cum = 0.0;
    size_t idx = probs.size() - 1;  
    
    for (size_t i = 0; i < probs.size(); ++i) {
        cum += probs[i];
        if (r < cum) { idx = i; break; }
    }
    return idx;
}

void InfoSets::discount(int t) {

    if (t <= last_t) throw std::runtime_error("discount: t must exceed last_t");

    double f = double(last_t + 1) / double(t + 1);
    for (double& r : regret_sum)   r *= f;
    for (double& s : strategy_sum) s *= f;
    last_t = t;
}

// std::vector<std::pair<Action, double>> InfoSets::get_average_strategy(const InfoKey& ikey) const{

//     std::vector<std::pair<Action, double>> output;

//     size_t offset = get_offset(ikey);
//     size_t n = ikey.get_num_actions();
//     output.resize(n);

//     double sum = 0.0;
//     for (size_t i = 0 ; i < n; ++i){
//         sum += strategy_sum[offset+i];
//     }

//     if (sum <= 0.0){
//         double p = 1.0/double(n); 
//         for (size_t i = 0; i < n; ++i) output[i] = {ikey.node.edges[i], p};
//     }
//     else {
//         for (size_t i = 0; i < n; ++i) output[i] = {ikey.node.edges[i], strategy_sum[offset + i] / sum};
//     }

//     return output;
// }