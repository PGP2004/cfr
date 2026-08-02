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
    dealer = Dealer();
    VectorPool::preallocate(4, 200);
    iters_per_discount = 1000;
}

//TODO: Review this and fix the way I check for folds
double CFR::get_reward(int player, ActionTree& at, Dealer& cur_dealer){
    if (! at.is_terminal()) throw std::runtime_error("cannot get reward for non-terminal node");

    int opp = 1 - player;

    //if someone folded in the game
    if (at.folded()){
        bool won = (player == at.active_player());
        if (won) return at.get_payoff(player);
        return -at.get_payoff(opp);
    }

    // if no one folded in the game. Look at the equities
    if (cur_dealer.equities[player] == 0.5) return 0.0;
    else if (cur_dealer.equities[player] == 1.0) return at.get_payoff(player);
    else if (cur_dealer.equities[player] == 0.0) return - at.get_payoff(opp);

    throw std::runtime_error("Should not be able to get here");
    return 0.0;
}

InfoKey CFR::get_InfoKey(const ActionTree& at, const Dealer& cur_dealer) {
    int player = at.active_player();
    int street = at.street();
    int hand_id = cur_dealer.get_hand_id(player, street);
    int hand_cluster = card_buckets.cluster_of(street, hand_id);  
    InfoKey ikey(at.nodes[at.cur_idx], hand_cluster);
    return ikey;
}

double CFR::traverse_helper(int player, ActionTree& at, Dealer& cur_dealer) {

    if (at.is_terminal()) {
        return get_reward(player, at, cur_dealer);
    }

    // Player Action Branch
    int active_player = at.active_player();

    if (active_player != player) {
        InfoKey ikey = get_InfoKey(at, cur_dealer);
        VectorPool::ProbsBuffer probs_buf;
        auto& probs = probs_buf.get();

        infosets.get_regret_strategy(ikey, probs);   // fill probs first
        infosets.update_strategy(ikey, probs);       // now sizes match

        size_t sampled_idx = infosets.sample_regret(ikey, rng, probs);
        at.apply_action(sampled_idx);
        double util = traverse_helper(player, at, cur_dealer);
        at.undo_action();

        return util;

    }
    // Branch where active player = current player
    InfoKey ikey = get_InfoKey(at, cur_dealer);
    VectorPool::ProbsBuffer probs_buf;
    VectorPool::DeltaBuffer delta_buf;
    std::vector<double>& probs = probs_buf.get();
    std::vector<double>& action_deltas = delta_buf.get();

    infosets.get_regret_strategy(ikey, probs);
    size_t num_actions = ikey.get_num_actions();
    action_deltas.assign(num_actions, 0.0);

    double node_util = 0.0;

    for (size_t i = 0; i < num_actions; i++) {

        at.apply_action(i);
        double action_util = traverse_helper(player, at, cur_dealer);
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

void CFR::traverse(int player){
    dealer.deal_and_update_equities(rng);
    action_tree.restart();

    traverse_helper(player, action_tree, dealer);
}

void CFR::train(int num_iterations, int starting_iter) {

    for (int i = starting_iter; i < starting_iter + num_iterations; ++i) {
        traverse(0);
        traverse(1);
        if (i % iters_per_discount == 0 && i != 0) infosets.discount(i);
    }
}