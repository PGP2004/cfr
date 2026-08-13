#include "info_sets.h"
#include "action_tree.h"
#include "matrix_loader.h"

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cmath>

InfoSets::InfoSets(const ActionTree& action_tree, const std::vector<size_t>& cluster_counts) {

    size_t cum_total = 0;

    for (const PublicState& pub_state : action_tree.pub_states){

        int st = pub_state.street_idx;
        int num_actions = pub_state.edge_labels.size();
        offsets.push_back(cum_total);

        if (num_actions != 0){
            size_t num_clusters = cluster_counts[st];
            cum_total += num_actions*num_clusters;
        }
    }

    regret_sum.assign(cum_total, 0.0);
    strategy_sum.assign(cum_total, 0.0);
}

InfoSets::InfoSets(const ISetsPaths& paths) {

    auto [iter_info, temp_header] = load_matrix_and_header<int>(paths.iter_info_path);
    if (iter_info.size() != 2) throw std::runtime_error("The iter_info vector should have size 1");
    last_discount_iter = iter_info[0]; cur_iter = iter_info[1];

    auto [loaded_regret, regret_header] = load_matrix_and_header<double>(paths.regret_path);
    regret_sum = std::move(loaded_regret);

    auto [loaded_strategy, strategy_header] = load_matrix_and_header<double>(paths.strategy_path);
    strategy_sum = std::move(loaded_strategy);

    auto [loaded_offsets, offset_header] = load_matrix_and_header<size_t>(paths.offset_path);
    offsets = std::move(loaded_offsets);
}

void InfoSets::write_check_point(const ISetsPaths& paths){

    MatrixHeader regret_header{
        .num_rows = regret_sum.size(),
        .num_cols = 1,
        .bytes_per_elt = sizeof(double),
        .is_signed = true,
        .is_float = true
    };
    write_matrix_and_header(paths.regret_path, regret_header, regret_sum);


    MatrixHeader strategy_header{
        .num_rows = strategy_sum.size(),
        .num_cols = 1,
        .bytes_per_elt = sizeof(double),
        .is_signed = true,
        .is_float = true
    };
    write_matrix_and_header(paths.strategy_path, strategy_header, strategy_sum);

    MatrixHeader offset_header{
        .num_rows = offsets.size(),
        .num_cols = 1,
        .bytes_per_elt = sizeof(size_t),
        .is_signed = false,
        .is_float = false
    };
    write_matrix_and_header(paths.offset_path, offset_header, offsets);

    MatrixHeader iter_info_header{
        .num_rows = 2,
        .num_cols = 1,
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false
    };
    std::vector<int> temp_vec = {last_discount_iter, cur_iter};
    write_matrix_and_header(paths.iter_info_path, iter_info_header, temp_vec);
}

void InfoSets::update_regret(const InfoKey& ikey, const std::vector<double>& action_deltas) {

    size_t offset = get_offset(ikey);
    size_t n = ikey.num_actions;

    if (n != action_deltas.size()) {
        throw std::invalid_argument("Action_deltas and regret_sum must have the same size");
    }

    for (size_t i = 0; i < n; i++) {
        regret_sum[offset+i] += action_deltas[i];  
    }
}

void InfoSets::update_strategy(const InfoKey& ikey , std::vector<double>& cur_strat) {

    size_t offset = get_offset(ikey);

    if (ikey.num_actions != cur_strat.size()) throw std::logic_error("size mismatch");

    for (size_t i = 0; i < ikey.num_actions; i++) {
        strategy_sum[offset+i] += cur_strat[i];
    }
}

void InfoSets::get_regret_strategy(const InfoKey& ikey, std::vector<double>& output) const{

    size_t offset = get_offset(ikey);
    size_t n = ikey.num_actions;
    
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

void InfoSets::get_strategy(const InfoKey& ikey, std::vector<double>& output) const{
    size_t offset = get_offset(ikey);
    size_t n = ikey.num_actions;
    
    output.resize(n);
    double total_sum = 0.0;

    for (size_t i = 0; i < n; ++i) total_sum += std::max(strategy_sum[offset+i], 0.0);
        
    if (total_sum > 0.0) {
        for (size_t i = 0; i < n; i++) output[i] = std::max(0.0, strategy_sum[offset+i]) / total_sum;
    }

    else {
        double uniform = 1.0 / n;
        for (size_t i = 0; i < n; i++) output[i] = uniform;
    }
}

size_t InfoSets::sample_regret( std::mt19937& rng, std::vector<double>& probs) const {

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

    if (t <= last_discount_iter) throw std::runtime_error("discount: t must exceed last_discounter_iter");

    double f = double(last_discount_iter + 1) / double(t + 1);
    for (double& r : regret_sum)   r *= f;
    for (double& s : strategy_sum) s *= f;
    last_discount_iter = t;
}
