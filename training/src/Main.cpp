#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>

#include "cfr.h"
#include "game_state.h"
#include "info_sets.h"
#include "action_tree.h"
#include "training.h"

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;

void run_and_time(char** argv){

    fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path().parent_path(); 

    fs::path runs_path = root / "data" / "runs";
    fs::path buckets_path = root / "data" / "clustering";

    TrainParams training_params{
        .train_iters = 1'000,
        .iters_per_discount = 10'000
    };
    
    ISetsPaths save_isets_paths{
        .regret_path = runs_path / "regret.bin",
        .strategy_path = runs_path / "strat.bin",
        .offset_path = runs_path / "offsets.bin",
        .iters_path = runs_path / "iters.bin"
    };

    LogParams log_params{
        .preflop_path = (runs_path / "test_preflop.csv"),
        .overwrite_preflop = true,
        .isets_paths = save_isets_paths,
        .overwrite_isets = true,
    };

    BucketPaths bucket_paths{
        .flop_path = buckets_path / "flop_assignments",
        .turn_path = buckets_path / "turn_assignments",
        .river_path = buckets_path / "river_assignments"
    };

    std::vector<std::vector<float>> bet_sizes = {
        {0.5f, 1.0f}, // preflop: 
        {0.33f, 0.75f, 1.5f}, // flop
        {0.75f, 1.5f}, // turn
        {0.75f, 1.25f}, // river
    };

    CFRSpec cfr_spec{
        .bucket_paths = bucket_paths,
        .bet_sizes = bet_sizes
    };

    steady::time_point start = steady::now();
    run_training(cfr_spec, training_params, log_params);
    steady::time_point finish = steady::now();

    std::cout << "Took " << std::chrono::duration<double>(finish - start).count() <<" seconds" << std::endl;
}

int main(int, char** argv) {
    try {
        run_and_time(argv);
    }

    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
