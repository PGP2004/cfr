#include "game_state.h"
#include "info_sets.h"
#include "abstraction.h"
#include "cfr.h"    
#include "vector_pool.h"

#include <memory>
#include <utility>
#include <iostream>

#include <iomanip>
#include <map>

using namespace std;

CFR::CFR(GameState init_state, Abstraction& abstraction, ActionTree& action_tree): state(std::move(init_state)),
    abs(abstraction), action_tree(action_tree), infosets(action_tree, abstraction.cluster_counts) {
    //TODO: make the params here knobs
    VectorPool::preallocate(4, 200);
    iters_per_discount = 1000;
}

InfoKey CFR::get_InfoKey(const GameState& state, const ActionTree& at) {
    int hand_id = state.get_hand_id();
    int street = state.get_street();
    int hand_cluster = abs.cluster_of(street, hand_id);  
    InfoKey ikey(at.nodes[at.cur_idx], hand_cluster);
    return ikey;
}

double CFR::traverse(int player, GameState& state, ActionTree& at) {
    
    if (state.is_terminal_node()) {
        return state.get_reward(player);
    }

    if (state.is_chance_node()) {
        
        ChanceUndo undo;
        state.write_chance_undo(undo);
        state.apply_chance(rng);
        double util = traverse(player, state, at);
        state.undo_chance(undo);
        return util;
    }
    // Player Action Branch
    int active_player = state.get_active_player();
    if (active_player != player) {

        InfoKey ikey = get_InfoKey(state, at);
        VectorPool::ProbsBuffer probs_buf;
        auto& probs = probs_buf.get();

        size_t sampled_idx = infosets.sample_regret(ikey, rng, probs);
        Action sampled_action = ikey.get_action(sampled_idx);

        infosets.update_strategy(ikey, probs);

        ActionUndo undo;
        state.write_action_undo(sampled_action, undo);

        state.apply_action(sampled_action);
        at.apply_action(sampled_idx);

        double util = traverse(player, state, at);
        
        state.undo_action(undo);
        at.undo_action();
        return util;
    }

    // Branch where active player = current player
    InfoKey ikey = get_InfoKey(state, at);
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
        ActionUndo undo;
        state.write_action_undo(action, undo);

        at.apply_action(i);
        state.apply_action(action);

        double action_util = traverse(player, state, at);
        state.undo_action(undo);
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
        traverse(0, state, action_tree);
        traverse(1, state, action_tree);

        if (i % iters_per_discount == 0 && i != 0){
            infosets.discount(i);
        }
    }

    double total = std::chrono::duration<double>(
                       std::chrono::steady_clock::now() - t0).count();

    cout << "runtime for " << num_iterations << " iterations is: " << total << endl;
}

