#include "pipeline.h"
#include "matrix_loader.h"
#include "indexer.h"
#include "L1_k_means.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

void get_flop_multiset(const std::array<uint8_t, 5>& cards, const std::vector<int>& assignments,
    hand_indexer_t& turn_indexer, std::array<bool, 52>& missing, std::vector<int>& multiset) {

    const int deck_size = 52;
    multiset.assign(deck_size - cards.size(), 0);

    // sim_turn must stay uint8_t for the hand indexer
    std::array<uint8_t, 6> sim_turn;
    for (size_t i = 0; i < cards.size(); ++i)
        sim_turn[i] = cards[i];

    missing.fill(false);
    for (uint8_t card : cards)
        missing[card] = true;

    int count = 0;
    for (uint8_t c1 = 0; c1 < deck_size; ++c1) {
        if (missing[c1]) continue;
        sim_turn[5] = c1;
        hand_index_t idx = hand_index_last(&turn_indexer, sim_turn.data());
        multiset[count] = assignments[idx];
        count += 1;
    }
}

void run_flop_multisets(const PipelineConfig& cfg) {
    if (fs::exists(cfg.art.flop_multisets))
        throw std::runtime_error("write path already exists: " + cfg.art.flop_multisets.string());

    auto [assignments, header] = load_matrix_and_header<int>(cfg.art.turn_assignments.string());

    std::array<uint8_t, 2> turn_cpr = {2, 4};
    Indexer turn_indexer(turn_cpr.size(), turn_cpr.data());

    std::array<uint8_t, 2> flop_cpr = {2, 3};
    Indexer flop_indexer(flop_cpr.size(), flop_cpr.data());

    const uint64_t total_flops = static_cast<uint64_t>(hand_indexer_size(&flop_indexer.h, 1));
    const uint64_t multiset_size = 47;  // one turn card per remaining card in the deck

    std::ofstream out(cfg.art.flop_multisets, std::ios::binary);
    if (!out) throw std::runtime_error("cant open the path: " + cfg.art.flop_multisets.string());

    MatrixHeader flop_header{total_flops, multiset_size, sizeof(int)};
    out.write(reinterpret_cast<const char*>(&flop_header), sizeof(flop_header));

    std::array<bool, 52> missing;
    std::vector<int> multiset(multiset_size);
    std::array<uint8_t, 5> cards;

    for (uint64_t i = 0; i < total_flops; ++i) {
        hand_unindex(&flop_indexer.h, 1, i, cards.data());
        get_flop_multiset(cards, assignments, turn_indexer.h, missing, multiset);
        out.write(reinterpret_cast<const char*>(multiset.data()), multiset.size() * sizeof(int));
    }

    out.flush();
    if (!out) throw std::runtime_error("write failed: " + cfg.art.flop_multisets.string());
}
