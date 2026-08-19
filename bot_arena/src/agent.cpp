#include "agent.h"
#include "action_tree.h"
#include "card_buckets.h"
#include "dealer.h"
#include "poker_state.h"
#include "indexer.h"

#include <stdexcept>
#include <string>
#include <random>
#include <cstdint>


Agent::Agent(ActionTree& tree, CardBuckets& buckets,
             std::vector<size_t>& offsets, std::vector<double>& strategy,
             std::mt19937& rng, RemapFn remap_fn)
    : tree(tree), node_idx(tree.root_idx), buckets(buckets),
      offsets(offsets), strategy(strategy), rng(rng), remap_fn(remap_fn) {}


size_t Agent::get_card_idx(const std::vector<uint8_t>& cards) {
    static constexpr std::array<uint8_t, 1> pre_c{2};
    static constexpr std::array<uint8_t, 2> flop_c{2, 3};
    static constexpr std::array<uint8_t, 2> turn_c{2, 4};
    static constexpr std::array<uint8_t, 2> river_c{2, 5};

    static const Indexer pre  {pre_c.size(),   pre_c.data()};
    static const Indexer flop {flop_c.size(),  flop_c.data()};
    static const Indexer turn {turn_c.size(),  turn_c.data()};
    static const Indexer river{river_c.size(), river_c.data()};

    const Indexer* idx = nullptr;
    switch (cards.size()) {
        case 2: idx = &pre;   break;
        case 5: idx = &flop;  break;
        case 6: idx = &turn;  break;
        case 7: idx = &river; break;
        default:
            throw std::runtime_error("bad card count: " + std::to_string(cards.size()));
    }

    return hand_index_last(&idx->h, cards.data());
}


Action Agent::get_action(const PokerState& state){

    const size_t n = tree.num_children(node_idx);
    if (n == 0) throw std::runtime_error("get_action called at terminal node");

    std::vector<uint8_t> cards = state.get_cards(state.active_player);
    const size_t card_idx = get_card_idx(cards);
    const size_t cluster_id = buckets.cluster_of(state.get_street(), card_idx);
    const size_t offset = get_offset(node_idx, cluster_id);

    std::uniform_real_distribution<double> unif(0.0, 1.0);
    const double r = unif(rng);

    double cum = 0.0;
    size_t chosen = n - 1;   // float sums land just under 1.0; slack goes to the last edge
    for (size_t i = 0; i < n; ++i) {
        cum += strategy[offset + i];
        if (r < cum) { chosen = i; break; }
    }

    return tree.get_action(node_idx, chosen);
}


void Agent::update_on_action(PokerState& state, const Action& observed) {
    const int edge = remap_fn(tree, state, observed, node_idx, rng);
    if (edge < 0 || static_cast<size_t>(edge) >= tree.num_children(node_idx))
        throw std::runtime_error("remap_fn returned out-of-range edge");
    node_idx = tree.nodes[node_idx].child_idxs[static_cast<size_t>(edge)];
}