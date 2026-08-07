#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include "cfr.h"
#include "game_state.h"
#include "info_sets.h"
#include "action_tree.h"

namespace fs = std::filesystem;

int run_training(char** argv){
    fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path().parent_path();
    fs::path storage = root / "clustering/storage";

    CardBuckets abs((storage / "flop_assignments").string(),
        (storage / "turn_assignments").string(),
        (storage / "river_strengths").string());

    GameState init_state;
    ActionTree at(init_state);
    CFR cfr(abs, at);

    int iters = 500000;
    cfr.train(iters, 0);
    return 0;
}

int main(int, char** argv) {
    try {
        run_training(argv);
    }

    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

