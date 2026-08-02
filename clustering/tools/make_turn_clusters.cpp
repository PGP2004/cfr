#include <array>

#include "indexer.h"
#include "dataloader.h"
#include "evaluator.h"

#include "l1_k_means.h"
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <random>

namespace fs = std::filesystem;
using namespace std;

void write_assignment_and_centers(const string& center_write_path, const string& assignment_write_path,
    const string& cdf_read_path, size_t num_centers, mt19937& rng) {

    if (fs::exists(center_write_path)) throw runtime_error("write path already exists");
    if (fs::exists(assignment_write_path)) throw runtime_error("write path already exists");

    auto [cdfs, cdf_header] = load_matrix_and_header<uint8_t>(cdf_read_path);

    size_t num_pts  = cdf_header.num_rows;
    size_t num_buckets = cdf_header.num_cols;
    size_t max_iters = 1000;

    l1::Params params{num_centers, num_pts , num_buckets, max_iters, rng};
    auto [centers, assignments] = l1::l1_k_means(params, cdfs);
        
    cout << "Finished L1 k means" << endl;

    DataHeader center_header{num_centers, num_buckets, 1};
    DataHeader assignment_header{assignments.size(), 1, 2};

    write_matrix_and_header<uint8_t>(center_write_path, center_header, centers);
    write_matrix_and_header<uint16_t>(assignment_write_path, assignment_header, assignments);
    //writes centers as [c0[0], c0[1] .. c0[k],
    //                   c1[0] ,c1[1],.. c1[k], where k = CDF dimension = num_clusters
}

int main(int , char** argv) {
    cout << "Started" << endl;
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path();                       
    fs::path storage = root / "storage";

    uint8_t num_clusters = 50;
    std::mt19937 rng{42};

    write_assignment_and_centers((storage / "turn_cdf_centers").string(),
        (storage / "turn_assignments").string(), (storage / "turn_cdfs").string(),
        num_clusters, rng);

    cout << "Finished" << endl;
}