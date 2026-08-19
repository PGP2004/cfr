#include "info_sets.h"
#include "card_buckets.h"
#include "cfr.h"    

#include <iostream>
#include <omp.h>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include <iostream>

CFR::CFR(CardBuckets buckets, ActionTree at):
    card_buckets(std::move(buckets)),
    action_tree(std::move(at)),
    infosets(this->action_tree, this->card_buckets.cluster_counts){}

CFR::CFR(InfoSets isets, CardBuckets buckets, ActionTree at):
    card_buckets(std::move(buckets)),
    action_tree(std::move(at)),
    infosets(std::move(isets)){}

InfoKey CFR::get_InfoKey(size_t node_idx, const ActionTree& at, const Dealer& d) const {
    size_t num_children = at.num_children(node_idx);
    int street = at.street(node_idx);
    int hand_id = d.get_card_id(at.active_player(node_idx), street);
    return {node_idx, (size_t)card_buckets.cluster_of(street, hand_id), num_children};
}


double CFR::get_reward(const Dealer& dealer, size_t node_idx, int player){

    if (!action_tree.is_terminal(node_idx)){
        throw std::runtime_error("cannot get reward for non-terminal node");
    }

    int opp = 1 - player;

    //if someone folded in the game
    if (action_tree.is_folded(node_idx)){
        bool won = (player == action_tree.active_player(node_idx));
        if (won) return action_tree.get_payoff(node_idx , player);
        return -action_tree.get_payoff(node_idx, opp);
    }

            // if no one folded in the game.
    if (dealer.winner == -1) return 0.0;
    else if (dealer.winner == player) return action_tree.get_payoff(node_idx, player);
    else if (dealer.winner == opp) return - action_tree.get_payoff(node_idx, opp);

    throw std::runtime_error("Should not be able to get here");
    return 0.0;
}

double CFR::traverse(int player, size_t node_idx, size_t depth, ThreadBuff& buff) {

    if (action_tree.is_terminal(node_idx)) {
        return get_reward(buff.dealer, node_idx, player);
    }

    int active_player = action_tree.active_player(node_idx);

    if (active_player != player) {

        std::vector<double>&probs = buff.probs_scratch[depth];

        InfoKey ikey = get_InfoKey(node_idx, action_tree, buff.dealer);
        infosets.get_regret_strategy(ikey, probs);
        infosets.update_strategy(ikey, probs); 

        size_t action_idx = infosets.sample_action_idx(buff.rng, probs);
        size_t child_idx = action_tree.apply_action(node_idx, action_idx);
        double util = traverse(player, child_idx, depth + 1, buff);

        return util;
    }

    // Branch where active player = current player
    std::vector<double>& probs = buff.probs_scratch[depth];
    std::vector<double>& action_deltas = buff.deltas_scratch[depth];

    InfoKey ikey = get_InfoKey(node_idx, action_tree, buff.dealer);
    infosets.get_regret_strategy(ikey, probs);
    action_deltas.assign(ikey.num_actions, 0.0);
    double node_util = 0.0;

    for (size_t i = 0; i < ikey.num_actions; i++) {

        size_t child_idx = action_tree.apply_action(node_idx, i);
        double action_util = traverse(player, child_idx, depth + 1, buff);
        node_util += probs[i] * action_util;
        action_deltas[i] = action_util;
    }

    for (size_t i = 0; i < action_deltas.size(); ++i) {
        action_deltas[i] = action_deltas[i] - node_util;
    }

    infosets.update_regret(ikey, action_deltas);
    return node_util;
}

std::vector<ThreadBuff> CFR::make_thread_buffs(size_t num_threads, uint32_t base_seed){

    //This is stupid should figure out seed per thread by scrambling currents eeed
    size_t depth = action_tree.depth() + 1;
    size_t branching = action_tree.max_branching();

    std::mt19937 base_rng(base_seed);
    std::vector<ThreadBuff> output(num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
        output[i].rng.seed(base_rng());
        output[i].probs_scratch.assign(depth, std::vector<double>(branching));
        output[i].deltas_scratch.assign(depth, std::vector<double>(branching));
    }
    return output;
}

void CFR::train(size_t iters, size_t iters_per_discount, 
    size_t num_threads, size_t omp_chunk_sz, uint32_t base_seed) {

    std::vector<ThreadBuff> thread_buffs = make_thread_buffs(num_threads, base_seed);
    size_t done = 0;

    while (done < iters) {
        
        const size_t batch= std::min(iters_per_discount, iters - done);

        #pragma omp parallel num_threads(num_threads)
        {
            ThreadBuff& buff = thread_buffs[omp_get_thread_num()];

            #pragma omp for schedule(dynamic, omp_chunk_sz)
            for (size_t i = 0; i <  batch; ++i) {
                buff.dealer.deal(buff.rng);
                traverse(0, action_tree.root_idx, 0, buff);
                traverse(1, action_tree.root_idx, 0, buff);
            }
        }

        done += batch;
        infosets.cur_iter += batch;
        infosets.discount(infosets.cur_iter);   
    }
}