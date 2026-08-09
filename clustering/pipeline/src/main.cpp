#include "pipeline.h"
#include <filesystem>
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;

void print_runtime(steady::time_point start_time, steady::time_point finish_time){
    std::cout << "Took " << std::chrono::duration<double>(finish_time - start_time).count() << "s" << std::endl;
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

    steady::time_point start_time;
    steady::time_point finish_time;

    start_time = steady::now();
    run_river_strengths(cfg);
    finish_time = steady::now();
    print_runtime(start_time, finish_time);

    start_time = steady::now();
    run_turn_cdfs(cfg);
    finish_time = steady::now();
    std::cout << "finished turn_cdfs" << std::endl;
    print_runtime(start_time, finish_time);

    start_time = steady::now();
    run_turn_clusters(cfg); 
    finish_time = steady::now();
    std::cout << "finished turn_clusters" << std::endl;
    print_runtime(start_time, finish_time);

    start_time = steady::now();
    run_turn_distance_matrix(cfg); 
    finish_time = steady::now();
    std::cout << "finished turn_distance_matrix" << std::endl;
    print_runtime(start_time, finish_time);

    start_time = steady::now();
    run_flop_multisets(cfg);
    finish_time = steady::now();
    std::cout << "finished flop_multisets" << std::endl;
    print_runtime(start_time, finish_time);

    start_time = steady::now();
    run_flop_ev_sdev(cfg);
    finish_time = steady::now();
    std::cout << "finished flop_ev_sdev" << std::endl;
    print_runtime(start_time, finish_time);

    start_time = steady::now();
    run_flop_clusters(cfg); 
    finish_time = steady::now();
    std::cout << "finished flop_clusters" << std::endl;
    print_runtime(start_time, finish_time);

    return 0;
}

int main(int, char** argv) {
    try {
        return run_pipeline(argv);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}