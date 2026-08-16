#include "poker_table.h"
#include "training.h"
#include "logger.h"
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include "training.h"

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;

int main(int , char** argv) {

    fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path();

    fs::path path_1B = root / "configs" / "1B_run.toml";
    RunConfig cfg_1B = load_run_config(path_1B, root);
    CFR cfr_1B = load_spec(cfg_1B.spec);

    fs::path path_100M = root / "configs" / "100M_run.toml";
    RunConfig cfg_100M = load_run_config(path_100M, root);
    CFR cfr_100M = load_spec(cfg_100M.spec);


    const ActionTree& tree = cfr_1B.get_action_tree();
    PokerState init_state{cfg_1B.spec.starting_stack, 
        cfg_1B.spec.big_blind, cfg_1B.spec.small_blind};

    Dealer dealer{};
    std::mt19937 rng;
    PokerTable table(tree, dealer, rng);

    // Logger log{};
    // std::array<double, 2> rewards = table.play_bot(init_state, 200, cfr, log);
    std::array<double,2> rewards = table.bot_duel(10'000, {cfr_100M, cfr_1B});
    std::cout << "rewards for 100M: " << rewards[0] << std::endl;
    std::cout << "rewards for 1B: " << rewards[1] << std::endl;

    rewards = table.bot_duel(10'000, {cfr_1B, cfr_100M});
    std::cout << "rewards for 1B: " << rewards[0] << std::endl;
    std::cout << "rewards for 100M: " << rewards[1] << std::endl;
    return 0;
}