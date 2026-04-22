/**
 * @file PathManager.h
 * @brief Path-centric data structure for faithful paper algorithm implementation
 * 
 * This class implements the path management system required for the original
 * paper's subtractive and iterative heuristic strategy, replacing the current
 * node-centric approach with a path-centric one.
 */

#ifndef PATHMANAGER_H
#define PATHMANAGER_H

#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <random>
#include <numeric>
#include "Graph.h"

/**
 * @brief Manages geodesic paths for the paper's algorithm implementation
 * 
 * This class centralizes the management of all geodesic paths between seed pairs,
 * implementing the path activation/deactivation mechanism essential for the
 * competitive pruning strategy described in the original paper.
 * 
 * The class follows the paper's approach of starting with all possible geodesic
 * paths and iteratively eliminating the less relevant ones through competitive
 * analysis.
 */
class PathManager {
public:
    /**
     * @brief Constructor that calculates and stores all initial geodesic paths
     * 
     * Implements the initialization phase of the paper's algorithm by computing
     * all geodesic paths between all seed pairs. This forms the complete set
     * from which the algorithm will subtract paths iteratively.
     * 
     * @param graph Original graph reference
     * @param seedNodes Vector of seed node IDs
     */
    PathManager(const Graph& graph, const std::vector<int>& seedNodes);
    
    void randomizeInternalPathOrder(std::mt19937& rng);
    void createVirtualView(const std::vector<int>& permutedSeedNodes, std::mt19937& rng);
    
    void deactivatePath(int pathIndex);
    std::vector<std::vector<int>> getActivePaths() const;
    std::set<int> getNodesInActivePaths() const;
    
    /**
     * @brief Returns active paths connecting a specific seed pair (using current virtual view)
     */
    std::vector<std::vector<int>> getActivePathsBetweenSeeds(int seedA, int seedB) const;
    
    /**
     * @brief Returns indices of active paths connecting a specific seed pair (using current virtual view)
     */
    std::vector<int> getActivePathIndicesBetweenSeeds(int seedA, int seedB) const;
    
    size_t getTotalPathCount() const;
    size_t getActivePathCount() const;
    bool isPathActive(int pathIndex) const;
    const std::vector<std::vector<int>>& getAllPaths() const;
    
    void randomizeSeedPairProcessingOrder(std::mt19937& rng);
    std::vector<int> getCurrentSeedPairOrder() const;
    std::pair<int, int> getPairFromIndex(int index) const;
    void resetAllPathsToActive();

private:
    const Graph& originalGraph_;
    std::vector<int> seedNodes_;
    
    std::vector<std::vector<int>> allGeodesicPaths_;

    std::map<std::pair<int, int>, std::vector<int>> seedPairToPathIndices_;
    
    std::vector<int> currentIndexMapping_;
    std::vector<std::pair<int, int>> virtualIndexToPair_;
    std::map<std::pair<int, int>, int> virtualPairToIndex_;
    std::vector<int> virtualSeedPairProcessingOrder_;
    
    std::vector<bool> activePaths_;
    
    void computeAllGeodesicPaths();
    std::vector<std::vector<int>> findAllGeodesicPathsBetween(int source, int target) const;
};

#endif