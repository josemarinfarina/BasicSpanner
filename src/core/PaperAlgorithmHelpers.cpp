/**
 * @file PaperAlgorithmHelpers.cpp
 * @brief Helper functions for the BasicSpanner algorithm.
 */

#include "PaperAlgorithmHelpers.h"
#include "PathManager.h"
#include "Graph.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <set>
#include <vector>
#include <map>

namespace PaperAlgorithmHelpers {

std::set<int> get_seed_n_units(int n, const PathManager& pm, const std::set<int>& seeds) {
    std::set<int> candidates;
    for (const auto& path : pm.getActivePaths()) {
        if (path.size() > static_cast<size_t>(n)) {
            int candidate = path[n];
            if (seeds.find(candidate) == seeds.end()) {
                candidates.insert(candidate);
            }
        }
    }
    return candidates;
}

std::set<int> get_basic_seed_n_units(int n, const PathManager& pm, const std::set<int>& seeds, const std::set<int>& candidates) {
    (void)n;

    std::set<int> basic_units;
    std::vector<int> seed_vec(seeds.begin(), seeds.end());

    for (int candidate : candidates) {
        bool is_basic_for_any_pair = false;

        for (size_t i = 0; i < seed_vec.size(); ++i) {
            for (size_t j = i + 1; j < seed_vec.size(); ++j) {
                if (is_globally_essential_for_pair(candidate, seed_vec[i], seed_vec[j], pm)) {
                    is_basic_for_any_pair = true;
                    break;
                }
            }
            if (is_basic_for_any_pair) {
                break;
            }
        }

        if (is_basic_for_any_pair) {
            basic_units.insert(candidate);
        }
    }

    return basic_units;
}

bool is_globally_essential_for_pair(int candidate_node, int seedA, int seedB, const PathManager& pm) {
    auto paths = pm.getActivePathsBetweenSeeds(seedA, seedB);

    if (paths.empty()) {
        return false;
    }

    for (const auto& path : paths) {
        auto it = std::find(path.begin(), path.end(), candidate_node);
        if (it == path.end()) {
            return false;
        }
    }

    return true;
}

}