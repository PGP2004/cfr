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

    PokerState init_state{spec.starting_stack, spec.big_blind, spec.small_blind};
    std::mt19937 rng;
    rng.seed(10);


    return 0;
}