#pragma once
#include <cstdint>  
#include <cstddef> 
#include <filesystem>  
#include <toml.hpp>

struct Artifacts {
    std::filesystem::path river_strengths;
    std::filesystem::path river_centers;
    std::filesystem::path river_assignments;

    std::filesystem::path turn_cdfs;
    std::filesystem::path turn_cdf_centers;
    std::filesystem::path turn_assignments;
    std::filesystem::path turn_distance_matrix;
    std::filesystem::path flop_multisets;
    std::filesystem::path flop_ctrs_wts;
    std::filesystem::path flop_ctrs_verts;
    std::filesystem::path flop_assignments;
    std::filesystem::path flop_ev_sdev;
};

struct ClusteringConfig {
    Artifacts art;

    size_t river_clusters;
    size_t river_max_iters;

    size_t turn_buckets;
    size_t turn_clusters;
    size_t turn_max_iters;

    size_t flop_clusters;
    size_t flop_max_iters;
    size_t flop_center_support;

    uint32_t seed;
};

ClusteringConfig load_config(const std::filesystem::path& cfg_path, const std::filesystem::path& root);

void run_river_strengths(const ClusteringConfig& cfg);
void run_river_clusters(const ClusteringConfig& cfg);

void run_turn_cdfs(const ClusteringConfig& cfg);
void run_turn_clusters(const ClusteringConfig& cfg);
void run_turn_distance_matrix(const ClusteringConfig& cfg);

void run_flop_multisets(const ClusteringConfig& cfg);
void run_flop_clusters(const ClusteringConfig& cfg);
void run_flop_ev_sdev(const ClusteringConfig& cfg);