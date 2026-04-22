/**
 * @file PaperAlgorithmHelpers.h
 * @brief Helper functions for a faithful implementation of the BasicSpanner algorithm.
 */

#ifndef PAPERALGORITHMHELPERS_H
#define PAPERALGORITHMHELPERS_H

#include <vector>
#include <set>
#include <map>
#include <random>
#include "PathManager.h"
#include "Graph.h"

namespace PaperAlgorithmHelpers {

std::set<int> get_seed_n_units(int n, const PathManager& pm, const std::set<int>& seeds);

std::set<int> get_basic_seed_n_units(int n, const PathManager& pm, const std::set<int>& seeds, const std::set<int>& candidates);

/**
 * @brief Check whether a candidate node is globally essential for a specific seed pair
 * 
 * @param candidate_node The node to check
 * @param seedA First seed node ID
 * @param seedB Second seed node ID
 * @param pm PathManager containing the active paths
 * @return true if the node is present in all active paths for this pair
 */
bool is_globally_essential_for_pair(int candidate_node, int seedA, int seedB, const PathManager& pm);

int eliminate_paths_at_level(int n, PathManager& pm, const std::set<int>& global_essential_nodes, const std::map<int, int>& importance_map, const std::vector<std::pair<int, int>>& seed_pairs, const Graph& graph, std::mt19937& rng);

}
#endif