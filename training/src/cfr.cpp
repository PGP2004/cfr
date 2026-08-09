#include "game_state.h"
#include "info_sets.h"
#include "card_buckets.h"
#include "cfr.h"    
#include "vector_pool.h"

#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include <iostream>

CFR::CFR(CardBuckets& buckets, ActionTree& action_tree): 
    card_buckets(buckets), 
    action_tree(action_tree),
    infosets(action_tree, buckets.cluster_counts){
    VectorPool::preallocate(4, 200);
    iters_per_discount = 1000;
}

CFR::CFR(const CheckPoint& ck_pt, CardBuckets& buckets, ActionTree& action_tree): 
    card_buckets(buckets), 
    action_tree(action_tree),
    infosets(ck_pt){
    VectorPool::preallocate(4, 200);
    iters_per_discount = 1000;
}

InfoKey CFR::get_InfoKey(const ActionTree& at, const Dealer& d) {
    const TreeNode& n = at.nodes[at.cur_idx];
    int street = at.street();
    int hand_id = d.get_hand_id(at.active_player(), street);
    return {n.node_idx, (size_t)card_buckets.cluster_of(street, hand_id), n.child_idxs.size()};
}

double CFR::traverse_helper(int player) {

    if (action_tree.is_terminal()) {
        return dealer.get_reward(player, action_tree);
    }

    int active_player = action_tree.active_player();

    if (active_player != player) {
        InfoKey ikey = get_InfoKey(action_tree, dealer);
        VectorPool::ProbsBuffer probs_buf;
        auto& probs = probs_buf.get();

        infosets.get_regret_strategy(ikey, probs);
        infosets.update_strategy(ikey, probs); 

        size_t sampled_idx = infosets.sample_regret(rng, probs);
        action_tree.apply_action(sampled_idx);
        double util = traverse_helper(player);
        action_tree.undo_action();

        return util;
    }

    // Branch where active player = current player
    InfoKey ikey = get_InfoKey(action_tree, dealer);
    VectorPool::ProbsBuffer probs_buf;
    VectorPool::DeltaBuffer delta_buf;
    std::vector<double>& probs = probs_buf.get();
    std::vector<double>& action_deltas = delta_buf.get();

    infosets.get_regret_strategy(ikey, probs);
    action_deltas.assign(ikey.num_actions, 0.0);

    double node_util = 0.0;

    for (size_t i = 0; i < ikey.num_actions; i++) {

        action_tree.apply_action(i);
        double action_util = traverse_helper(player);
        action_tree.undo_action();

        node_util += probs[i] * action_util;
        action_deltas[i] = action_util;
    }

    for (size_t i = 0; i < action_deltas.size(); ++i) {
        action_deltas[i] = action_deltas[i] - node_util;
    }

    infosets.update_regret(ikey, action_deltas);
    return node_util;
}

void CFR::traverse(int player){
    dealer.deal(rng);
    action_tree.restart();
    traverse_helper(player);
}

void CFR::train(int num_iterations, int starting_iter) {

    for (int i = starting_iter; i < starting_iter + num_iterations; ++i) {
        traverse(0); 
        traverse(1);
        if (i % iters_per_discount == 0 && i != 0) infosets.discount(i);
    }
}


double CFR::get_prob(InfoKey ikey, Action a){

    std::vector<double> strat_vec;
    infosets.get_strategy(ikey, strat_vec);
    std::vector<Action> actions = action_tree.pub_states[ikey.node_idx].edge_labels;
    
    for (size_t i = 0; i < actions.size(); ++i){
        if (actions[i] == a){
            return strat_vec[i];
        }
    }

    return -1.0;
}

//chatbot function <- Clean at some point!
std::unordered_map<std::string, double> CFR::preflop_probs(Action target_action) {
    static const std::array<uint8_t, 1> cpr = {2};
    static Indexer idx{1, cpr.data()};
    static const std::array<std::string, 13> rank = {
        "2","3","4","5","6","7","8","9","T","J","Q","K","A"};

    size_t root_idx = action_tree.root_idx;
    size_t num_actions = action_tree.pub_states[root_idx].edge_labels.size();

    auto prob = [&](uint8_t c0, uint8_t c1) {
        uint8_t cards[2] = {c0, c1};
        InfoKey ikey{root_idx, hand_index_last(&idx.h, cards), num_actions};
        return get_prob(ikey, target_action);
    };

    std::unordered_map<std::string, double> output;
    for (uint8_t hi = 0; hi < 13; ++hi) {
        for (uint8_t lo = 0; lo <= hi; ++lo) {
            if (hi == lo) { 
                output[rank[hi] + rank[hi]] = prob(make_card(hi, 0), make_card(hi, 1));
            } else {
                output[rank[hi] + rank[lo] + "o"] = prob(make_card(hi, 0), make_card(lo, 1));
                output[rank[hi] + rank[lo] + "s"] = prob(make_card(hi, 0), make_card(lo, 0));
            }
        }
    }
    return output;
}