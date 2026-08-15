#include <toml.hpp>
#include "clustering_config.h"
namespace fs = std::filesystem;

ClusteringConfig load_config(const fs::path& cfg_path, const fs::path& root) {
    toml::table t = toml::parse_file(cfg_path.string());
    ClusteringConfig cfg;

    cfg.river_clusters = t["params"]["river_clusters"].value<size_t>().value();
    cfg.river_max_iters = t["params"]["river_max_iters"].value<size_t>().value();

    cfg.turn_buckets = t["params"]["turn_buckets"].value<size_t>().value();
    cfg.turn_clusters = t["params"]["turn_clusters"].value<size_t>().value();
    cfg.turn_max_iters = t["params"]["turn_max_iters"].value<size_t>().value();

    cfg.flop_clusters = t["params"]["flop_clusters"].value<size_t>().value();
    cfg.flop_max_iters = t["params"]["flop_max_iters"].value<size_t>().value();
    cfg.flop_center_support = t["params"]["flop_center_support"].value<size_t>().value();

    cfg.seed = t["params"]["seed"].value<uint32_t>().value();

    cfg.art = Artifacts{
        .river_strengths = root / t["artifacts"]["river_strengths"].value<std::string>().value(),
        .river_centers = root / t["artifacts"]["river_centers"].value<std::string>().value(),
        .river_assignments = root / t["artifacts"]["river_assignments"].value<std::string>().value(),

        .turn_cdfs = root / t["artifacts"]["turn_cdfs"].value<std::string>().value(),
        .turn_cdf_centers = root / t["artifacts"]["turn_cdf_centers"].value<std::string>().value(),
        .turn_assignments = root / t["artifacts"]["turn_assignments"].value<std::string>().value(),
        .turn_distance_matrix = root / t["artifacts"]["turn_distance_matrix"].value<std::string>().value(),

        .flop_multisets = root / t["artifacts"]["flop_multisets"].value<std::string>().value(),
        .flop_ctrs_wts = root / t["artifacts"]["flop_ctrs_wts"].value<std::string>().value(),
        .flop_ctrs_verts = root / t["artifacts"]["flop_ctrs_verts"].value<std::string>().value(),
        .flop_assignments = root / t["artifacts"]["flop_assignments"].value<std::string>().value(),
        .flop_ev_sdev = root / t["artifacts"]["flop_ev_sdev"].value<std::string>().value()
    };

    return cfg;
}