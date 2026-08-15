#pragma once
#include "card_buckets.h"
#include "action.h"
#include "action_tree.h"

#include <filesystem>
#include <vector>
#include <string>
#include <utility>
#include <random>

struct ISetsPaths{
    std::string regret_path;
    std::string strategy_path;
    std::string offset_path;
    std::string iters_path;

    void remove() const {
        std::filesystem::remove(regret_path);
        std::filesystem::remove(strategy_path);
        std::filesystem::remove(offset_path);
        std::filesystem::remove(iters_path);
    };
};

struct InfoKey {
    size_t node_idx;
    size_t cluster_idx;
    size_t num_actions;
};

class InfoSets {
    
public:

    inline size_t get_offset(const InfoKey& ikey)  const{
        return offsets[ikey.node_idx] + ikey.cluster_idx*ikey.num_actions;
    }

    std::vector<size_t> offsets;  
    std::vector<double> regret_sum; 
    std::vector<double> strategy_sum; 
    int last_discount_iter = 0;
    int cur_iter = 0;

    explicit InfoSets(const ActionTree& action_tree, const std::vector<size_t>& cluster_counts);

    explicit InfoSets(const ISetsPaths& paths);

    void write_ckpt(const ISetsPaths& paths) const;

    void update_regret(const InfoKey& ikey, const std::vector<double>& action_deltas);

    void update_strategy(const InfoKey& ikey, std::vector<double>& cur_strat);

    void get_regret_strategy(const InfoKey& ikey, std::vector<double>& output) const;

    void get_strategy(const InfoKey& ikey, std::vector<double>& output) const;
   
    size_t sample_regret(std::mt19937& rng, std::vector<double>& probs) const;

    void discount(int t);

};