#include "pipeline_config.h"
#include "dataloader.h"
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
using namespace std;

void get_flop_multiset(const array<uint8_t, 5>& cards, const vector<int>& assignments,
    hand_indexer_t& turn_indexer, array<bool, 52>& missing, vector<int>& multiset) {

    const int deck_size = 52;
    multiset.assign(deck_size - cards.size(), 0);

    // sim_turn must stay uint8_t for the hand indexer
    array<uint8_t, 6> sim_turn;
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
        throw runtime_error("write path already exists: " + cfg.art.flop_multisets.string());

    auto [assignments, header] = load_matrix_and_header<int>(cfg.art.turn_assignments.string());

    array<uint8_t, 2> turn_cpr = {2, 4};
    Indexer turn_indexer(turn_cpr.size(), turn_cpr.data());

    array<uint8_t, 2> flop_cpr = {2, 3};
    Indexer flop_indexer(flop_cpr.size(), flop_cpr.data());

    const uint64_t total_flops = static_cast<uint64_t>(hand_indexer_size(&flop_indexer.h, 1));
    const uint64_t multiset_size = 47;  // one turn card per remaining card in the deck

    ofstream out(cfg.art.flop_multisets, ios::binary);
    if (!out) throw runtime_error("cant open the path: " + cfg.art.flop_multisets.string());

    DataHeader flop_header{total_flops, multiset_size, sizeof(int)};
    out.write(reinterpret_cast<const char*>(&flop_header), sizeof(flop_header));

    array<bool, 52> missing;
    vector<int> multiset(multiset_size);
    array<uint8_t, 5> cards;

    for (uint64_t i = 0; i < total_flops; ++i) {
        hand_unindex(&flop_indexer.h, 1, i, cards.data());
        get_flop_multiset(cards, assignments, turn_indexer.h, missing, multiset);
        out.write(reinterpret_cast<const char*>(multiset.data()), multiset.size() * sizeof(int));
    }

    out.flush();
    if (!out) throw runtime_error("write failed: " + cfg.art.flop_multisets.string());
}

vector<int> get_dist_matrix(const vector<int>& centers, size_t vector_dim, size_t num_centers) {
    vector<int> dist_matrix(num_centers * num_centers);

    for (size_t i = 0; i < num_centers; ++i) {
        size_t c_i0_idx = vector_dim * i;
        dist_matrix[num_centers * i + i] = 0;

        for (size_t j = 0; j < i; ++j) {
            size_t c_j0_idx = vector_dim * j;

            span<const int> ctr_i_span(&centers[c_i0_idx], vector_dim);
            span<const int> ctr_j_span(&centers[c_j0_idx], vector_dim);

            int d = L1::L1_dist(ctr_i_span, ctr_j_span);
            dist_matrix[num_centers * i + j] = d;
            dist_matrix[num_centers * j + i] = d;
        }
    }
    return dist_matrix;
}

void run_turn_distance_matrix(const PipelineConfig& cfg) {
    if (fs::exists(cfg.art.turn_distance_matrix))
        throw runtime_error("write path already exists: " + cfg.art.turn_distance_matrix.string());

    auto [centers, center_header] = load_matrix_and_header<int>(cfg.art.turn_cdf_centers.string());
    size_t num_centers = static_cast<size_t>(center_header.num_rows);
    size_t num_buckets = static_cast<size_t>(center_header.num_cols);

    vector<int> distance_matrix = get_dist_matrix(centers, num_buckets, num_centers);
    DataHeader dist_matrix_header{num_centers, num_centers, sizeof(int)};
    write_matrix_and_header<int>(cfg.art.turn_distance_matrix.string(), dist_matrix_header, distance_matrix);
}