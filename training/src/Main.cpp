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

void write_preflop_csv(const std::string& path,
        const std::unordered_map<std::string, double>& preflop) {

    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot open " + path);

    out << "hand,prob\n";
    for (const auto& [key, val] : preflop)
        out << key << ',' << val << '\n';
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

    std::vector<int> ckpt_iters = { 1'000'000, 2'000'000, 4'000'000,
    8'000'000 , 16'000'000, 32'000'000, 64'000'000, 128'000'000,
    256'000'000, 512'000'000, 1024'000'000};
    int cur_iter = 0;
    std::unordered_map<std::string, double> potbets;
    std::unordered_map<std::string, double> limps;
    Action pot_bet = {3,6};
    Action limp = {2,0};

    std::string potbet_path = (root / "training" / "preflops" /  "potbet_prob_").string();
    std::string limp_path = (root / "training" / "preflops" /  "limp_prob_").string();

    for (size_t i = 0; i < ckpt_iters.size(); ++i){

        int train_iters = ckpt_iters[i] - cur_iter;       
        cfr.train(train_iters, cur_iter);
        cur_iter = ckpt_iters[i];

        std::string potbet_title = potbet_path + std::to_string(cur_iter) + ".csv";
        potbets = cfr.preflop_probs(pot_bet);
        write_preflop_csv(potbet_title, potbets);

        std::string limp_title = limp_path + std::to_string(cur_iter) + ".csv";
        limps = cfr.preflop_probs(limp);
        write_preflop_csv(limp_title, limps);
    }
       
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

