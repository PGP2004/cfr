#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include "training.h"

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;

int main(int , char** argv) {
    try {
        fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
        fs::path root = exe.parent_path().parent_path().parent_path().parent_path();
        fs::path cfg_path = root / "solver" / "configs" / "default.toml";

        Config cfg = load_config(cfg_path, root);

        steady::time_point start = steady::now();
        run_training(cfg.spec, cfg.train, cfg.log);
        steady::time_point finish = steady::now();

        std::cout << "Took " << std::chrono::duration<double>(finish - start).count()
            << " seconds" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}