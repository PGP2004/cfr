#pragma once
#include <cstddef>
#include <random>
#include <vector>
#include <array>
#include <cstdint>
#include "indexer.h"
#include "action_tree.h"
#include "action.h"
#include "card_buckets.h"
#include "dealer.h"

// Remap(tree, state, action, node_idx, rng) -> edge index
using RemapFn = int(*)(const ActionTree&, PokerState&, const Action&, size_t, std::mt19937&);

struct Agent{

private:
    ActionTree& tree;
    size_t node_idx;

    CardBuckets& buckets;
    std::vector<size_t>& offsets;
    std::vector<double>& strategy;
    std::mt19937& rng;
    RemapFn remap_fn;

    static size_t get_card_idx(const std::vector<uint8_t>& cards);

    inline size_t get_offset(size_t cur_node, size_t cluster_idx) const {
        size_t num_actions = tree.num_children(cur_node);
        return offsets[cur_node] + cluster_idx * num_actions;
    }

public:
    Agent(ActionTree& tree, CardBuckets& buckets,
          std::vector<size_t>& offsets, std::vector<double>& strategy,
          std::mt19937& rng, RemapFn remap_fn);

    void reset() { node_idx = tree.root_idx; }
    size_t get_node_idx() const { return node_idx; }

    Action get_action(const PokerState& state);
    void update_on_action(PokerState& state, const Action& observed);
};