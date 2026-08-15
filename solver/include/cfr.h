#pragma once
#include <random>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <omp.h>

#include "action.h"
#include "poker_state.h"
#include "card_buckets.h"

#include "info_sets.h"
#include "action_tree.h"
#include "dealer.h"

struct ThreadBuff{
    std::mt19937 rng;
    std::vector<std::vector<double>> probs_scratch;
    std::vector<std::vector<double>> deltas_scratch;
    Dealer dealer;
};

class CFR {

    private:
        CardBuckets card_buckets;
        ActionTree action_tree;
        InfoSets infosets;

        double traverse(int player, size_t node_idx, size_t depth, ThreadBuff& buff);
        std::vector<ThreadBuff> make_thread_buffs(size_t num_threads, uint32_t base_seed);

    public:
        CFR(CardBuckets buckets, ActionTree at);
        CFR(InfoSets isets, CardBuckets buckets, ActionTree at);
        InfoKey get_InfoKey(size_t node_idx, const ActionTree& at, const Dealer& d);
 
        void train(size_t iters, size_t iters_per_discount, 
            size_t num_threads, size_t omp_chunk_sz, uint32_t base_seed);

        const ActionTree& get_action_tree()const {return action_tree;}
        const InfoSets& get_infosets()const {return infosets;}

    };