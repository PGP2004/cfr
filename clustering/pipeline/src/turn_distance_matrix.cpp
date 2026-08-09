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

void run_turn_distance_matrix(const PipelineConfig& cfg) {
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