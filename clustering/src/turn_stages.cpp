#include "clustering_config.h"
#include "L1_k_means.h"
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

void run_turn_cdfs(const ClusteringConfig& cfg) {

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

    MatrixHeader turn_cdf_header{
        .num_rows = static_cast<uint64_t>(total_turns), 
        .num_cols = cfg.turn_buckets,
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false
    };

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

void run_turn_clusters(const ClusteringConfig& cfg) {
    if (fs::exists(cfg.art.turn_cdf_centers))
        throw std::runtime_error("write path already exists: " + cfg.art.turn_cdf_centers.string());
    if (fs::exists(cfg.art.turn_assignments))
        throw std::runtime_error("write path already exists: " + cfg.art.turn_assignments.string());
    
    auto [cdfs, cdf_header] = load_matrix_and_header<int>(cfg.art.turn_cdfs.string());

    L1::ClusteringParams params{
        .num_clusters = cfg.turn_clusters,
        .num_pts = static_cast<size_t>(cdf_header.num_rows),
        .dim = static_cast<size_t>(cdf_header.num_cols),
        .max_iters = cfg.turn_max_iters,
        .rng = std::mt19937{cfg.seed},
    };

    auto [assignments, centers] = L1::l1_k_means(params, cdfs);

    MatrixHeader center_header{
        .num_rows = cfg.turn_clusters, 
        .num_cols = cdf_header.num_cols,
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false
    };

    write_matrix_and_header<int>(cfg.art.turn_cdf_centers.string(), center_header, centers);

    MatrixHeader assignment_header{
        .num_rows = assignments.size(),
        .num_cols = 1, 
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false,
    };

    write_matrix_and_header<int>(cfg.art.turn_assignments.string(), assignment_header, assignments);
}

std::vector<int> get_dist_matrix(const std::vector<int>& centers, size_t vector_dim, size_t num_centers) {
    std::vector<int> dist_matrix(num_centers * num_centers);

    for (size_t i = 0; i < num_centers; ++i) {
        size_t c_i0_idx = vector_dim * i;
        dist_matrix[num_centers * i + i] = 0;

        for (size_t j = 0; j < i; ++j) {
            size_t c_j0_idx = vector_dim * j;

            std::span<const int> ctr_i_span(&centers[c_i0_idx], vector_dim);
            std::span<const int> ctr_j_span(&centers[c_j0_idx], vector_dim);

            int d = L1::L1_dist(ctr_i_span, ctr_j_span);
            dist_matrix[num_centers * i + j] = d;
            dist_matrix[num_centers * j + i] = d;
        }
    }
    return dist_matrix;
}

void run_turn_distance_matrix(const ClusteringConfig& cfg) {
    if (fs::exists(cfg.art.turn_distance_matrix))
        throw std::runtime_error("write path already exists: " + cfg.art.turn_distance_matrix.string());

    auto [centers, center_header] = load_matrix_and_header<int>(cfg.art.turn_cdf_centers.string());
    size_t num_centers = static_cast<size_t>(center_header.num_rows);
    size_t num_buckets = static_cast<size_t>(center_header.num_cols);

    std::vector<int> distance_matrix = get_dist_matrix(centers, num_buckets, num_centers);
    MatrixHeader dist_matrix_header{
        .num_rows = num_centers, 
        .num_cols = num_centers, 
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false
    };
    
    write_matrix_and_header<int>(cfg.art.turn_distance_matrix.string(), dist_matrix_header, distance_matrix);
}
