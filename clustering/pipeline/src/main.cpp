#include "pipeline.h"
#include <filesystem>
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;
using StageFunc = void(*)(const PipelineConfig&);

void run_stage(StageFunc stage_func, PipelineConfig& cfg, std::string stage_name){
    steady::time_point start_time = steady::now();
    stage_func(cfg);
    steady::time_point finish_time = steady::now();
    std::cout << stage_name <<
        " Took " <<
        std::chrono::duration<double>(finish_time - start_time).count() <<
        " seconds" <<
        std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

int run_pipeline(char** argv) {
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path storage = exe.parent_path().parent_path().parent_path() / "storage";

    Artifacts artifacts{
        .river_strengths = storage/"river_strengths",

        .turn_cdfs = storage/"turn_cdfs",
        .turn_cdf_centers = storage/"turn_cdf_centers",
        .turn_assignments = storage/"turn_assignments",
        .turn_distance_matrix = storage/"turn_distance_matrix",

        .flop_multisets = storage/"flop_multisets",
        .flop_ctrs_wts = storage/"flop_ctrs_wts",
        .flop_ctrs_verts = storage/"flop_ctrs_verts",
        .flop_assignments = storage/"flop_assignments",
        .flop_ev_sdev = storage/"flop_ev_sdev"
    };

    PipelineConfig cfg{
        .art = artifacts,

        .turn_buckets = 20,
        .turn_clusters = 50,
        .turn_max_iters = 1000,

        .flop_clusters = 50,
        .flop_max_iters = 40,
        .flop_center_support = 47,
        .seed = 42
    };

    run_stage(run_river_strengths, cfg, "Generating River Strengths");
    run_stage(run_turn_cdfs, cfg, "Generating Turn CDFs");
    run_stage(run_turn_clusters, cfg, "Clustering Turn");
    run_stage(run_turn_distance_matrix, cfg, "Generating Turn Distance Matirx");
    run_stage(run_flop_multisets, cfg, "Generating Flop Multisets");
    run_stage(run_flop_ev_sdev, cfg, "Generating Flop EV and Std Dev");
    run_stage(run_flop_clusters, cfg, "Clustering Flop");
    return 0;
}


int main(int, char** argv) {
    try {
        return run_pipeline(argv);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}