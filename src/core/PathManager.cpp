/**
 * @file PathManager.cpp
 * @brief Implementation of path-centric data structure for paper algorithm
 */

#include "PathManager.h"
#include <queue>
#include <algorithm>
#include <functional>
#include <random>

PathManager::PathManager(const Graph& graph, const std::vector<int>& seedNodes)
    : originalGraph_(graph), seedNodes_(seedNodes) {
    
    computeAllGeodesicPaths();
    
    activePaths_.resize(allGeodesicPaths_.size(), true);
    
    std::mt19937 tempRng(42);
    createVirtualView(seedNodes_, tempRng);
}

void PathManager::createVirtualView(const std::vector<int>& permutedSeedNodes, std::mt19937& rng) {
    virtualIndexToPair_.clear();
    virtualPairToIndex_.clear();
    virtualSeedPairProcessingOrder_.clear();
    
    int virtualIndex = 0;
    
    for (size_t i = 0; i < permutedSeedNodes.size(); ++i) {
        for (size_t j = i + 1; j < permutedSeedNodes.size(); ++j) {
            int seedA = permutedSeedNodes[i];
            int seedB = permutedSeedNodes[j];
            
            std::pair<int, int> virtualPair = {seedA, seedB};
            
            virtualIndexToPair_.push_back(virtualPair);
            virtualPairToIndex_[virtualPair] = virtualIndex;
            virtualSeedPairProcessingOrder_.push_back(virtualIndex);
            
            virtualIndex++;
        }
    }
    
    std::shuffle(virtualSeedPairProcessingOrder_.begin(), virtualSeedPairProcessingOrder_.end(), rng);
}

void PathManager::deactivatePath(int pathIndex) {
    if (pathIndex >= 0 && pathIndex < static_cast<int>(activePaths_.size())) {
        activePaths_[pathIndex] = false;
    }
}

std::vector<std::vector<int>> PathManager::getActivePaths() const {
    std::vector<std::vector<int>> activePaths;
    
    for (size_t i = 0; i < allGeodesicPaths_.size(); ++i) {
        if (activePaths_[i]) {
            activePaths.push_back(allGeodesicPaths_[i]);
        }
    }
    
    return activePaths;
}

std::set<int> PathManager::getNodesInActivePaths() const {
    std::set<int> nodes;
    
    for (size_t i = 0; i < allGeodesicPaths_.size(); ++i) {
        if (activePaths_[i]) {
            for (int nodeId : allGeodesicPaths_[i]) {
                nodes.insert(nodeId);
            }
        }
    }
    
    return nodes;
}

std::vector<std::vector<int>> PathManager::getActivePathsBetweenSeeds(int seedA, int seedB) const {
    std::vector<std::vector<int>> paths;
    
    std::pair<int, int> canonicalPair = std::make_pair(std::min(seedA, seedB), std::max(seedA, seedB));
    
    auto it = seedPairToPathIndices_.find(canonicalPair);
    if (it != seedPairToPathIndices_.end()) {
        for (int pathIndex : it->second) {
            if (activePaths_[pathIndex]) {
                paths.push_back(allGeodesicPaths_[pathIndex]);
            }
        }
    }
    
    return paths;
}

std::vector<int> PathManager::getActivePathIndicesBetweenSeeds(int seedA, int seedB) const {
    std::vector<int> indices;
    
    std::pair<int, int> canonicalPair = std::make_pair(std::min(seedA, seedB), std::max(seedA, seedB));
    
    auto it = seedPairToPathIndices_.find(canonicalPair);
    if (it != seedPairToPathIndices_.end()) {
        for (int pathIndex : it->second) {
            if (activePaths_[pathIndex]) {
                indices.push_back(pathIndex);
            }
        }
    }
    
    return indices;
}

size_t PathManager::getTotalPathCount() const {
    return allGeodesicPaths_.size();
}

size_t PathManager::getActivePathCount() const {
    return std::count(activePaths_.begin(), activePaths_.end(), true);
}

bool PathManager::isPathActive(int pathIndex) const {
    if (pathIndex >= 0 && pathIndex < static_cast<int>(activePaths_.size())) {
        return activePaths_[pathIndex];
    }
    return false;
}

/**
 * @brief Returns all stored geodesic paths
 */
const std::vector<std::vector<int>>& PathManager::getAllPaths() const {
    return allGeodesicPaths_;
}

