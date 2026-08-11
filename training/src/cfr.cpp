#include "game_state.h"
#include "info_sets.h"
#include "card_buckets.h"
#include "cfr.h"    

#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include <iostream>


CFR::CFR(CardBuckets buckets, ActionTree at):
    card_buckets(std::move(buckets)),
    action_tree(std::move(at)),
    infosets(this->action_tree, this->card_buckets.cluster_counts){

    probs_scratch.assign(action_tree.depth() + 1, std::vector<double>(action_tree.max_branching()));
    deltas_scratch.assign(action_tree.depth() + 1, std::vector<double>(action_tree.max_branching()));
}

InfoKey CFR::get_InfoKey(const ActionTree& at, const Dealer& d) {
    const TreeNode& n = at.nodes[at.cur_idx];
    int street = at.street();
    int hand_id = d.get_hand_id(at.active_player(), street);
    return {n.node_idx, (size_t)card_buckets.cluster_of(street, hand_id), n.child_idxs.size()};
}

double CFR::traverse_helper(int player, int depth) {

    if (action_tree.is_terminal()) {
        return dealer.get_reward(player, action_tree);
    }

    int active_player = action_tree.active_player();

    if (active_player != player) {

        std::vector<double>&probs = probs_scratch[depth];

        InfoKey ikey = get_InfoKey(action_tree, dealer);
        infosets.get_regret_strategy(ikey, probs);
        infosets.update_strategy(ikey, probs); 

        size_t sampled_idx = infosets.sample_regret(rng, probs);
        action_tree.apply_action(sampled_idx);
        double util = traverse_helper(player, depth + 1);
        action_tree.undo_action();

        return util;
    }

    // Branch where active player = current player
    std::vector<double>& probs = probs_scratch[depth];
    std::vector<double>& action_deltas = deltas_scratch[depth];

    InfoKey ikey = get_InfoKey(action_tree, dealer);
    infosets.get_regret_strategy(ikey, probs);
    action_deltas.assign(ikey.num_actions, 0.0);

    double node_util = 0.0;

    for (size_t i = 0; i < ikey.num_actions; i++) {

        action_tree.apply_action(i);
        double action_util = traverse_helper(player, depth + 1);
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
    traverse_helper(player, 0);
}

void CFR::train(int num_iters, int iters_per_discount) {

    int starting_iter = infosets.cur_iter;

    for (int i = 0; i < num_iters; ++i) {
        traverse(0); 
        traverse(1);
        infosets.cur_iter += 1;

        if ((i+starting_iter) % iters_per_discount == 0 && i != 0){
            infosets.discount(i+starting_iter);
        }

        infosets.cur_iter +=1;

    }
}
