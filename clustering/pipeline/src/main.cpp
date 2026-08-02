#include "pipeline.h"
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

int main(int, char** argv) {
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path storage = exe.parent_path().parent_path().parent_path() / "storage";

    Artifacts artifacts{
        .river_strengths = storage / "river_strengths",
        .turn_cdfs = storage / "turn_cdfs",
        .turn_cdf_centers = storage / "turn_cdf_centers",
        .turn_assignments = storage / "turn_assignments",
        .turn_distance_matrix = storage / "turn_distance_matrix",
        .flop_multisets = storage / "flop_multisets",
        .flop_ctrs_wts = storage / "flop_ctrs_wts",
        .flop_ctrs_verts = storage / "flop_ctrs_verts",
        .flop_assignments = storage / "flop_assignments",
        .flop_ev_sdev = storage / "flop_ev_sdev"
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

    run_river_strengths(cfg);
    std::cout << "finished river_strengths" << std::endl;

    run_turn_cdfs(cfg);
    std::cout << "finished turn_cdfs" << std::endl;

    run_turn_clusters(cfg); 
    std::cout << "finished turn_clusters" << std::endl;

    run_turn_distance_matrix(cfg); 
    std::cout << "finished turn_distance_matrix" << std::endl;

    run_flop_multisets(cfg);
    std::cout << "finished flop_multisets" << std::endl;

    run_flop_clusters(cfg); 
    std::cout << "finished flop_clusters" << std::endl;

    run_flop_ev_sdev(cfg);
    std::cout << "finished flop_ev_sdev" << std::endl;
}