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

//Training run ideation
//has like a set of iters to write checkpoints in
// has like paths to load the clusterings from
// has like a max number of iters
// has a way to extract the preflop range
//

// struct Trainer{

//     std::string flop_path;
//     std::string turn_path;
//     std::string river_path;
    
// };

double raise_prob(CFR cfr, std::array<std::string, 2> hand){

    
    
}



int run_training(char** argv){
    fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path().parent_path();
    fs::path storage = root / "clustering/storage";
    CardBuckets buckets((storage / "flop_assignments").string(),
        (storage / "turn_assignments").string(),
        (storage / "river_assignments").string());

    GameState init_state;
    std::vector<float> bet_sizes = {1.0};
    ActionTree at(init_state, bet_sizes);
    CFR cfr(buckets, at);
    int iters = 500000;

    steady::time_point start = steady::now();
    cfr.train(iters, 0);
    steady::time_point end = steady::now();
    std::cout << "Took " << std::chrono::duration<double>(end - start).count() << "s" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;


    std::string run_path = (root / "training/ckpts/run_0/").string() ;
    CheckPoint ck_pt{
        .regret_path = run_path + "regret_sum",
        .strategy_path = run_path + "strategy_sum",
        .offset_path = run_path + "offsets",
        .last_t_path = run_path + "last_t"
    };
    cfr.write_isets_check_point(ck_pt);

    CFR new_cfr(ck_pt, buckets, at);
    return 0;
}

void train_and_time(char** argv){
    run_training(argv);
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

