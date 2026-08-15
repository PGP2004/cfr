#include "clustering_config.h"
#include "matrix_loader.h"
#include "L1_k_means.h"
#include "evaluator.h"
#include "indexer.h"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

int get_strength(std::array<uint8_t, 7>& board) {

    uint32_t board_strength = evaluate(board);
    int deck_size = 52;
    int shared_cards = 5;

    std::array<uint8_t, 7> opp_board;
    for (uint8_t i = 0; i < shared_cards; i++) {
        opp_board[i] = board[i + 2];
    }

    std::vector<bool> missing(deck_size);
    for (int card : board) missing[card] = true;

    double score = 0.0;
    double num_opps = 0;
    for (int c1 = 0; c1 < deck_size; ++c1) {
        if (missing[c1]) continue;

        for (int c2 = c1 + 1; c2 < deck_size; ++c2) {
            if (missing[c2]) continue;
            opp_board[shared_cards] = c1;
            opp_board[shared_cards + 1] = c2;
            uint32_t opp_strength = evaluate(opp_board);
            num_opps++;

            if (board_strength > opp_strength) score += 1.0;
            else if (board_strength == opp_strength) score += 0.5;
        }
    }

    double win_rate = score / num_opps;
    uint8_t strength = static_cast<int>(100*win_rate);
    return strength;
}

void run_river_strengths(const ClusteringConfig& cfg) {
    if (fs::exists(cfg.art.river_strengths)){
        throw std::runtime_error("write path already exists: " + cfg.art.river_strengths.string());
    }

    std::array<uint8_t, 2> cards_per_round = {2, 5};
    Indexer indexer(cards_per_round.size(), cards_per_round.data());

    const uint32_t round = 1;
    const hand_index_t total = hand_indexer_size(&indexer.h, round);

    std::ofstream out(cfg.art.river_strengths, std::ios::binary);
    if (!out) throw std::runtime_error("cant open the write path: " + cfg.art.river_strengths.string());

    MatrixHeader header{
        .num_rows = static_cast<uint64_t>(total),
        .num_cols = 1,
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false
    };
    
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    std::array<uint8_t, 7> cards;
    for (hand_index_t i = 0; i < total; ++i) {
        hand_unindex(&indexer.h, round, i, cards.data());
        int strength = get_strength(cards);
        out.write(reinterpret_cast<const char*>(&strength), sizeof(strength));
    }

    if (!out) throw std::runtime_error("write failed partway through: " + cfg.art.river_strengths.string());
}


void run_river_clusters(const ClusteringConfig& cfg) {
    if (fs::exists(cfg.art.river_centers))
        throw std::runtime_error("write path already exists: " + cfg.art.river_centers.string());
    if (fs::exists(cfg.art.river_assignments))
        throw std::runtime_error("write path already exists: " + cfg.art.river_assignments.string());
    
    auto [strengths, strength_header] = load_matrix_and_header<int>(cfg.art.river_strengths.string());

    L1::ClusteringParams params{
        .num_clusters = cfg.river_clusters,
        .num_pts = static_cast<size_t>(strength_header.num_rows),
        .dim = 1,
        .max_iters = cfg.river_max_iters,
        .rng = std::mt19937{cfg.seed},
    };

    auto [assignments, centers] = L1::l1_k_means(params, strengths);

    MatrixHeader center_header{
        .num_rows = cfg.river_clusters, 
        .num_cols = 1,
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false
    };

    write_matrix_and_header<int>(cfg.art.river_centers.string(), center_header, centers);

    MatrixHeader assignment_header{
        .num_rows = static_cast<uint64_t>(assignments.size()),
        .num_cols = 1, 
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false,
    };

    write_matrix_and_header<int>(cfg.art.river_assignments.string(), assignment_header, assignments);
}