#include <array>
#include "dataloader.h"
#include "emd_k_means.h"
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <random>

namespace fs = std::filesystem;
using namespace std;

void write_flop_assignment_and_centers(const string& sparse_reps_path, const string& dist_matrix_path, 
    const string& ctrs_wts_path, const string& ctrs_atoms_path, const string& assignment_path,  mt19937& rng) {

    auto [sparse_reps, sparse_rep_header] = load_matrix_and_header<uint16_t>(sparse_reps_path);
    auto [dist_matrix, dist_matrix_header] = load_matrix_and_header<int>(dist_matrix_path);

    size_t num_atoms = static_cast<size_t>(dist_matrix_header.num_rows);
    size_t flop_size = static_cast<size_t>(sparse_rep_header.num_cols);
    size_t num_flops = static_cast<size_t>(sparse_rep_header.num_rows);

    //TODO This should prolly be a param but it clutters up a lot
    size_t atoms_per_center = 47;
    size_t max_iters = 40;
    size_t num_centers = 50;

    vector<float> f_dist_matrix(dist_matrix.begin(), dist_matrix.end());
    emd::Params params{num_centers, num_atoms, atoms_per_center, 
        flop_size, num_flops, f_dist_matrix, max_iters, rng};
    
    auto [ctrs, assignments] = emd::emd_k_means(params, sparse_reps);

    size_t num_ctr_elts = ctrs.size()*ctrs[0].atom_wts.size();
    vector<float> wts; vector<uint16_t> atoms;
    wts.reserve(num_ctr_elts); atoms.reserve(num_ctr_elts);

    for (const emd::Center& ctr : ctrs){
        for (size_t i = 0; i < ctr.atoms.size(); ++i) {
            wts.push_back(ctr.atom_wts[i]);
            atoms.push_back(ctr.atoms[i]);
        }
    }

    DataHeader wts_header{num_centers, atoms_per_center, sizeof(float)};
    write_matrix_and_header<float>(ctrs_wts_path, wts_header, wts);

    DataHeader atoms_header{num_centers, atoms_per_center, sizeof(uint16_t)};
    write_matrix_and_header<uint16_t>(ctrs_atoms_path, atoms_header, atoms);
    
    DataHeader assignment_header{assignments.size(), 1, sizeof(uint16_t)};
    write_matrix_and_header<uint16_t>(assignment_path, assignment_header, assignments);
}

int main(int, char** argv) {
    cout << "Started" << endl;
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path();                                   
    fs::path storage = root / "storage";

    std::mt19937 rng{42};

    write_flop_assignment_and_centers((storage / "sparse_flop_pdfs").string(),
        (storage / "turn_distance_matrix").string(),
        (storage / "flop_ctrs_wts").string(),
        (storage / "flop_ctrs_atoms").string(),
        (storage / "flop_assignments").string(), rng);

    cout << "Finished" << endl;
}
