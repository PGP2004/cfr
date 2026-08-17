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
    fs::path run_path = root / "configs" / "100M_run.toml";
    fs::path cfr_path = root / "configs" / "fresh_cfr.toml";

    CFRSpec spec = load_cfr_config(cfr_path, root);
    auto [train_params, train_log] = load_run_config(run_path, root);

    std::cout << "Got to here" << std::endl;
    steady::time_point start = steady::now();
    run_training(spec, train_params, train_log);
    steady::time_point finish = steady::now();

    std::cout << "Took " << 
        std::chrono::duration<double>(finish - start).count()
        << " seconds" << std::endl;
    return 0;
}
