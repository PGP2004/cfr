#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "cfr.h"
#include "game_state.h"
#include "info_sets.h"
#include "action_tree.h"

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;



int run_training(char** argv){
    fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path().parent_path();
    fs::path storage = root / "clustering/storage";
    CardBuckets abs((storage / "flop_assignments").string(),
        (storage / "turn_assignments").string(),
        (storage / "river_strengths").string());

    GameState init_state;

    std::vector<float> bet_sizes = {0.5, 1.0, 3.0};
    ActionTree at(init_state, bet_sizes);
    CFR cfr(abs, at);
    int iters = 500000;
    cfr.train(iters, 0);


    std::string run_path = (root / "training/checkpoints/run_001/").string() ;
    std::cout << "Do we get to here?" << std::endl;


    CheckPoint ck_pt{
        .regret_path = run_path + "regret_sum",
        .strategy_path = run_path + "strategy_sum",
        .offset_path = run_path + "offsets",
        .last_t_path = run_path + "last_t"
    };
    cfr.write_isets_check_point(ck_pt);
    return 0;
}

void train_and_time(char** argv){
    steady::time_point start = steady::now();
    run_training(argv);
    steady::time_point end = steady::now();

    std::cout << "Took " << std::chrono::duration<double>(end - start).count() << "s" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}


int main(int, char** argv) {
    try {
        train_and_time(argv);
    }

    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

