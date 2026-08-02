#include "game_state.h"
#include "info_sets.h"
#include "card_buckets.h"
#include "cfr.h"    
#include "vector_pool.h"

#include <memory>
#include <utility>
#include <vector>
#include <array>
#include <iostream>


CFR::CFR(GameState init_state, CardBuckets& buckets, ActionTree& action_tree): state(std::move(init_state)),
   card_buckets(buckets), action_tree(action_tree), infosets(action_tree, buckets.cluster_counts) {
    //TODO: make the params here knobs

    dealer = Dealer();
    VectorPool::preallocate(4, 200);
    iters_per_discount = 1000;
}

InfoKey CFR::get_InfoKey(const ActionTree& at) {
    int player = at.get_player();
    int street = at.get_street();
    int hand_id = dealer.get_hand_id(player, street);
    int hand_cluster = card_buckets.cluster_of(street, hand_id);  
    InfoKey ikey(at.nodes[at.cur_idx], hand_cluster);
    return ikey;
}

double CFR::traverse(int player, ActionTree& at, Dealer& cur_dealer) {
    
    if (at.is_terminal()) {
        return get_reward(at, cur_dealer);
    }

    if (at.is_root_node()){
        cur_dealer.deal_and_update_equities(rng);
        return traverse(player, at, cur_dealer);
    }

    // Player Action Branch
    int active_player = at.get_player();
    if (active_player != player) {

        InfoKey ikey = get_InfoKey(at);
        VectorPool::ProbsBuffer probs_buf;
        auto& probs = probs_buf.get();

        size_t sampled_idx = infosets.sample_regret(ikey, rng, probs);
        Action sampled_action = ikey.get_action(sampled_idx);
        infosets.update_strategy(ikey, probs);

        at.apply_action(sampled_idx);
        double util = traverse(player, at, cur_dealer);
        at.undo_action();

        return util;
    }

    // Branch where active player = current player
    InfoKey ikey = get_InfoKey(at);
    VectorPool::ProbsBuffer probs_buf;
    VectorPool::DeltaBuffer delta_buf;
    vector<double>& probs = probs_buf.get();
    vector<double>& action_deltas = delta_buf.get();

    infosets.get_probs(ikey, probs);
    size_t num_actions = ikey.get_num_actions();
    action_deltas.assign(num_actions, 0.0);

    double node_util = 0.0;

    for (size_t i = 0; i < num_actions; i++) {

        Action action = ikey.get_action(i);
        at.apply_action(i);
        double action_util = traverse(player, at, cur_dealer);
        at.undo_action();

        node_util += probs[i] * action_util;
        action_deltas[i] = action_util;
    }

    for (size_t i = 0; i < action_deltas.size(); ++i) {
        action_deltas[i] = action_deltas[i] - node_util;
    }

    infosets.update_regret(ikey, action_deltas);
    return node_util;
}

void CFR::train(int num_iterations, int starting_iter) {
    auto t0 = std::chrono::steady_clock::now();

    for (int i = starting_iter; i < starting_iter + num_iterations; ++i) {
        traverse(0, action_tree, dealer);
        traverse(1, action_tree, dealer);
        if (i % iters_per_discount == 0 && i != 0) infosets.discount(i);
    }
}

