// flop_clusters.cpp
#include "pipeline_config.h"
#include "dataloader.h"
#include "emd_k_means.h"

#include <cstdint>
#include <random>
#include <vector>

using namespace std;

void run_flop_clusters(const PipelineConfig& cfg) {
    auto [raw_reps, rep_header] = load_matrix_and_header<uint16_t>(cfg.art.sparse_flop_pdfs.string());
    auto [dist_matrix, dist_header] = load_matrix_and_header<int>(cfg.art.turn_distance_matrix.string());

    emd::Params params{
        .num_clusters = cfg.flop_clusters,
        .num_verts = static_cast<size_t>(dist_header.num_rows),
        .center_support = cfg.flop_center_support,
        .multiset_size = static_cast<size_t>(rep_header.num_cols),
        .num_multisets = static_cast<size_t>(rep_header.num_rows),
        .weight_matrix = vector<float>(dist_matrix.begin(), dist_matrix.end()),
        .max_iters = cfg.flop_max_iters,
        .rng = mt19937{cfg.seed},
    };

    vector<int> multisets(raw_reps.begin(), raw_reps.end());
    auto [assignments, ctrs] = emd::emd_k_means(params, multisets);

    vector<float> wts;
    vector<uint16_t> verts;
    wts.reserve(ctrs.size() * cfg.flop_center_support);
    verts.reserve(ctrs.size() * cfg.flop_center_support);

    for (const emd::Center& ctr : ctrs) {
        for (size_t i = 0; i < ctr.verts.size(); ++i) {
            wts.push_back(ctr.wts[i]);
            verts.push_back(static_cast<uint16_t>(ctr.verts[i]));
        }
    }

    DataHeader wts_header{cfg.flop_clusters, cfg.flop_center_support, sizeof(float)};
    write_matrix_and_header<float>(cfg.art.flop_ctrs_wts.string(), wts_header, wts);

    DataHeader verts_header{cfg.flop_clusters, cfg.flop_center_support, sizeof(uint16_t)};
    write_matrix_and_header<uint16_t>(cfg.art.flop_ctrs_verts.string(), verts_header, verts);

    DataHeader assignment_header{assignments.size(), 1, sizeof(int)};
    write_matrix_and_header<int>(cfg.art.flop_assignments.string(), assignment_header, assignments);
}