#pragma once
#include <string>
#include <algorithm>
#include <random>
#include <limits>
#include <vector>
#include <span>
#include <stdexcept>

/**
 * @file emd_k_means.h
 * @brief Approximate Earth Movers Distance (EMD) version of k-means clustering for
 * pdfs encoded multisets over fully connected weighted graphs. This is an implementation of algorithm two of the paper "Potential-Aware Imperfect-Recall
 * Abstraction with Earth Mover’s Distance in Imperfect-Information Games"
 * which can be found here: https://www.cs.cmu.edu/~sandholm/potential-aware_imperfect-recall.aaai14.multiset
 * I recomend reading the paper before the code.
 * 
 * @note This code uses non-standard encodings of probabily distributions.
 * Point encoding: each point is a distribution over the graph's vertices encoded as a multiset
 * We limit ourselves to distributions which can be represented empirically as the outcomes of `multiset_size` draws from the vertex
 * set. A point is a vector x of length `multiset_size` with x[i] the vertex drawn
 * on the i-th draw, so vertex v carries probability (number of entries of x equal to v) / multiset_size.
 * 
 * Center encoding: centers are also distributions over the graph's vertices. 
 * @warning Centers are stored differently from points.
 * A center is stored as a pair of parallel vectors (wts, verts), which tell
 * us that the center places mass wts[i] on vertex verts[i], and zero on vertices absent from verts.
 */
 

namespace emd{

/// @brief Sparse center representation as described above.
struct Center{
    std::vector<float>wts;
    std::vector<int> verts;
};

/// @brief A buffer used to store information about a given center used in approx_EMD.
/// Passed into the approximate EMD distance function and must be reconfigured before using a new center
/// See paper for definitions of "sorted_distances" and "ordered_clusters"
struct EMDCache{
    std::vector<float> sorted_distances; 
    std::vector<int> ordered_clusters;
};

/// @brief A buffer used to store vectors used in the approx_EMD. 
/// See paper for definitions of targets/mean_remaining/done.
struct EMDScratch{
    std::vector<float> targets;
    std::vector<float> mean_remaining;
    std::vector<bool> done;
};

/// @brief A struct used to hold a ton of vectors used throughout the clustering process.
struct ClusterBuffer{
    std::vector<float> min_dists; // min_dist[i] = distance from i^th multiset to closest center

    // Sparse multiset representations, grouped by cluster and ordered by
    // point index within each cluster.
    std::vector<int> grouped;

    std::vector<int> assignments; // assignments[i] gives the cluster to which the i^th point is assigned
    std::vector<int> prev_assignments;

    std::vector<size_t> counts; // counts[i] = number of points assigned to i^th cluster
    std::vector<Center> centers; // centers[i] = center of i^th cluster
};

/// @brief Set of params which define the behavior of the clustering algorithm.
struct Params{
    size_t num_clusters;
    size_t num_verts; // number of vertices in the graph

    // center_support is a parameter controling center_quantization.
    // We quantize each center to have a supports of size = center_support
    size_t center_support;

    size_t multiset_size; // The size of the multisets
    size_t num_multisets; // number of multisets we are clustering

    // weight_matrix[i,j] = weight of edge from vertex i to vertex j. 
    // weight_matrix is assumed to be symmetric and is flattened in row-major format
    // ie weight_matrix[i,j] = weight_matrix[i*num_verts + j]
    std::vector<float> weight_matrix; 

    size_t max_iters;
    mutable std::mt19937 rng;
};

/// @brief Configures emd cache values for the given cente
void fill_emd_cache(const Params& params, const Center& ctr, EMDCache& emd_cache);

/// @brief Approximately computes and returns EMD distance between center and multiset
float approx_EMD(const Params& params, const Center& ctr, std::span<const int> multiset, const EMDCache& emd_cache, EMDScratch& emd_scratch);

/// @brief Quantizes a dense, unnormalized multiset over the vertices into a Center.
/// then takes the params.center_support vertices with the largest counts and normalizes
/// their counts to sum to 1 and writes this result into ctr.verts and ctr.wts.
/// @param dense_rep Dense vector of length params.num_verts, where dense_rep[v] is the count of vertex v. 
/// @warning undefined stuff happens if params.center_support > dense_rep.size().
void clipped_dense_center(const Params& params, Center& ctr, const std::vector<int>& dense_rep);


/// @brief Updates assignment and counts
/// Writes new assignments into the c_buff.assingments vector and new counts into c_buff.counts
void update_assignments_and_counts(const Params& params, ClusterBuffer& c_buff,
 const std::vector<int>& multisets, EMDCache& emd_cache);


/// @brief Writes the data from points into c_buff.grouped
/// sorted by cluster, then by point index within each cluster
void update_grouped(const Params& params, ClusterBuffer& c_buff, const std::vector<int>& multisets);



/// @brief Given updated `c_buff.grouped`, writes the new centers into `c_buff.centers`.
/// Each center is computed by embedding the cluster's points into R^N, where N
/// is the number of vertices and coordinate v holds the probability assigned to
/// vertex v. The center is the coordinate-wise mean of the cluster's points in
/// this embedding, truncated to its `params.center_support` largest coordinates
/// and renormalized to a distribution and storing it in our center representation
/// @note This is NOT computing the EMD centroid of the multisets. But its close enough that we still 
/// get decent clustering ! 
/// @note Right now I re-initialize a center iff the cluster for that center is empty.
/// I should probably do something smarter.
std::vector<bool> update_centers(const Params& params, ClusterBuffer& c_buff);

/// @brief Writes the re-initialized centers into c_buff.centers, for which re_init[i] = True 
/// @warning Does NOT use the same heuristic as the intialization. It uses uniform intialization, which is not ideal.
/// This is obviously stupid and should be fixed
void reinit_centers(const Params& params, ClusterBuffer& c_buff,
     const std::vector<int>& multisets, const std::vector<bool>& reseeded);


/// @brief Randomly intializes centers for each cluster and writes this data into c_buff.centers
/// It selects intial centers via the following procedure. 
/// Throutout the description Let c_i = i^th center.
/// Select c_0 uniformly from the set of points
/// For i > 0: select c_i from a distribution W on the set of pts, where
///  W(p) is proportional to the square of the EMD between p and the closest existing center
void init_centers(const Params& params, ClusterBuffer& c_buff, 
    const std::vector<int>&multisets, EMDCache& emd_cache);


/// @brief Runs one step of the clustering algorithm.
/// It computes the new cluster assignment and cluster sizes for the current centers. 
/// It updates c_buff.grouped with this information, 
/// It computes the new centers for each cluster and re-initializes any points that might need it
/// It checks if the algorithm has converged and updates the prev_assignments
/// @return changed, true iff at least one point was moved to a different cluster 
bool clustering_step(const Params& params, ClusterBuffer& c_buff, const std::vector<int>& multisets, EMDCache& emd_cache);

/// @brief Runs an approximately EMD k means style clustering algorithm on multisets over vertices in finite graphs
/// @param params Encodes the settings for the quantization algorithm
/// @param multisets Sparsely encoded multisets we wish to cluster
///@throw Runtime error if multisets.size() != params.num_mutlisets*params.multiset_size
/// @return {assignments, centers}
/// assignments[i] is the cluster to which the i^th point is assigned
/// centers - Flattened array of the "params.num_clusters" centroids of each cluster
std::pair<std::vector<int>, std::vector<Center>> emd_k_means(const Params& params, const std::vector<int>& multisets);
   
}