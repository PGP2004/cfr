#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <chrono>

#include "cfr.h"
#include "game_state.h"
#include "info_sets.h"
#include "action_tree.h"
#include "trainer.h"

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;

void run_and_time(char** argv){

    fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path().parent_path(); 
   
    fs::path buckets_path = root / "data" / "clustering";
    fs::path runs_path = root / "data" / "runs";

    BucketPaths bucketing_paths{
        .flop_path = buckets_path / "flop_assignments",
        .turn_path = buckets_path / "turn_assignments",
        .river_path = buckets_path / "river_assignments"
    };
        
    std::vector<std::vector<float>> bet_sizes = {
        {0.5f, 1.0f},          // preflop: ~2.5bb open, pot-sized 3bet
        {0.33f, 0.75f, 1.5f},  // flop
        {0.75f, 1.5f},         // turn
        {0.75f, 1.25f},        // river
    };

    int k = 1'000;
    int m = 1'000'000;
    int b = 1'000'000'000;

    Trainer trainer{
        .bp =  bucketing_paths,
        .game_state = GameState{},
        .bet_sizes = bet_sizes,

        .runs_folder = runs_path ,
        .run_name = "run_001",

        .preflop_ckpts = {1*k, 10*k, 1*m, 10*m, 100*m, 250*m,
            500*m, 750*m, 1*b},
            
        .max_iters = 1*b
    };

    steady::time_point start = steady::now();
    run_training(trainer);
    steady::time_point finish = steady::now();

    std::cout << "training  ook " <<
        std::chrono::duration<double>(finish - start).count() <<
        " seconds" << std::endl;
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
