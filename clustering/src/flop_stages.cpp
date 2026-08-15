#include "clustering_config.h"
#include "matrix_loader.h"
#include "emd_k_means.h"
#include "indexer.h"
#include <cstdint>
#include <random>
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

void run_flop_multisets(const ClusteringConfig& cfg) {
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

    MatrixHeader flop_header{
        .num_rows = total_flops,
        .num_cols = multiset_size, 
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false
    };

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


void run_flop_clusters(const ClusteringConfig& cfg) {

    if (fs::exists(cfg.art.flop_ctrs_wts))
        throw std::runtime_error("write path already exists: " + cfg.art.flop_ctrs_wts.string());
    if (fs::exists(cfg.art.flop_ctrs_verts))
        throw std::runtime_error("write path already exists: " + cfg.art.flop_ctrs_verts.string());
    if (fs::exists(cfg.art.flop_assignments))
        throw std::runtime_error("write path already exists: " + cfg.art.flop_assignments.string());

    auto [multisets, multisets_header] = load_matrix_and_header<int>(cfg.art.flop_multisets.string());
    auto [dist_matrix, dist_header] = load_matrix_and_header<int>(cfg.art.turn_distance_matrix.string());

    emd::Params params{
        .num_clusters = cfg.flop_clusters,
        .num_verts = static_cast<size_t>(dist_header.num_rows),
        .center_support = cfg.flop_center_support,
        .multiset_size = static_cast<size_t>(multisets_header.num_cols),
        .num_multisets = static_cast<size_t>(multisets_header.num_rows),
        .weight_matrix = std::vector<float>(dist_matrix.begin(), dist_matrix.end()),
        .max_iters = cfg.flop_max_iters,
        .rng = std::mt19937{cfg.seed},
    };


    auto [assignments, ctrs] = emd::emd_k_means(params, multisets);

    std::vector<float> wts;
    std::vector<int> verts;
    wts.reserve(ctrs.size() * cfg.flop_center_support);
    verts.reserve(ctrs.size() * cfg.flop_center_support);

    for (const emd::Center& ctr : ctrs) {
        for (size_t i = 0; i < ctr.verts.size(); ++i) {
            wts.push_back(ctr.wts[i]);
            verts.push_back(static_cast<int>(ctr.verts[i]));
        }
    }

    MatrixHeader wts_header{
        .num_rows = cfg.flop_clusters, 
        .num_cols = cfg.flop_center_support, 
        .bytes_per_elt = sizeof(float),
        .is_signed = true,
        .is_float = true};

    write_matrix_and_header<float>(cfg.art.flop_ctrs_wts.string(), wts_header, wts);

    MatrixHeader verts_header{
        .num_rows = cfg.flop_clusters, 
        .num_cols = cfg.flop_center_support,
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false
        };
    write_matrix_and_header<int>(cfg.art.flop_ctrs_verts.string(), verts_header, verts);

    MatrixHeader assignment_header{
        .num_rows = assignments.size(), 
        .num_cols = 1,
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false};

    write_matrix_and_header<int>(cfg.art.flop_assignments.string(), assignment_header, assignments);
}


void cdfs_to_pdfs(size_t num_centers, size_t num_buckets, std::vector<int>& cdfs) {
    //write over the cdfs to get pdfs;
    for (size_t c = 0; c < num_centers; ++c) {
        for (size_t i = num_buckets - 1; i >= 1; --i) {
            size_t idx = num_buckets * c + i;
            cdfs[idx] = cdfs[idx] - cdfs[idx - 1];
        }
    }
}

std::pair<int, int> get_ev_and_sdev(size_t num_buckets, std::vector<int>& multiset,
                               const std::vector<int>& centers, std::vector<float>& buff) {

    buff.resize(num_buckets);
    for (size_t i = 0; i < num_buckets; ++i) buff[i] = 0.0;

    //num buckets is the number of buckets over which we chop up the [0,100] strength interval.
    //Bucket sizes are uniform.
    //the multiset is a sparse representation of a distribution over the centers
    //Aggregate the distribution over turns into just a distributino over strengths

    float total = 0.0;
    for (int c : multiset) {
        size_t ctr_idx = static_cast<size_t>(c) * num_buckets;
        for (size_t i = 0; i < num_buckets; ++i) {
            buff[i] += static_cast<float>(centers[ctr_idx + i]);
            total += static_cast<float>(centers[ctr_idx + i]);
        }
    }

    float ev = 0;
    float sdev = 0;
    //get the std_dev by first computing E(X^2) then subtracting E(X)^2 and taking sqrt root.

    for (size_t i = 0; i < num_buckets; ++i) {
        float prob = buff[i] / total;
        float wt = (100.0 / static_cast<float>(num_buckets)) * (i + 0.5);
        ev += wt * prob;
        sdev += wt * wt * prob;
    }

    sdev += -(ev * ev);
    sdev = sqrt(sdev);

    std::pair<int, int> output = {static_cast<int>(ev), static_cast<int>(sdev)};
    return output;
}

void run_flop_ev_sdev(const ClusteringConfig& cfg) {

    if (fs::exists(cfg.art.flop_ev_sdev))
        throw std::runtime_error("write path already exists: " + cfg.art.flop_ev_sdev.string());

    const size_t num_centers = cfg.turn_clusters;
    const size_t num_buckets = cfg.turn_buckets;

    auto [centers, centers_header] = load_matrix_and_header<int>(cfg.art.turn_cdf_centers.string());
    if (centers_header.num_rows != num_centers || centers_header.num_cols != num_buckets)
        throw std::runtime_error("turn_cdf_centers shape does not match config: " + centers_header.to_string());

    cdfs_to_pdfs(num_centers, num_buckets, centers);

    auto [multisets, multisets_header] = load_matrix_and_header<int>(cfg.art.flop_multisets.string());
    size_t num_flops = static_cast<size_t>(multisets_header.num_rows);
    size_t multiset_size = static_cast<size_t>(multisets_header.num_cols);

    std::vector<int> multiset_buff(multiset_size, 0);
    std::vector<float> prob_buff(num_buckets, 0.0);
    std::vector<int> output(2 * num_flops, 0);

    for (size_t i = 0; i < num_flops; ++i) {
        size_t idx = multiset_size * i;
        for (size_t j = 0; j < multiset_size; ++j) {
            multiset_buff[j] = multisets[idx + j];
        }

        auto [ev, sdev] = get_ev_and_sdev(num_buckets, multiset_buff, centers, prob_buff);
        size_t o_idx = 2 * i;
        output[o_idx] = ev;
        output[o_idx + 1] = sdev;
    }

    MatrixHeader output_header{
        .num_rows = num_flops,
        .num_cols =  2, 
        .bytes_per_elt = sizeof(int),
        .is_signed = true,
        .is_float = false};
        
    write_matrix_and_header<int>(cfg.art.flop_ev_sdev.string(), output_header, output);
}