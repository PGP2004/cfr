#include "indexer.h"
#include "dataloader.h"
#include "evaluator.h"
#include "l1_k_means.h"

#include <array>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;
using namespace std;


void get_sparse_flop_pdf(const array<uint8_t, 5>& cards, const vector<uint16_t>& assignments, 
    hand_indexer_t& turn_indexer, array<bool, 52>& missing, vector<uint16_t>&pdf ) {
    //fills the cdf vector passed as arg

    const int deck_size = 52;

    pdf.resize(deck_size - cards.size());

    array<uint8_t, 6> sim_turn;
    for (size_t i = 0; i < cards.size(); ++i)
        sim_turn[i] = cards[i];

    missing.fill(false);
    for (uint8_t card : cards)
        missing[card] = true;

    int count = 0;

    for (uint8_t c1 = 0; c1 < deck_size; ++c1) {
        if (missing[c1]) continue;
        sim_turn[5] = c1;
        hand_index_t idx = hand_index_last(&turn_indexer, sim_turn.data());
        uint16_t cluster_id = assignments[idx];
        pdf[count] = cluster_id;
        count += 1;
    }
}

void write_sparse_flop_pdfs(const string& turn_assignments_path, const string& write_path) {

    if (fs::exists(write_path)) throw runtime_error("write path already exists");
    auto [assignments, header] = load_matrix_and_header<uint16_t>(turn_assignments_path);

    array<uint8_t, 2> turn_cpr = {2, 4};
    Indexer turn_indexer(turn_cpr.size(), turn_cpr.data()); 
    
    array<uint8_t, 2> flop_cpr  = {2, 3};
    Indexer flop_indexer(flop_cpr.size(), flop_cpr.data());

   const uint64_t total_flops =static_cast<uint64_t>(hand_indexer_size(&flop_indexer.h, 1));   

    ofstream out(write_path, ios::binary);
    if (!out) throw runtime_error("cant open the path:  " + write_path);

    DataHeader flop_header{total_flops, 47, sizeof(uint16_t)};
    out.write(reinterpret_cast<const char*>(&flop_header), sizeof(flop_header));

    array<bool, 52> missing;
    vector<uint16_t> pdf(47);
    array<uint8_t, 5> cards;
    
    for (uint64_t i = 0; i < total_flops; ++i) {
        hand_unindex(&flop_indexer.h, 1, i, cards.data());                           
        get_sparse_flop_pdf(cards, assignments, turn_indexer.h, missing, pdf);
        out.write(reinterpret_cast<const char*>(pdf.data()), pdf.size()*sizeof(uint16_t));
    }

    out.flush();
    if (!out) throw runtime_error("write failed: " + write_path);
}

vector<int> get_dist_matrix(const vector<uint8_t>& centers, uint8_t vector_dim, uint64_t num_centers){
    size_t num_elts = num_centers*num_centers;

    vector<int> dist_matrix(num_elts);

    for (uint64_t i = 0; i < num_centers; ++i){

        size_t c_i0_idx = vector_dim*i;
        size_t d_ii_idx = num_centers*i+i;

        dist_matrix[d_ii_idx] = 0;

        for (size_t j = 0; j < i; ++j){

            size_t c_j0_idx = vector_dim*j;
            
            size_t d_ij_idx = num_centers*i + j;
            size_t d_ji_idx = num_centers*j + i;

            span<const uint8_t>ctr_i_span(&centers[c_i0_idx], vector_dim);
            span<const uint8_t>ctr_j_span(&centers[c_j0_idx], vector_dim);

            dist_matrix[d_ij_idx] =  l1::L1_dist(ctr_i_span, ctr_j_span);
            dist_matrix[d_ji_idx] = dist_matrix[d_ij_idx];
        }
    }
   return dist_matrix;
}

void write_flop_pdfs_and_dist_matrix(const string& assignments_path, const string& centers_path,
    const string& pdfs_path, const string& dist_matrix_path){

    if (fs::exists(pdfs_path)) throw runtime_error("write path already exists");
    if (fs::exists(dist_matrix_path)) throw runtime_error("write path already exists");

    auto [centers, center_header] = load_matrix_and_header<uint8_t>(centers_path);
    uint64_t num_centers = center_header.num_rows;
    uint64_t num_buckets = center_header.num_cols;

    vector<int> distance_matrix = get_dist_matrix(centers, num_buckets, num_centers);
    DataHeader dist_matrix_header{num_centers, num_centers, sizeof(int)};
    
    write_matrix_and_header<int>(dist_matrix_path, dist_matrix_header, distance_matrix);   
    write_sparse_flop_pdfs(assignments_path, pdfs_path);
}

int main(int , char** argv) {
    cout << "Started" << endl;
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path();                
    fs::path storage = root / "storage";

    write_flop_pdfs_and_dist_matrix((storage / "turn_assignments").string(),
        (storage / "turn_cdf_centers").string(),(storage / "sparse_flop_pdfs").string(),
        (storage / "turn_distance_matrix").string());

    cout << "Finished" << endl;
}

