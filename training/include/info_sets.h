#pragma once
#include "card_buckets.h"
#include "action.h"
#include "action_tree.h"

#include <vector>
#include <string>
#include <utility>
#include <random>

struct InfoKey {
    const ActionNode& node;
    size_t cluster_idx;

    inline size_t get_num_actions() const{
        return node.edges.size();
    }

    Action get_action(size_t action_idx) const{
        if (action_idx >= node.child_idxs.size()){
             throw std::out_of_range("the idx is out of range");
        }
        return node.edges[action_idx];
    }
};

class InfoSets {
    
private: 

    std::vector<size_t> offsets;  
    std::vector<double> regret_sum; 
    std::vector<double> strategy_sum; 
    int last_t = 0;

    inline size_t get_offset(const InfoKey& ikey)  const{
        return offsets[ikey.node.node_idx] + ikey.cluster_idx*ikey.get_num_actions();
    }

public:

    explicit InfoSets(const ActionTree& action_tree, const std::vector<size_t> cluster_counts);

    void update_regret(const InfoKey& ikey, const std::vector<double>& action_deltas);

    void update_strategy(const InfoKey& ikey, std::vector<double>& cur_strat);

    void get_regret_strategy(const InfoKey& ikey, std::vector<double>& output) const;

    std::vector<std::pair<Action, double>> get_average_strategy(const InfoKey& ikey) const;

    size_t sample_regret(const InfoKey& ikey, std::mt19937& rng, std::vector<double>& probs) const;

    void discount(int t);
   
    void get_probs(const InfoKey& ikey, std::vector<double>& probs) const;

};