#pragma once
#include <cstdint>  
#include <cstddef> 
#include <filesystem>  
// stages.h
struct Artifacts {
    std::filesystem::path river_strengths;
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

struct PipelineConfig {
    Artifacts art;

    size_t turn_buckets;
    size_t turn_clusters;
    size_t turn_max_iters;

    size_t flop_clusters;
    size_t flop_max_iters;
    size_t flop_center_support;

    uint32_t seed;
};

void run_river_strengths(const PipelineConfig& cfg);
void run_turn_cdfs(const PipelineConfig& cfg);
void run_turn_clusters(const PipelineConfig& cfg);
void run_turn_distance_matrix(const PipelineConfig& cfg);
void run_flop_multisets(const PipelineConfig& cfg);
void run_flop_clusters(const PipelineConfig& cfg);
void run_flop_ev_sdev(const PipelineConfig& cfg);