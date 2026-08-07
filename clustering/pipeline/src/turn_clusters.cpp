#include "pipeline.h"
#include "matrix_loader.h"
#include "L1_k_means.h"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

void run_turn_clusters(const PipelineConfig& cfg) {
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
    std::cout << "Finished L1 k means" << std::endl;

    MatrixHeader center_header{cfg.turn_clusters, cdf_header.num_cols, sizeof(int)};
    write_matrix_and_header<int>(cfg.art.turn_cdf_centers.string(), center_header, centers);

    MatrixHeader assignment_header{assignments.size(), 1, sizeof(int)};
    write_matrix_and_header<int>(cfg.art.turn_assignments.string(), assignment_header, assignments);
}