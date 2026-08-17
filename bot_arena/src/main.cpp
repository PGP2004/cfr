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

    fs::path path = root / "configs" / "100M_cfr.toml";
    CFRSpec spec = load_cfr_config(path, root);
    CFR cfr = load_spec(spec);

    const ActionTree& tree = cfr.get_action_tree();
    PokerState init_state{spec.starting_stack, 
        spec.big_blind, spec.small_blind};

    Dealer dealer{};
    std::mt19937 rng;
    PokerTable table(tree, dealer, rng);
    Logger log{};
    table.play_bot(init_state, cfr, log, 10, 13);

    return 0;
}