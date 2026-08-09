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

void run_river_clusters(const PipelineConfig& cfg) {
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