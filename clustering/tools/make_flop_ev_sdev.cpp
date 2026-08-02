#include <array>
#include "dataloader.h"
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <cmath> 

namespace fs = std::filesystem;
using namespace std;

struct Params{
    size_t num_pdfs;
    size_t num_buckets;
};

void cdfs_to_pdfs(const Params params, vector<uint8_t>& cdfs){
    //write over the cdfs to get pdfs;
    for (size_t c = 0; c < params.num_pdfs; ++c){
        for (size_t i = params.num_buckets - 1 ; i >=1; --i){
            size_t idx = params.num_buckets * c + i;
            cdfs[idx] = cdfs[idx] - cdfs[idx-1];
        }
    }
}

pair<uint8_t, uint8_t> get_ev_and_sdev(Params p, vector<uint16_t>&atom_vec, const vector<uint8_t>& pdfs, vector<float>& buff){

    buff.resize(p.num_buckets);
    for (size_t i = 0; i < p.num_buckets; ++i) buff[i] = 0.0;

    //num buckets is the number of buckets over which we chop up the [0,100] strength interval.
    //Bucket sizes are uniform.
    //the atom vec is a sparse representation over a set of pdfs, given by the pdfs vector.
    //Aggregate the distribution over turns into just a distributino over strengths

    float total = 0.0;
    for (uint16_t c : atom_vec){
        size_t ctr_idx = c*p.num_buckets;
        for (size_t i = 0; i < p.num_buckets; ++i){
            buff[i] += static_cast<float>(pdfs[ctr_idx + i]);
            total += static_cast<float>(pdfs[ctr_idx + i]);
        }
    }

    float ev = 0;
    float sdev = 0;
    //get the std_dev by first computing E(X^2) then subtracting E(X)^2 and taking sqrt root.

    for (size_t i = 0; i < p.num_buckets; ++i){
        float prob = buff[i]/total;
        float wt = (100.0/static_cast<float>(p.num_buckets))*(i+0.5);
        ev += wt*prob;
        sdev += wt*wt*prob;
    }

    sdev += -(ev*ev);
    sdev = sqrt(sdev);

    pair<uint8_t, uint8_t> output = {static_cast<uint8_t>(ev), static_cast<uint8_t>(sdev)};
    return output;
}

void write_ev_and_sdev(const string& turn_cdfs_path, const string& flop_pdfs_path,
const string& write_path) {

    auto [pdfs, pdfs_header] = load_matrix_and_header<uint8_t>(turn_cdfs_path);
    size_t num_pdfs = static_cast<size_t>(pdfs_header.num_rows);
    size_t num_buckets = static_cast<size_t>(pdfs_header.num_cols);
    Params params{num_pdfs, num_buckets};
    cdfs_to_pdfs(params, pdfs);

    cout << "loaded the cdfs" << endl;

    auto [atoms, atoms_header] = load_matrix_and_header<uint16_t>(flop_pdfs_path);
    size_t num_flops = static_cast<size_t>(atoms_header.num_rows);
    size_t atoms_per_flop = static_cast<size_t>(atoms_header.num_cols);

    vector<uint16_t> atoms_buff(atoms_per_flop, 0);
    vector<float> prob_buff(num_buckets, 0.0);

    vector<uint8_t> output(2*num_flops, 0.0);

    for (size_t i = 0; i < num_flops; ++i){
        size_t idx = atoms_per_flop*i;
        for (size_t j = 0; j < atoms_per_flop; ++j){
            atoms_buff[j] = atoms[idx+j];
        }

        auto [ev, sdev] = get_ev_and_sdev(params, atoms_buff, pdfs, prob_buff);
        size_t o_idx = 2*i;
        output[o_idx] = ev;
        output[o_idx+1] = sdev;
    }

    DataHeader output_header{num_flops, 2, sizeof(uint8_t)};
    write_matrix_and_header(write_path, output_header, output);
}

int main(int, char** argv) {
    cout << "Started" << endl;
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path();                
    fs::path storage = root / "storage";

    write_ev_and_sdev((storage / "turn_cdf_centers").string(),
        (storage / "sparse_flop_pdfs").string(), (storage / "flop_ev_sdev").string());

    cout << "Finished" << endl;
}

