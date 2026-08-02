/**
 * @file L1_k_means.h
 * @brief L1 version of k-means clustering.
 * 
 * @note Low hanging fruit: Add some level of parallelization.
 */

#pragma once
#include<vector>
#include<random>
#include <span> 
#include <stdexcept>
#include <cstdlib>
#include <utility>
#include <cstddef>

namespace L1{

/// @brief L1 distance between two equal-length spans of ints.
/// @warning Does not protect against overflow.
/// @throws std::runtime_error if sizes differ.
inline int L1_dist(std::span<const int> a, std::span<const int> b){
    if (a.size() != b.size()) throw std::runtime_error("Dimensions dont match in L1 dist");
    int sum = 0;
    for (size_t i = 0; i < a.size(); ++i)
        sum += std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i]));
    return sum;
}

/// @brief Contains a series of vectors reused throughout the clustering algorithm.
struct ClusterBuffer{
   
    std::vector<size_t> counts; //counts[i] = number of points assigned to cluster i.
    std::vector<int> grouped; // Flattened array of points sorted by cluster, then by point index within each cluster

    std::vector<int> assignments; // assignments[i] = cluster to which i^th point belongs
    std::vector<int> prev_assignments; //Same layout as assignments but for prev iteration. 

    std::vector<int> centers; //Flattened array of centers
};

 /// @brief parameters that define behavior of the clustering algorithm
struct ClusteringParams{

    size_t num_clusters; //number of clusters the algorithm will form
    size_t num_pts; //number of points being clustered
    size_t dim; // dimension of points being clustered
    size_t max_iters; //maximum number of steps the algorihm can run for
    mutable std::mt19937 rng; 
};

/// @brief Randomly intializes centers for each cluster and writes this data into c_buff.centers
/// It selects intial centers via the following procedure. 
/// Throutout the description Let c_i = i^th center.
/// Select c_0 uniformly from the set of points
/// For i > 0: select c_i from a distribution W on the set of pts, where
///  W(p) is proportional to the square of the L1 distance between p and the closest existing center
void init_centers(const ClusteringParams& params, ClusterBuffer& c_buff, const std::vector<int>&pts);

/// @brief Updates assignment and counts
/// Writes new assignments into the c_buff.assingments vector and new counts into c_buff.counts
void update_assignments_and_counts(const ClusteringParams& params, ClusterBuffer& c_buff, const std::vector<int>& pts);

/// @brief  Writes the data from points into c_buff.grouped
/// sorted by cluster, then by point index within each cluster
void update_grouped(const ClusteringParams& params, ClusterBuffer& c_buff, const std::vector<int>& pts);

/// @brief Given updated values for grouped, writes the new centers into c_buff.centers
/// The i^th center is given by the L1 centroid of the i^th cluster of points
/// @return re_init, where re_init[i] = True iff the i^th center needs to be re-initialized
/// @note Right now I re-initialize a center iff the cluster for that center is empty.
/// I should probably do something smarter, like I should have some param and if its sufficiently small I re-init
std::vector<bool> update_centers(const ClusteringParams& params, ClusterBuffer& c_buff);

/// @brief Writes the re-initialized centers into c_buff.centers, for which re_init[i] = True 
/// @warning Does NOT use the same heuristic as the intialization. It uses uniform intialization, which is not ideal.
/// This is obviously stupid and should be fixed
void reinit_centers(const ClusteringParams& params,ClusterBuffer& c_buff, const std::vector<int>& pts, const std::vector<bool>& re_init); 
                    
/// @brief Runs one step of the clustering algorithm.
/// It computes the new cluster assignment and cluster sizes for the current centers. 
/// It updates c_buff.grouped with this information, 
/// It computes the new centers for each cluster and re-initializes any points that might need it
/// It checks if the algorithm has converged and updates the prev_assignments
/// @return changed, true iff at least one point was moved to a different cluster 
bool clustering_step(const ClusteringParams& params, ClusterBuffer& c_buff, const std::vector<int>& pts);

/// @brief Runs L1 k-means on "pts" until convergence or max iterations.
/// @param params  number of clusters, dimension of vectors, number of points, and maximum number of iterations
/// @param pts: Flattened array of "params.num_pts" vectors of dimension "params.dim"
/// Given points [x_0, x_1, ...] with x_i[j] the j-th entry of the i-th vector:
///  pts = [x_0[0], ..., x_0[dim-1], x_1[0], ..., x_1[dim-1], ...]
/// @return {assignments, centroids}, assignments[i] is the cluster to which the i^th point is assigned
/// centroids : Flattened array of the "params.num_clusters" centroids of each cluster
///@throw Runtime error if pts.size() != params.num_pts*params.dim
std::pair<std::vector<int>,std::vector<int>> l1_k_means(const ClusteringParams& params, const std::vector<int>& pts);

}