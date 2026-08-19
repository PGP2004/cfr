#include <chrono>
#include <filesystem>
#include <iostream>
#include "training.h"

namespace fs = std::filesystem;
using steady = std::chrono::steady_clock;

int main(int , char** argv) {
    fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path();

    fs::path cfr_path = root / "configs/cfr.toml";
    fs::path run_path = root / "configs/train.toml";
    fs::path report_path = root / "configs/report.toml";

    CFRSpec spec = load_cfr_config(cfr_path, root); 
    ReportParams report = load_report_config(report_path, root);
    TrainParams train = load_train_config(run_path);
    CFR cfr = load_spec(std::move(spec));

    steady::time_point start = steady::now();
    cfr.train(train.train_iters, train.iters_per_discount, train.num_threads,
        train.omp_chunk_sz, train.base_seed);
    steady::time_point finish = steady::now();

    std::cout << "Trained in "
         << std::chrono::duration<double>(finish - start).count()
         << " seconds\n";

    generate_report(report, cfr);
    return 0;
}