#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include "clustering_config.h"

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;
using StageFunc = void(*)(const ClusteringConfig&);

void run_stage(StageFunc stage_func, const ClusteringConfig& cfg, const std::string& stage_name) {
    steady::time_point start_time = steady::now();
    stage_func(cfg);
    steady::time_point finish_time = steady::now();
    std::cout << stage_name << " took "
        << std::chrono::duration<double>(finish_time - start_time).count()
        << " seconds" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

int main(int, char** argv) {
    try {
        fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
        fs::path root = exe.parent_path().parent_path().parent_path().parent_path();   // repo root

        fs::path cfg_path = root / "clustering" / "configs" / "default.toml";
        ClusteringConfig cfg = load_config(cfg_path, root);

        run_stage(run_river_strengths,cfg, "Generating River Strengths");
        run_stage(run_river_clusters, cfg, "Clustering River");
        run_stage(run_turn_cdfs, cfg, "Generating Turn CDFs");
        run_stage(run_turn_clusters, cfg, "Clustering Turn");
        run_stage(run_turn_distance_matrix,cfg, "Generating Turn Distance Matrix");
        run_stage(run_flop_multisets, cfg, "Generating Flop Multisets");
        run_stage(run_flop_ev_sdev, cfg, "Generating Flop EV and Std Dev");
        run_stage(run_flop_clusters, cfg, "Clustering Flop");
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}