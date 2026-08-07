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


CFR::CFR(CardBuckets& buckets, ActionTree& action_tree): 
    card_buckets(buckets), 
    action_tree(action_tree),
    infosets(action_tree, buckets.cluster_counts){

    dealer = Dealer();
    VectorPool::preallocate(4, 200);
    iters_per_discount = 1000;
}


InfoKey CFR::get_InfoKey(const ActionTree& at, const Dealer& cur_dealer) {
    int player = at.active_player();
    int street = at.street();
    int hand_id = cur_dealer.get_hand_id(player, street);
    int hand_cluster = card_buckets.cluster_of(street, hand_id);  
    InfoKey ikey(at.nodes[at.cur_idx], hand_cluster);
    return ikey;
}

double CFR::traverse_helper(int player) {

    if (action_tree.is_terminal()) {
        return dealer.get_reward(player, action_tree);
    }

    // Player Action Branch
    int active_player = action_tree.active_player();

    if (active_player != player) {
        InfoKey ikey = get_InfoKey(action_tree, dealer);
        VectorPool::ProbsBuffer probs_buf;
        auto& probs = probs_buf.get();

        infosets.get_regret_strategy(ikey, probs);
        infosets.update_strategy(ikey, probs); 

        size_t sampled_idx = infosets.sample_regret(ikey, rng, probs);
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
    size_t num_actions = ikey.get_num_actions();
    action_deltas.assign(num_actions, 0.0);

    double node_util = 0.0;

    for (size_t i = 0; i < num_actions; i++) {

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
        traverse(0); traverse(1);
        if (i % iters_per_discount == 0 && i != 0) infosets.discount(i);
    }
}