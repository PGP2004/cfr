#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <filesystem> 

#include "card_buckets.h"
#include "cfr.h"
#include "info_sets.h"

namespace fs = std::filesystem;

struct TrainParams {            
    int train_iters;
    int iters_per_discount;
};

struct LogParams{
    std::optional<fs::path> preflop_path;
    std::optional<ISetsPaths> isets_paths;
};

struct CFRSpec{
    std::optional<ISetsPaths> isets_paths;
    BucketPaths bucket_paths;
    std::vector<std::vector<float>> bet_sizes;
};

CFR load_spec(CFRSpec spec);
void write_preflop_csv(const std::string& path, const CFR& cfr);
void run_training(const CFRSpec& spec, const TrainParams& tp, const LogParams& lp);