void PathManager::computeAllGeodesicPaths() {
    allGeodesicPaths_.clear();
    seedPairToPathIndices_.clear();

    for (size_t i = 0; i < seedNodes_.size(); ++i) {
        for (size_t j = i + 1; j < seedNodes_.size(); ++j) {
            int seedA = seedNodes_[i];
            int seedB = seedNodes_[j];
            
            std::vector<std::vector<int>> paths = findAllGeodesicPathsBetween(seedA, seedB);

            std::pair<int, int> canonicalPair = std::make_pair(std::min(seedA, seedB), std::max(seedA, seedB));
            std::vector<int>& pathIndices = seedPairToPathIndices_[canonicalPair];
            
            for (const auto& path : paths) {
                int pathIndex = allGeodesicPaths_.size();
                allGeodesicPaths_.push_back(path);
                pathIndices.push_back(pathIndex);
            }
        }
    }
}

void PathManager::randomizeSeedPairProcessingOrder(std::mt19937& rng) {
    std::shuffle(virtualSeedPairProcessingOrder_.begin(), virtualSeedPairProcessingOrder_.end(), rng);
}

std::vector<int> PathManager::getCurrentSeedPairOrder() const {
    return virtualSeedPairProcessingOrder_;
}

std::pair<int, int> PathManager::getPairFromIndex(int index) const {
    if (index >= 0 && index < static_cast<int>(virtualIndexToPair_.size())) {
        return virtualIndexToPair_[index];
    }
    throw std::out_of_range("Invalid virtual seed pair index: " + std::to_string(index));
}

void PathManager::resetAllPathsToActive() {
    std::fill(activePaths_.begin(), activePaths_.end(), true);
}

std::vector<std::vector<int>> PathManager::findAllGeodesicPathsBetween(int source, int target) const {
    std::vector<std::vector<int>> allPaths;
    
    if (source == target) {
        return allPaths;
    }
    
    std::queue<int> queue;
    std::map<int, int> distances;
    std::map<int, std::vector<int>> predecessors;
    
    queue.push(source);
    distances[source] = 0;
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        if (current == target) {
            break;
        }
        
        int currentDist = distances[current];
        
        for (int neighbor : originalGraph_.getNeighbors(current)) {
            if (distances.find(neighbor) == distances.end()) {
                distances[neighbor] = currentDist + 1;
                predecessors[neighbor].push_back(current);
                queue.push(neighbor);
            } else if (distances[neighbor] == currentDist + 1) {
                predecessors[neighbor].push_back(current);
            }
        }
    }
    
    if (distances.find(target) == distances.end()) {
        return allPaths;
    }
    
    std::vector<int> currentPath;
    std::function<void(int)> reconstructPaths = [&](int node) {
        currentPath.push_back(node);
        
        if (node == source) {
            std::vector<int> path = currentPath;
            std::reverse(path.begin(), path.end());
            allPaths.push_back(path);
        } else {
            auto it = predecessors.find(node);
            if (it != predecessors.end()) {
                for (int pred : it->second) {
                    reconstructPaths(pred);
                }
            }
        }
        
        currentPath.pop_back();
    };
    
    reconstructPaths(target);

    return allPaths;
} 

void PathManager::randomizeInternalPathOrder(std::mt19937& rng) {
    if (allGeodesicPaths_.empty()) return;
    
    std::vector<int> permutation(allGeodesicPaths_.size());
    std::iota(permutation.begin(), permutation.end(), 0);
    std::shuffle(permutation.begin(), permutation.end(), rng);
    
    std::vector<int> inversePermutation(allGeodesicPaths_.size());
    for (size_t i = 0; i < permutation.size(); ++i) {
        inversePermutation[permutation[i]] = i;
    }
    
    std::vector<std::vector<int>> reorderedPaths;
    reorderedPaths.reserve(allGeodesicPaths_.size());
    for (int newIndex : permutation) {
        reorderedPaths.push_back(allGeodesicPaths_[newIndex]);
    }
    allGeodesicPaths_ = std::move(reorderedPaths);
    
    for (auto& pair : seedPairToPathIndices_) {
        std::vector<int>& pathIndices = pair.second;
        
        for (int& oldIndex : pathIndices) {
            oldIndex = inversePermutation[oldIndex];
        }
        std::shuffle(pathIndices.begin(), pathIndices.end(), rng);
    }
    
    std::fill(activePaths_.begin(), activePaths_.end(), true);
} 