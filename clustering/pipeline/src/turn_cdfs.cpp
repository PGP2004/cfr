// turn_cdfs.cpp
#include "pipeline.h"
#include "matrix_loader.h"
#include "evaluator.h"
#include "indexer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

void get_strength_cdf(const std::array<uint8_t, 6>& cards, uint8_t num_buckets,
        const std::vector<int>& strengths, hand_indexer_t& river_indexer,
        std::array<bool, 52>& missing, std::vector<int>& cdf) {
    //fills the cdf vector passed as arg

    const int strength_max = 100;
    const int deck_size = 52;

    std::array<uint8_t, 7> sim_board;
    for (size_t i = 0; i < cards.size(); ++i)
        sim_board[i] = cards[i];

    missing.fill(false);
    for (uint8_t card : cards)
        missing[card] = true;

    //Note the naming is a bit confusing here: first I use the cdf to store the unnormalized pdf,
    //then later I cumsum to turn it into a (unnormalized) cdf

    fill(cdf.begin(), cdf.end(), 0);

    for (uint8_t c1 = 0; c1 < deck_size; ++c1) {
        if (missing[c1]) continue;
        sim_board[6] = c1;

        hand_index_t idx = hand_index_last(&river_indexer, sim_board.data());
        int strength = strengths[idx];

        int bucket = (strength * num_buckets) / (strength_max + 1);
        ++cdf[bucket];
    }

    for (size_t i = 1; i < cdf.size(); ++i)
        cdf[i] = cdf[i] + cdf[i - 1];
}

void run_turn_cdfs(const PipelineConfig& cfg) {
    if (fs::exists(cfg.art.turn_cdfs))
        throw std::runtime_error("write path already exists: " + cfg.art.turn_cdfs.string());

    auto [strengths, river_header] = load_matrix_and_header<int>(cfg.art.river_strengths.string());

    std::array<uint8_t, 2> river_cpr = {2, 5};
    Indexer river_indexer(river_cpr.size(), river_cpr.data());

    std::array<uint8_t, 2> turn_cpr = {2, 4};
    Indexer turn_indexer(turn_cpr.size(), turn_cpr.data());

    // total_turns is around 13 million; num_buckets should always be < 100,
    // so the product cannot overflow uint64_t.
    const hand_index_t total_turns = hand_indexer_size(&turn_indexer.h, 1);

    std::ofstream out(cfg.art.turn_cdfs, std::ios::binary);
    if (!out) throw std::runtime_error("cant open the path: " + cfg.art.turn_cdfs.string());

    MatrixHeader turn_cdf_header{static_cast<uint64_t>(total_turns), cfg.turn_buckets, sizeof(int)};
    out.write(reinterpret_cast<const char*>(&turn_cdf_header), sizeof(turn_cdf_header));

    std::array<bool, 52> missing;
    std::array<uint8_t, 6> cards;
    std::vector<int> cdf(cfg.turn_buckets);

    for (hand_index_t i = 0; i < total_turns; ++i) {
        hand_unindex(&turn_indexer.h, 1, i, cards.data());
        get_strength_cdf(cards, cfg.turn_buckets, strengths, river_indexer.h, missing, cdf);
        out.write(reinterpret_cast<const char*>(cdf.data()), cdf.size() * sizeof(int));
    }

    out.flush();
    if (!out) throw std::runtime_error("write failed: " + cfg.art.turn_cdfs.string());
}