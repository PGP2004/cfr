#pragma once
#include "card_buckets.h"
#include "action.h"
#include "action_tree.h"

#include <vector>
#include <string>
#include <utility>
#include <random>

struct InfoKey {
    size_t node_idx;
    size_t cluster_idx;
    size_t num_actions;
};

class InfoSets {
    
private: 

    std::vector<size_t> offsets;  
    std::vector<double> regret_sum; 
    std::vector<double> strategy_sum; 
    int last_t = 0;

    inline size_t get_offset(const InfoKey& ikey)  const{
        return offsets[ikey.node_idx] + ikey.cluster_idx*ikey.num_actions;
    }

public:

    explicit InfoSets(const ActionTree& action_tree, const std::vector<int>& cluster_counts);

    void update_regret(const InfoKey& ikey, const std::vector<double>& action_deltas);

    void update_strategy(const InfoKey& ikey, std::vector<double>& cur_strat);

    void get_regret_strategy(const InfoKey& ikey, std::vector<double>& output) const;

    size_t sample_regret(std::mt19937& rng, std::vector<double>& probs) const;

    void discount(int t);

};