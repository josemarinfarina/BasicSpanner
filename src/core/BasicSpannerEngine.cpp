/**
 * @file BasicSpannerEngine.cpp
 * @brief Implementation of the BasicSpannerEngine class for basic network analysis
 */

#include "BasicSpannerEngine.h"
#include "PathManager.h"
#include "PaperAlgorithmHelpers.h"
#include "ParallelGraphAlgorithms.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <limits>
#include <fstream>

BasicSpannerEngine::BasicSpannerEngine() : randomGenerator_(std::random_device{}()) {
}

BasicSpannerEngine::~BasicSpannerEngine() {
}

void BasicSpannerEngine::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
}

void BasicSpannerEngine::setDetailedProgressCallback(DetailedProgressCallback callback) {
    detailedProgressCallback_ = callback;
}

void BasicSpannerEngine::setPermutationCompletedCallback(PermutationCompletedCallback callback) {
    permutationCompletedCallback_ = callback;
}

void BasicSpannerEngine::setStopRequestCallback(StopRequestCallback callback) {
    stopRequestCallback_ = callback;
}

void BasicSpannerEngine::setNumPermutations(int numPermutations) {
    config_.numPermutations = numPermutations;
}

void BasicSpannerEngine::setEnablePruning(bool enable) {
    config_.enablePruning = enable;
}

void BasicSpannerEngine::setRandomSeed(unsigned int seed) {
    config_.randomSeed = seed;
    randomGenerator_.seed(seed);
}

BasicNetworkResult BasicSpannerEngine::findBasicNetwork(const Graph& graph, 
                                                   const std::vector<int>& seedNodes,
                                                   const BasicSpannerConfig& config) {
    config_ = config;
    randomGenerator_.seed(config_.randomSeed);
    
    if (seedNodes.empty()) {
        logProgress("Error: No seed nodes provided");
        return BasicNetworkResult();
    }
    
    for (int seed : seedNodes) {
        if (!graph.hasNode(seed)) {
            logProgress("Error: Seed node " + std::to_string(seed) + " not found in graph");
            return BasicNetworkResult();
        }
    }
    
    PathManager masterPathManager(graph, seedNodes);
    return findBasicNetwork(graph, seedNodes, masterPathManager, config);
}

BasicNetworkResult BasicSpannerEngine::findBasicNetwork(const Graph& graph, 
                                                   const std::vector<int>& seedNodes,
                                                   PathManager& masterPathManager,
                                                   const BasicSpannerConfig& config) {
    config_ = config;
    randomGenerator_.seed(config_.randomSeed);
    
    if (seedNodes.empty()) {
        logProgress("Error: No seed nodes provided");
        return BasicNetworkResult();
    }
    
    logProgress("Starting BasicSpanner algorithm with " + std::to_string(seedNodes.size()) + " seeds (using pre-calculated PathManager)");
    
    BasicSpannerConfig mutableConfig = config;
    mutableConfig.collectAllResults = true;
    AllPermutationResults allResults = findBasicNetworkWithAllResults(graph, seedNodes, masterPathManager, mutableConfig);
    
    return allResults.bestResult;
}

BasicNetworkResult BasicSpannerEngine::runSingleIteration_impl(const Graph& graph,
                                                          const std::vector<int>& seedNodes,
                                                          int currentPermutation,
                                                          std::mt19937& rng,
                                                          PathManager& pathManager,
                                                          int threadId)
{
    std::string logPrefix = (threadId >= 0) ? 
        "[PAPER ALGORITHM P:" + std::to_string(currentPermutation) + "][T:" + std::to_string(threadId) + "]" :
        "[PAPER ALGORITHM P:" + std::to_string(currentPermutation) + "]";
    
    BasicNetworkResult result;
    result.seedNodes.insert(seedNodes.begin(), seedNodes.end());
    
    result.permutationNumber = currentPermutation;
    result.randomSeedUsed = config_.randomSeed;
    
    int ties_essential_vs_non_essential = 0;
    int ties_both_essential = 0;
    int ties_neither_essential = 0;
    int wins = 0;
    int loses = 0;
    int ties = 0;
    int ties_resolved = 0;
    int ties_maintained = 0;
    
    if (pathManager.getTotalPathCount() == 0) return result;

    std::vector<int> seedPairIndices = pathManager.getCurrentSeedPairOrder();
    result.seedPairsOrder = seedPairIndices;

    int max_dist = 0;
    for(const auto& p : pathManager.getAllPaths()) max_dist = std::max(max_dist, (int)p.size() - 1);
    
    std::set<int> global_essential_nodes;
    global_essential_nodes.insert(seedNodes.begin(), seedNodes.end());
    
    for (int n = 1; n < max_dist; ++n) {
        std::string levelPrefix = (threadId >= 0) ? 
            "\n--- [T:" + std::to_string(threadId) + "] Processing Level seed+" + std::to_string(n) + " ---" :
            "\n--- Processing Level seed+" + std::to_string(n) + " ---";
        
        bool level_converged = false;
        int iteration_count = 0;
        
        while (!level_converged) {
            iteration_count++;
            std::string iterPrefix = (threadId >= 0) ? 
                "  [T:" + std::to_string(threadId) + "][Level " + std::to_string(n) + "] Iteration " + std::to_string(iteration_count) :
                "  [Level " + std::to_string(n) + "] Iteration " + std::to_string(iteration_count);
            
            std::map<int, int> importanceMap;
            std::string impPrefix = (threadId >= 0) ? 
                "    [T:" + std::to_string(threadId) + "] Recalculating Importance Map from active paths..." :
                "    Recalculating Importance Map from active paths...";
            for (const auto& path : pathManager.getActivePaths()) {
                std::set<int> uniqueNodes(path.begin(), path.end());
                for (int node : uniqueNodes) {
                    if (result.seedNodes.find(node) == result.seedNodes.end()) {
                        importanceMap[node]++;
                    }
                }
            }
            
            auto candidates = PaperAlgorithmHelpers::get_seed_n_units(n, pathManager, result.seedNodes);
            if(candidates.empty()) {
                level_converged = true;
                continue;
            }
            auto basic_units_this_level = PaperAlgorithmHelpers::get_basic_seed_n_units(n, pathManager, result.seedNodes, candidates);
            global_essential_nodes.insert(basic_units_this_level.begin(), basic_units_this_level.end());
            
            std::vector<int> seedPairIndices = pathManager.getCurrentSeedPairOrder();
            
            int paths_eliminated_this_pass = 0;

            for (int pair_index : seedPairIndices) {
                auto pair = pathManager.getPairFromIndex(pair_index);
                auto paths = pathManager.getActivePathsBetweenSeeds(pair.first, pair.second);
                auto path_indices_for_pair = pathManager.getActivePathIndicesBetweenSeeds(pair.first, pair.second);
                
                if (paths.size() <= 1) continue;

                std::map<int, std::vector<size_t>> paths_by_connector;
                std::vector<int> competitors;
                for (size_t i = 0; i < paths.size(); ++i) {
                    if (paths[i].size() > static_cast<size_t>(n)) {
                        int connector = paths[i][n];
                        paths_by_connector[connector].push_back(i);
                        if (std::find(competitors.begin(), competitors.end(), connector) == competitors.end()) {
                            competitors.push_back(connector);
                        }
                    }
                }

                if (competitors.empty()) continue;

                for (int connector_jj : competitors) {
                    if (global_essential_nodes.count(connector_jj)) continue;

                    for (int connector_j : competitors) {
                        if (connector_j == connector_jj) continue;

                        int nc1 = importanceMap.count(connector_jj) ? importanceMap.at(connector_jj) : 0;
                        int nc2 = importanceMap.count(connector_j) ? importanceMap.at(connector_j) : 0;
                        bool j_is_essential = global_essential_nodes.count(connector_j);

                        bool should_eliminate = false;
                        bool jj_is_essential = global_essential_nodes.count(connector_jj);
                        
                        if (nc1 < nc2) {
                            should_eliminate = true;
                            wins++;
                        } else if (nc1 > nc2) {
                            loses++;
                        } else if (nc1 == nc2) {
                            ties++;
                            if (jj_is_essential && !j_is_essential) {
                                ties_essential_vs_non_essential++;
                                result.permutationStats.nodes_in_ties.insert(connector_jj);
                                result.permutationStats.nodes_in_ties.insert(connector_j);
                            } else if (!jj_is_essential && j_is_essential) {
                                ties_essential_vs_non_essential++;
                                result.permutationStats.nodes_in_ties.insert(connector_jj);
                                result.permutationStats.nodes_in_ties.insert(connector_j);
                            } else if (jj_is_essential && j_is_essential) {
                                ties_both_essential++;
                                result.permutationStats.nodes_in_ties.insert(connector_jj);
                                result.permutationStats.nodes_in_ties.insert(connector_j);
                            } else if (!jj_is_essential && !j_is_essential) {
                                ties_neither_essential++;
                                result.permutationStats.nodes_in_ties.insert(connector_jj);
                                result.permutationStats.nodes_in_ties.insert(connector_j);
                            }
                            
                            if (j_is_essential) {
                                should_eliminate = true;
                                ties_resolved++;
                                result.permutationStats.nodes_in_resolved_ties.insert(connector_jj);
                                result.permutationStats.nodes_in_resolved_ties.insert(connector_j);
                            } else {
                                ties_maintained++;
                                result.permutationStats.nodes_in_maintained_ties.insert(connector_jj);
                                result.permutationStats.nodes_in_maintained_ties.insert(connector_j);
                            }
                        }
                        
                        if (should_eliminate) {
                            for (size_t path_local_idx : paths_by_connector[connector_jj]) {
                                int global_path_index = path_indices_for_pair[path_local_idx];
                                if (pathManager.isPathActive(global_path_index)) {
                                    
                                    const auto& eliminated_path = pathManager.getAllPaths()[global_path_index];
                                    std::set<int> uniqueNodes(eliminated_path.begin(), eliminated_path.end());
                                    for(int node : uniqueNodes) {
                                        if (importanceMap.count(node)) {
                                            importanceMap[node]--;
                                        }
                                    }
                                    
                                    pathManager.deactivatePath(global_path_index);
                                    paths_eliminated_this_pass++;
                                }
                            }
                            goto next_competitor_loop;
                        }
                    }
                    next_competitor_loop:;
                }
            }
            if (paths_eliminated_this_pass == 0) {
                level_converged = true;
            }
        }
    }
    
    std::set<int> finalNodes = pathManager.getNodesInActivePaths();
    std::set<int> finalConnectors;
    for (int node : finalNodes) {
        if (result.seedNodes.find(node) == result.seedNodes.end()) {
            finalConnectors.insert(node);
        }
    }
    
    result.connectorNodes = finalConnectors;
    result.totalConnectors = finalConnectors.size();
    result.allBasicUnits = finalNodes;
    result.basicNetwork_ptr = std::make_unique<Graph>(constructBasicNetwork(graph, finalNodes));
    
    result.permutationStats.wins = wins;
    result.permutationStats.loses = loses;
    result.permutationStats.ties = ties;
    result.permutationStats.ties_essential_vs_non_essential = ties_essential_vs_non_essential;
    result.permutationStats.ties_both_essential = ties_both_essential;
    result.permutationStats.ties_neither_essential = ties_neither_essential;
    result.permutationStats.ties_resolved = ties_resolved;
    result.permutationStats.ties_maintained = ties_maintained;
    
    if (validateBasicNetwork(graph, *result.basicNetwork_ptr, seedNodes)) {
        result.isValid = true;
    } else {
        result.isValid = false;
        std::cerr << "[FATAL ERROR] Permutation " << currentPermutation << " produced an invalid network!" << std::endl;
    }
    
    return result;
}

BasicNetworkResult BasicSpannerEngine::runSingleIteration(const Graph& graph, 
                                                     const std::vector<int>& seedNodes,
                                                     int currentPermutation, int totalPermutations,
                                                     std::mt19937& rng,
                                                     PathManager& pathManager) {
    return runSingleIteration_impl(graph, seedNodes, currentPermutation, rng, pathManager, -1);
}

std::unordered_map<std::pair<int, int>, int, PairHash> BasicSpannerEngine::calculateSeedDistances(
    const Graph& graph, const std::vector<int>& seedNodes) {
    
    return GraphAlgorithms::getAllPairDistances(graph, seedNodes);
}

std::set<int> BasicSpannerEngine::identifySeedPlusOneUnits(const Graph& graph, 
                                                     const std::vector<int>& seedNodes) {
    std::set<int> seedPlusOneUnits;
    
    for (int seed : seedNodes) {
        auto units = GraphAlgorithms::getSeedPlusOneNodes(graph, seed, seedNodes);
        seedPlusOneUnits.insert(units.begin(), units.end());
    }
    
    return seedPlusOneUnits;
}

std::set<int> BasicSpannerEngine::identifySeedPlusNUnits(const Graph& graph,
                                                   const std::vector<int>& seedNodes,
                                                   const std::set<int>& existingUnits,
                                                   int n) {
    std::set<int> seedPlusNUnits;
    
    for (int seed : seedNodes) {
        auto units = GraphAlgorithms::getSeedPlusNNodes(graph, seed, seedNodes, n);
        
        for (int unit : units) {
            if (existingUnits.find(unit) == existingUnits.end()) {
                seedPlusNUnits.insert(unit);
            }
        }
    }
    
    return seedPlusNUnits;
}

bool BasicSpannerEngine::validateBasicNetwork(const Graph& originalGraph,
                                         const Graph& basicNetwork,
                                         const std::vector<int>& seedNodes) {
    if (seedNodes.empty() || basicNetwork.getNodeCount() == 0) {
        logProgress("Validation failed: Empty seed nodes or basic network");
        return false;
    }

    logProgress("Validation Debug: Basic network has " + std::to_string(basicNetwork.getNodeCount()) + " nodes and " + std::to_string(basicNetwork.getEdgeCount()) + " edges");

    for (int seed : seedNodes) {
        if (!basicNetwork.hasNode(seed)) {
            logProgress("Validation failed: Seed node " + std::to_string(seed) + " not found in basic network");
            return false;
        }
    }
    logProgress("Validation Check 1 passed: All seeds present in basic network");

    for (size_t i = 0; i < seedNodes.size(); ++i) {
        for (size_t j = i + 1; j < seedNodes.size(); ++j) {
            int originalDistance = GraphAlgorithms::getDistance(originalGraph, seedNodes[i], seedNodes[j]);
            int basicDistance = GraphAlgorithms::getDistance(basicNetwork, seedNodes[i], seedNodes[j]);
            
            if (originalDistance != basicDistance) {
                logProgress("Validation failed: Distance not preserved between seeds " + 
                           std::to_string(seedNodes[i]) + " and " + std::to_string(seedNodes[j]) + 
                           " (original: " + std::to_string(originalDistance) + 
                           ", basic: " + std::to_string(basicDistance) + ")");
                return false;
            }
        }
    }
    logProgress("Validation Check 2 passed: All seed distances preserved");

    if (!basicNetwork.isConnected()) {
        auto components = basicNetwork.getConnectedComponents();
        logProgress("Validation Note: Basic network has " + std::to_string(components.size()) + " connected components");
        
        for (size_t i = 0; i < components.size(); ++i) {
            int seedsInComponent = 0;
            for (int node : components[i]) {
                if (std::find(seedNodes.begin(), seedNodes.end(), node) != seedNodes.end()) {
                    seedsInComponent++;
                }
            }
            logProgress("Validation Note: Component " + std::to_string(i + 1) + " has " + 
                       std::to_string(components[i].size()) + " nodes (" + 
                       std::to_string(seedsInComponent) + " seeds)");
        }
        
        logProgress("Validation Note: Multiple components are acceptable for distant seeds (as per paper)");
    } else {
        logProgress("Validation Check 3 passed: Basic network is fully connected");
    }

    logProgress("Validation PASSED: Basic network satisfies all requirements");
    return true;
}

bool BasicSpannerEngine::validateBasicNetworkParallel(
    const Graph& trialNetwork,
    const std::vector<int>& seedNodes,
    const std::unordered_map<std::pair<int, int>, int, PairHash>& originalDistances) {

    if (seedNodes.empty() || trialNetwork.getNodeCount() == 0) {
        logProgress("Validation failed (Parallel): Empty seed nodes or trial network");
        return false;
    }

    auto trialDistances = ParallelGraphAlgorithms::getAllPairDistancesParallel(trialNetwork, seedNodes);

    for (const auto& seed_pair_info : originalDistances) {
        const auto& seed_pair = seed_pair_info.first;
        int original_dist = seed_pair_info.second;

        auto it = trialDistances.find(seed_pair);

        if (it == trialDistances.end() || it->second != original_dist) {
            logProgress("Validation failed (Parallel): Distance mismatch for pair (" + 
                       std::to_string(seed_pair.first) + ", " + std::to_string(seed_pair.second) + "). " +
                       "Original: " + std::to_string(original_dist) + ", Trial: " + 
                       (it == trialDistances.end() ? "Not Found" : std::to_string(it->second)));
            return false;
        }
    }

    logProgress("Validation PASSED (Parallel): All seed distances preserved");
    return true;
}

std::vector<std::vector<int>> BasicSpannerEngine::getCachedShortestPaths(const Graph& graph, int source, int target) const {
    std::pair<int, int> key = std::make_pair(std::min(source, target), std::max(source, target));
    
    auto it = pathCache_.find(key);
    if (it != pathCache_.end()) {
        return it->second;
    }
    
    std::vector<std::vector<int>> paths = GraphAlgorithms::getAllShortestPaths(graph, source, target);
    pathCache_[key] = paths;
    
    return paths;
}

void BasicSpannerEngine::clearPathCache() {
    pathCache_.clear();
}

std::set<int> BasicSpannerEngine::getSeedPlusNUnits(const std::vector<std::vector<int>>& paths, 
                                                const std::vector<int>& seedNodes, int level) {
    std::set<int> candidates;
    std::set<int> seedSet(seedNodes.begin(), seedNodes.end());
    
    for (const auto& path : paths) {
        if (level <= static_cast<int>(path.size())) {
            if (level > 0 && level <= static_cast<int>(path.size())) {
                int nodeIndex = level - 1;
                if (nodeIndex < static_cast<int>(path.size())) {
                    int candidate = path[nodeIndex];
                    if (seedSet.find(candidate) == seedSet.end()) {
                        candidates.insert(candidate);
                    }
                }
            }
        }
    }
    
    return candidates;
}

void BasicSpannerEngine::markEssentialNodes(std::set<int>& essentialNodes, 
                                       const std::vector<std::vector<int>>& paths,
                                       const std::vector<int>& seedNodes,
                                       const std::set<int>& candidates) {
    std::set<int> seedSet(seedNodes.begin(), seedNodes.end());
    
    for (int candidate : candidates) {
        if (essentialNodes.find(candidate) != essentialNodes.end()) {
            continue;
        }
        
        bool isEssential = false;
        
        std::map<std::pair<int, int>, std::vector<int>> pathsPerPair;
        std::map<std::pair<int, int>, int> pathsWithCandidate;
        
        for (size_t i = 0; i < paths.size(); ++i) {
            const auto& path = paths[i];
            if (path.size() >= 2) {
                int start = path[0];
                int end = path[path.size() - 1];
                
                if (seedSet.find(start) != seedSet.end() && seedSet.find(end) != seedSet.end()) {
                    auto pairKey = std::make_pair(std::min(start, end), std::max(start, end));
                    pathsPerPair[pairKey].push_back(i);
                    
                    if (std::find(path.begin(), path.end(), candidate) != path.end()) {
                        pathsWithCandidate[pairKey]++;
                    }
                }
            }
        }
        
        for (const auto& pair : pathsPerPair) {
            int totalPaths = pair.second.size();
            int pathsWithCand = pathsWithCandidate[pair.first];
            
            if (totalPaths > 0 && pathsWithCand == totalPaths) {
                isEssential = true;
                break;
            }
        }
        
        if (isEssential) {
            essentialNodes.insert(candidate);
        }
    }
}

std::vector<int> BasicSpannerEngine::findPathsToEliminate(const std::vector<std::vector<int>>& paths,
                                                     const std::vector<int>& seedNodes,
                                                     const std::set<int>& candidates,
                                                     const std::set<int>& essentialNodes,
                                                     int level) {
    std::vector<int> pathsToEliminate;
    std::set<int> seedSet(seedNodes.begin(), seedNodes.end());
    
    std::map<int, int> pathCounts;
    for (int candidate : candidates) {
        pathCounts[candidate] = 0;
        for (const auto& path : paths) {
            if (std::find(path.begin(), path.end(), candidate) != path.end()) {
                pathCounts[candidate]++;
            }
        }
    }
    
    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& path = paths[i];
        
        if (path.size() < 2) continue;
        
        int start = path[0];
        int end = path[path.size() - 1];
        
        if (seedSet.find(start) == seedSet.end() || seedSet.find(end) == seedSet.end()) {
            continue;
        }
        
        std::vector<int> pathCandidates;
        for (int node : path) {
            if (candidates.find(node) != candidates.end()) {
                pathCandidates.push_back(node);
            }
        }
        
        for (size_t j = 0; j < pathCandidates.size(); ++j) {
            for (size_t k = 0; k < pathCandidates.size(); ++k) {
                if (j != k) {
                    int cand1 = pathCandidates[j];
                    int cand2 = pathCandidates[k];
                    
                    int nc1 = pathCounts[cand1];
                    int nc2 = pathCounts[cand2];
                    
                    bool shouldEliminate = false;
                    
                    
                    if (nc1 < nc2) {
                        shouldEliminate = true;
                    } else if (nc1 == nc2 && essentialNodes.find(cand2) != essentialNodes.end()) {
                        shouldEliminate = true;
                    }
                    
                    if (essentialNodes.find(cand1) != essentialNodes.end()) {
                        shouldEliminate = false;
                    }
                    
                    if (shouldEliminate && std::find(path.begin(), path.end(), cand1) != path.end()) {
                        pathsToEliminate.push_back(i);
                        break;
                    }
                }
            }
        }
    }
    
    return pathsToEliminate;
}

std::vector<std::vector<int>> BasicSpannerEngine::eliminateSelectedPaths(const std::vector<std::vector<int>>& paths,
                                                                    const std::vector<int>& pathsToEliminate) {
    std::vector<std::vector<int>> newPaths;
    std::set<int> eliminateSet(pathsToEliminate.begin(), pathsToEliminate.end());
    
    for (size_t i = 0; i < paths.size(); ++i) {
        if (eliminateSet.find(i) == eliminateSet.end()) {
            newPaths.push_back(paths[i]);
        }
    }
    
    return newPaths;
}

bool BasicSpannerEngine::isNodeInPaths(int node, const std::vector<std::vector<int>>& paths) {
    for (const auto& path : paths) {
        if (std::find(path.begin(), path.end(), node) != path.end()) {
            return true;
        }
    }
    return false;
}

std::vector<BasicNetworkResult> BasicSpannerEngine::pruneBasicNetwork(
    const BasicNetworkResult& tiedNetworkInput, const Graph& originalGraph,
    const AnalysisConfig& config, PruningNodeAnalysis* pruningAnalysis) {

    std::vector<BasicNetworkResult> prunedSolutions;
    if (!tiedNetworkInput.isValid || tiedNetworkInput.connectorNodes.empty()) {
        return prunedSolutions;
    }

    logProgress("Starting pruning phase. Initial connectors: " + std::to_string(tiedNetworkInput.connectorNodes.size()));

    std::vector<int> seedVector(tiedNetworkInput.seedNodes.begin(), tiedNetworkInput.seedNodes.end());
    const auto originalDistances = ParallelGraphAlgorithms::getAllPairDistancesParallel(originalGraph, seedVector);
    
    logProgress("Pre-calculated original distances for " + std::to_string(originalDistances.size()) + " seed pairs using parallel algorithm");

    BasicNetworkResult bestPrunedResult = tiedNetworkInput;

    std::vector<int> connectorsToTry(tiedNetworkInput.connectorNodes.begin(), tiedNetworkInput.connectorNodes.end());
    
    bool improvementMadeInLastPass = true;
    while(improvementMadeInLastPass) {
        improvementMadeInLastPass = false;
        
        if(config.randomPruning) {
            std::shuffle(connectorsToTry.begin(), connectorsToTry.end(), randomGenerator_);
        }

        auto iterator = connectorsToTry.begin();
        while (iterator != connectorsToTry.end()) {
            int connectorToRemove = *iterator;

            std::set<int> trialBasicUnits = bestPrunedResult.allBasicUnits;
            trialBasicUnits.erase(connectorToRemove);

            std::unique_ptr<Graph> trialGraph_ptr = std::make_unique<Graph>(constructBasicNetwork(originalGraph, trialBasicUnits));
            
            if (validateBasicNetworkParallel(*trialGraph_ptr, seedVector, originalDistances)) {
                logProgress("Pruning successful: Removed connector " + std::to_string(connectorToRemove) + 
                           ". New count: " + std::to_string(bestPrunedResult.connectorNodes.size() - 1));

                if (pruningAnalysis) {
                    pruningAnalysis->nodes_eliminated_by_pruning.insert(connectorToRemove);
                    
                    if (tiedNetworkInput.permutationStats.nodes_in_ties.count(connectorToRemove)) {
                        pruningAnalysis->eliminated_nodes_in_ties.insert(connectorToRemove);
                    }
                    if (tiedNetworkInput.permutationStats.nodes_in_resolved_ties.count(connectorToRemove)) {
                        pruningAnalysis->eliminated_nodes_in_resolved_ties.insert(connectorToRemove);
                    }
                    if (tiedNetworkInput.permutationStats.nodes_in_maintained_ties.count(connectorToRemove)) {
                        pruningAnalysis->eliminated_nodes_in_maintained_ties.insert(connectorToRemove);
                    }
                }

                bestPrunedResult.allBasicUnits = trialBasicUnits;
                bestPrunedResult.connectorNodes.erase(connectorToRemove);
                bestPrunedResult.totalConnectors = bestPrunedResult.connectorNodes.size();
                bestPrunedResult.basicNetwork_ptr = std::move(trialGraph_ptr);
                
                iterator = connectorsToTry.erase(iterator);
                improvementMadeInLastPass = true;
            } else {
                ++iterator;
            }
        }
    }

    logProgress("Pruning phase complete. Final connector count: " + std::to_string(bestPrunedResult.totalConnectors));

    prunedSolutions.push_back(std::move(bestPrunedResult));
    return prunedSolutions;
}

std::vector<BasicNetworkResult> BasicSpannerEngine::pruneBasicNetwork(
    const BasicNetworkResult& tiedNetworkInput,
    const Graph& originalGraph,
    const BasicSpannerConfig& config,
    PruningNodeAnalysis* pruningAnalysis) {

    std::vector<BasicNetworkResult> prunedSolutions;
    if (!tiedNetworkInput.isValid || tiedNetworkInput.connectorNodes.empty()) {
        return prunedSolutions;
    }

    logProgress("Starting pruning phase. Initial connectors: " + std::to_string(tiedNetworkInput.connectorNodes.size()));

    std::vector<int> seedVector(tiedNetworkInput.seedNodes.begin(), tiedNetworkInput.seedNodes.end());
    const auto originalDistances = ParallelGraphAlgorithms::getAllPairDistancesParallel(originalGraph, seedVector);
    
    logProgress("Pre-calculated original distances for " + std::to_string(originalDistances.size()) + " seed pairs using parallel algorithm");

    BasicNetworkResult bestPrunedResult = tiedNetworkInput;

    std::vector<int> connectorsToTry(tiedNetworkInput.connectorNodes.begin(), tiedNetworkInput.connectorNodes.end());
    
    bool improvementMadeInLastPass = true;
    while(improvementMadeInLastPass) {
        improvementMadeInLastPass = false;
        
        if(config.randomPruning) {
            std::shuffle(connectorsToTry.begin(), connectorsToTry.end(), randomGenerator_);
        }

        auto iterator = connectorsToTry.begin();
        while (iterator != connectorsToTry.end()) {
            int connectorToRemove = *iterator;

            std::set<int> trialBasicUnits = bestPrunedResult.allBasicUnits;
            trialBasicUnits.erase(connectorToRemove);

            std::unique_ptr<Graph> trialGraph_ptr = std::make_unique<Graph>(constructBasicNetwork(originalGraph, trialBasicUnits));
            
            if (validateBasicNetworkParallel(*trialGraph_ptr, seedVector, originalDistances)) {
                logProgress("Pruning successful: Removed connector " + std::to_string(connectorToRemove) + 
                           ". New count: " + std::to_string(bestPrunedResult.connectorNodes.size() - 1));

                if (pruningAnalysis) {
                    pruningAnalysis->nodes_eliminated_by_pruning.insert(connectorToRemove);
                    
                    if (tiedNetworkInput.permutationStats.nodes_in_ties.count(connectorToRemove)) {
                        pruningAnalysis->eliminated_nodes_in_ties.insert(connectorToRemove);
                    }
                    if (tiedNetworkInput.permutationStats.nodes_in_resolved_ties.count(connectorToRemove)) {
                        pruningAnalysis->eliminated_nodes_in_resolved_ties.insert(connectorToRemove);
                    }
                    if (tiedNetworkInput.permutationStats.nodes_in_maintained_ties.count(connectorToRemove)) {
                        pruningAnalysis->eliminated_nodes_in_maintained_ties.insert(connectorToRemove);
                    }
                }

                bestPrunedResult.allBasicUnits = trialBasicUnits;
                bestPrunedResult.connectorNodes.erase(connectorToRemove);
                bestPrunedResult.totalConnectors = bestPrunedResult.connectorNodes.size();
                bestPrunedResult.basicNetwork_ptr = std::move(trialGraph_ptr);
                
                iterator = connectorsToTry.erase(iterator);
                improvementMadeInLastPass = true;
            } else {
                ++iterator;
            }
        }
    }

    logProgress("Pruning phase complete. Final connector count: " + std::to_string(bestPrunedResult.totalConnectors));

    prunedSolutions.push_back(std::move(bestPrunedResult));
    return prunedSolutions;
}

Graph BasicSpannerEngine::constructBasicNetwork(const Graph& originalGraph,
                                           const std::set<int>& basicUnits) {
    Graph basicNetworkGraph;
    
    logProgress("ConstructBasicNetwork Debug: Starting with " + std::to_string(basicUnits.size()) + " basic units");
    
    int nodesAdded = 0;
    for (int unitId : basicUnits) {
        if (originalGraph.hasNode(unitId)) {
            basicNetworkGraph.addNode(unitId, originalGraph.getNodeName(unitId));
            nodesAdded++;
        } else {
            logProgress("ConstructBasicNetwork Warning: Basic unit " + std::to_string(unitId) + " not found in original graph");
        }
    }
    logProgress("ConstructBasicNetwork Debug: Added " + std::to_string(nodesAdded) + " nodes");
    
    int edgesAdded = 0;
    for (int unitId : basicUnits) {
        if (!originalGraph.hasNode(unitId)) continue;
        for (int neighbor : originalGraph.getNeighbors(unitId)) {
            if (basicUnits.count(neighbor)) {
                if (basicNetworkGraph.hasNode(unitId) && basicNetworkGraph.hasNode(neighbor)) {
                    basicNetworkGraph.addEdge(unitId, neighbor);
                    edgesAdded++;
                }
            }
        }
    }
    logProgress("ConstructBasicNetwork Debug: Added " + std::to_string(edgesAdded) + " edges (may include duplicates)");
    logProgress("ConstructBasicNetwork Debug: Final graph has " + std::to_string(basicNetworkGraph.getNodeCount()) + " nodes and " + std::to_string(basicNetworkGraph.getEdgeCount()) + " edges");
    
    return basicNetworkGraph;
}

std::vector<int> BasicSpannerEngine::createRandomOrder(const std::set<int>& nodes) {
    std::vector<int> order(nodes.begin(), nodes.end());
    std::shuffle(order.begin(), order.end(), randomGenerator_);
    return order;
}

int BasicSpannerEngine::countPathsThrough(const Graph& graph,
                                     int source, int target, int throughNode) {
    
    auto allPaths = getCachedShortestPaths(graph, source, target);
    
    int count = 0;
    for (const auto& path : allPaths) {
        if (std::find(path.begin(), path.end(), throughNode) != path.end()) {
            count++;
        }
    }
    
    return count;
}

std::unordered_map<int, int> BasicSpannerEngine::countAllPathsThrough(
    const Graph& graph,
    const std::vector<int>& seedNodes,
    const std::set<int>& candidateNodes) {
    
    std::unordered_map<int, int> pathCounts;
    
    for (int node : candidateNodes) {
        pathCounts[node] = 0;
    }
    
    for (size_t i = 0; i < seedNodes.size(); ++i) {
        for (size_t j = i + 1; j < seedNodes.size(); ++j) {
            for (int node : candidateNodes) {
                pathCounts[node] += countPathsThrough(graph, seedNodes[i], seedNodes[j], node);
            }
        }
    }
    
    return pathCounts;
}

void BasicSpannerEngine::logProgress(const std::string& message) {
    if (isDebugEnabled()) {
        std::cout << "[BasicSpannerEngine] " << message << std::endl;
    }
}

bool BasicSpannerEngine::isDebugEnabled() const {
    return false;
}

void BasicSpannerEngine::reportProgress(int permutation, int totalPermutations, const std::string& status) {
    if (progressCallback_ && config_.enableProgressCallback) {
        progressCallback_(permutation, totalPermutations, status);
    }
}

void BasicSpannerEngine::reportDetailedProgress(int permutation, int totalPermutations, 
                                           const std::string& stage, int stageProgress, int stageTotal) {
    if (detailedProgressCallback_ && config_.enableProgressCallback) {
        detailedProgressCallback_(permutation, totalPermutations, stage, stageProgress, stageTotal);
    }
}

void BasicSpannerEngine::reportPermutationCompleted(int permutation, int totalPermutations, int connectorsFound, const std::vector<int>& connectorIds, int preConnectorsFound) {
    if (permutationCompletedCallback_ && config_.enableProgressCallback) {
        permutationCompletedCallback_(permutation, totalPermutations, connectorsFound, connectorIds, preConnectorsFound);
    }
}

AllPermutationResults BasicSpannerEngine::findBasicNetworkWithAllResults(
    const Graph& graph, const std::vector<int>& seedNodes, PathManager& masterPathManager, const BasicSpannerConfig& config)
{
    config_ = config;
    
    AllPermutationResults finalResultsCollection;
    
    if (seedNodes.empty()) {
        logProgress("Error: No seed nodes provided");
        return finalResultsCollection;
    }
    for (int seed : seedNodes) {
        if (!graph.hasNode(seed)) {
            logProgress("Error: Seed node " + std::to_string(seed) + " not found in graph");
            return finalResultsCollection;
        }
    }
    
    logProgress("Starting BasicSpanner algorithm with " + std::to_string(seedNodes.size()) + " seeds (collecting all results)");
    
    BasicNetworkResult currentBestIterationResult; 
    currentBestIterationResult.totalConnectors = std::numeric_limits<int>::max();
    currentBestIterationResult.seedNodes.insert(seedNodes.begin(), seedNodes.end());
    
    std::vector<BasicNetworkResult> allValidSummaries; 
    std::vector<BasicNetworkResult> currentTiedSummaries;
    
    for (int permutation = 0; permutation < config_.numPermutations; ++permutation)
    {
        if (stopRequestCallback_ && stopRequestCallback_()) {
            break;
        }
        
        PathManager pathManagerForPermutation = masterPathManager;
        pathManagerForPermutation.randomizeInternalPathOrder(randomGenerator_);
        
        std::vector<int> permutedSeedNodes = seedNodes;
        std::shuffle(permutedSeedNodes.begin(), permutedSeedNodes.end(), randomGenerator_);
        pathManagerForPermutation.createVirtualView(permutedSeedNodes, randomGenerator_);
        
        pathManagerForPermutation.resetAllPathsToActive();
        clearPathCache();
        
        reportProgress(permutation + 1, config_.numPermutations, 
                      "Running permutation " + std::to_string(permutation + 1));
        
        BasicNetworkResult iterationResult = runSingleIteration(graph, seedNodes, permutation + 1, config_.numPermutations, randomGenerator_, pathManagerForPermutation);
        
        if (iterationResult.isValid) {
            iterationResult.preTotalConnectors = iterationResult.totalConnectors;

            const bool pruningEnabled = (config_.enablePruning || config_.randomPruning);
            if (pruningEnabled && iterationResult.basicNetwork_ptr) {
                auto perPermPruned = pruneBasicNetwork(iterationResult, graph, config, nullptr);
                if (!perPermPruned.empty() && perPermPruned.back().isValid) {
                    int preCount = iterationResult.preTotalConnectors;
                    iterationResult = std::move(perPermPruned.back());
                    iterationResult.preTotalConnectors = preCount;
                    iterationResult.permutationNumber = permutation + 1;
                }
            }

            std::ofstream permFile("permutation_results.txt", std::ios::app);
            if (permFile.is_open()) {
                permFile << "Permutation " << (permutation + 1) << ": " << iterationResult.totalConnectors << " connectors" << std::endl;
                permFile.close();
            }
            
            std::vector<int> connectorIds(iterationResult.connectorNodes.begin(), iterationResult.connectorNodes.end());
            reportPermutationCompleted(permutation + 1, config_.numPermutations, iterationResult.totalConnectors, connectorIds, iterationResult.preTotalConnectors);
            
            BasicNetworkResult summaryForOverallList;
            summaryForOverallList.isValid = iterationResult.isValid;
            summaryForOverallList.isTiedNetwork = iterationResult.isTiedNetwork;
            summaryForOverallList.seedNodes = iterationResult.seedNodes;
            summaryForOverallList.connectorNodes = iterationResult.connectorNodes;
            summaryForOverallList.allBasicUnits = iterationResult.allBasicUnits;
            summaryForOverallList.totalConnectors = iterationResult.totalConnectors;
            summaryForOverallList.preTotalConnectors = iterationResult.preTotalConnectors;
            summaryForOverallList.prunedNetworks = iterationResult.prunedNetworks;
            
            summaryForOverallList.permutationNumber = iterationResult.permutationNumber;
            summaryForOverallList.seedPairsOrder = iterationResult.seedPairsOrder;
            summaryForOverallList.seedNodesOrder = iterationResult.seedNodesOrder;
            summaryForOverallList.randomSeedUsed = iterationResult.randomSeedUsed;
            summaryForOverallList.debugInfo = iterationResult.debugInfo;
            for (auto& pruned_summary : summaryForOverallList.prunedNetworks) {
                pruned_summary.basicNetwork_ptr.reset();
                for (auto& sub_pruned_summary : pruned_summary.prunedNetworks) {
                    sub_pruned_summary.basicNetwork_ptr.reset();
                }
            }
            allValidSummaries.push_back(std::move(summaryForOverallList)); 

            if (iterationResult.totalConnectors < currentBestIterationResult.totalConnectors) {
                currentBestIterationResult = std::move(iterationResult);
                
                BasicNetworkResult tiedSummary;
                tiedSummary.isValid = currentBestIterationResult.isValid;
                tiedSummary.isTiedNetwork = currentBestIterationResult.isTiedNetwork;
                tiedSummary.seedNodes = currentBestIterationResult.seedNodes;
                tiedSummary.connectorNodes = currentBestIterationResult.connectorNodes;
                tiedSummary.allBasicUnits = currentBestIterationResult.allBasicUnits;
                tiedSummary.totalConnectors = currentBestIterationResult.totalConnectors;
                tiedSummary.preTotalConnectors = currentBestIterationResult.preTotalConnectors;
                tiedSummary.prunedNetworks = currentBestIterationResult.prunedNetworks;
                for (auto& pruned_summary_in_tied : tiedSummary.prunedNetworks) {
                    pruned_summary_in_tied.basicNetwork_ptr.reset();
                     for (auto& sub_pruned_summary : pruned_summary_in_tied.prunedNetworks) {
                        sub_pruned_summary.basicNetwork_ptr.reset();
                    }
                }
                currentTiedSummaries.clear();
                currentTiedSummaries.push_back(std::move(tiedSummary));

            } else if (iterationResult.totalConnectors == currentBestIterationResult.totalConnectors) {
                BasicNetworkResult tiedSummary;
                tiedSummary.isValid = iterationResult.isValid;
                tiedSummary.isTiedNetwork = iterationResult.isTiedNetwork;
                tiedSummary.seedNodes = iterationResult.seedNodes;
                tiedSummary.connectorNodes = iterationResult.connectorNodes;
                tiedSummary.allBasicUnits = iterationResult.allBasicUnits;
                tiedSummary.totalConnectors = iterationResult.totalConnectors;
                tiedSummary.preTotalConnectors = iterationResult.preTotalConnectors;
                tiedSummary.prunedNetworks = iterationResult.prunedNetworks;
                for (auto& pruned_summary_in_tied : tiedSummary.prunedNetworks) {
                    pruned_summary_in_tied.basicNetwork_ptr.reset();
                    for (auto& sub_pruned_summary : pruned_summary_in_tied.prunedNetworks) {
                        sub_pruned_summary.basicNetwork_ptr.reset();
                    }
                }
                currentTiedSummaries.push_back(std::move(tiedSummary));
            }
        } else {
            std::vector<int> emptyConnectorIds;
            reportPermutationCompleted(permutation + 1, config_.numPermutations, -1, emptyConnectorIds, -1);
        }
    } 
    
    if (currentBestIterationResult.isValid) {
        finalResultsCollection.bestResult = std::move(currentBestIterationResult); 
        finalResultsCollection.allResults = std::move(allValidSummaries); 
        finalResultsCollection.tiedResults = std::move(currentTiedSummaries);
        finalResultsCollection.hasMultipleResults = (finalResultsCollection.allResults.size() > 1 || finalResultsCollection.tiedResults.size() > 1);

        if (finalResultsCollection.tiedResults.size() > 1) {
            reportProgress(config_.numPermutations, config_.numPermutations, "Found tied solutions - using best individual result");
            logProgress("Found " + std::to_string(finalResultsCollection.tiedResults.size()) + " tied solutions with " + 
                       std::to_string(finalResultsCollection.bestResult.totalConnectors) + " connectors each. Using first tied result.");
            
            finalResultsCollection.bestResult.isTiedNetwork = true;
        } 
        
        if ((config_.enablePruning || config_.randomPruning) && finalResultsCollection.bestResult.basicNetwork_ptr) {
            logProgress("Per-permutation pruning already applied during heuristic loop. Skipping redundant final pruning pass. Final connectors: " +
                       std::to_string(finalResultsCollection.bestResult.totalConnectors));
        }
        
        reportProgress(config_.numPermutations, config_.numPermutations, "Algorithm completed");
        logProgress("Paper algorithm completed. Final basic network has " + 
                   std::to_string(finalResultsCollection.bestResult.totalConnectors) + " connectors (" +\
                   std::to_string(finalResultsCollection.allResults.size()) + " total permutations)");
    } else {
        reportProgress(config_.numPermutations, config_.numPermutations, "Algorithm failed");
        logProgress("Algorithm failed to find valid basic network");
    }
    
    return finalResultsCollection;
}

void PermutationDescriptiveStatistics::calculateFromVector(const std::vector<PermutationStatistics>& stats) {
    if (stats.empty()) {
        return;
    }
    
    total_iterations = static_cast<int>(stats.size());
    
    double wins_sum = 0.0, loses_sum = 0.0, ties_sum = 0.0;
    double ties_resolved_sum = 0.0, ties_maintained_sum = 0.0;
    double ties_essential_vs_non_essential_sum = 0.0, ties_both_essential_sum = 0.0, ties_neither_essential_sum = 0.0;
    double nodes_in_ties_sum = 0.0, nodes_in_resolved_ties_sum = 0.0, nodes_in_maintained_ties_sum = 0.0;
    
    wins_min = wins_max = stats[0].wins;
    loses_min = loses_max = stats[0].loses;
    ties_min = ties_max = stats[0].ties;
    ties_resolved_min = ties_resolved_max = stats[0].ties_resolved;
    ties_maintained_min = ties_maintained_max = stats[0].ties_maintained;
    ties_essential_vs_non_essential_min = ties_essential_vs_non_essential_max = stats[0].ties_essential_vs_non_essential;
    ties_both_essential_min = ties_both_essential_max = stats[0].ties_both_essential;
    ties_neither_essential_min = ties_neither_essential_max = stats[0].ties_neither_essential;
    nodes_in_ties_min = nodes_in_ties_max = stats[0].getNodesInTiesCount();
    nodes_in_resolved_ties_min = nodes_in_resolved_ties_max = stats[0].getNodesInResolvedTiesCount();
    nodes_in_maintained_ties_min = nodes_in_maintained_ties_max = stats[0].getNodesInMaintainedTiesCount();
    
    for (const auto& stat : stats) {
        wins_sum += stat.wins;
        wins_min = std::min(wins_min, stat.wins);
        wins_max = std::max(wins_max, stat.wins);
        
        loses_sum += stat.loses;
        loses_min = std::min(loses_min, stat.loses);
        loses_max = std::max(loses_max, stat.loses);
        
        ties_sum += stat.ties;
        ties_min = std::min(ties_min, stat.ties);
        ties_max = std::max(ties_max, stat.ties);
        
        ties_resolved_sum += stat.ties_resolved;
        ties_resolved_min = std::min(ties_resolved_min, stat.ties_resolved);
        ties_resolved_max = std::max(ties_resolved_max, stat.ties_resolved);
        
        ties_maintained_sum += stat.ties_maintained;
        ties_maintained_min = std::min(ties_maintained_min, stat.ties_maintained);
        ties_maintained_max = std::max(ties_maintained_max, stat.ties_maintained);
        
        ties_essential_vs_non_essential_sum += stat.ties_essential_vs_non_essential;
        ties_essential_vs_non_essential_min = std::min(ties_essential_vs_non_essential_min, stat.ties_essential_vs_non_essential);
        ties_essential_vs_non_essential_max = std::max(ties_essential_vs_non_essential_max, stat.ties_essential_vs_non_essential);
        
        ties_both_essential_sum += stat.ties_both_essential;
        ties_both_essential_min = std::min(ties_both_essential_min, stat.ties_both_essential);
        ties_both_essential_max = std::max(ties_both_essential_max, stat.ties_both_essential);
        
        ties_neither_essential_sum += stat.ties_neither_essential;
        ties_neither_essential_min = std::min(ties_neither_essential_min, stat.ties_neither_essential);
        ties_neither_essential_max = std::max(ties_neither_essential_max, stat.ties_neither_essential);
        
        int nodes_in_ties_count = stat.getNodesInTiesCount();
        nodes_in_ties_sum += nodes_in_ties_count;
        nodes_in_ties_min = std::min(nodes_in_ties_min, nodes_in_ties_count);
        nodes_in_ties_max = std::max(nodes_in_ties_max, nodes_in_ties_count);
        
        int nodes_in_resolved_ties_count = stat.getNodesInResolvedTiesCount();
        nodes_in_resolved_ties_sum += nodes_in_resolved_ties_count;
        nodes_in_resolved_ties_min = std::min(nodes_in_resolved_ties_min, nodes_in_resolved_ties_count);
        nodes_in_resolved_ties_max = std::max(nodes_in_resolved_ties_max, nodes_in_resolved_ties_count);
        
        int nodes_in_maintained_ties_count = stat.getNodesInMaintainedTiesCount();
        nodes_in_maintained_ties_sum += nodes_in_maintained_ties_count;
        nodes_in_maintained_ties_min = std::min(nodes_in_maintained_ties_min, nodes_in_maintained_ties_count);
        nodes_in_maintained_ties_max = std::max(nodes_in_maintained_ties_max, nodes_in_maintained_ties_count);
    }
    
    wins_mean = wins_sum / total_iterations;
    loses_mean = loses_sum / total_iterations;
    ties_mean = ties_sum / total_iterations;
    ties_resolved_mean = ties_resolved_sum / total_iterations;
    ties_maintained_mean = ties_maintained_sum / total_iterations;
    ties_essential_vs_non_essential_mean = ties_essential_vs_non_essential_sum / total_iterations;
    ties_both_essential_mean = ties_both_essential_sum / total_iterations;
    ties_neither_essential_mean = ties_neither_essential_sum / total_iterations;
    nodes_in_ties_mean = nodes_in_ties_sum / total_iterations;
    nodes_in_resolved_ties_mean = nodes_in_resolved_ties_sum / total_iterations;
    nodes_in_maintained_ties_mean = nodes_in_maintained_ties_sum / total_iterations;
    
    double wins_variance = 0.0, loses_variance = 0.0, ties_variance = 0.0;
    double ties_resolved_variance = 0.0, ties_maintained_variance = 0.0;
    double ties_essential_vs_non_essential_variance = 0.0, ties_both_essential_variance = 0.0, ties_neither_essential_variance = 0.0;
    double nodes_in_ties_variance = 0.0, nodes_in_resolved_ties_variance = 0.0, nodes_in_maintained_ties_variance = 0.0;
    
    for (const auto& stat : stats) {
        wins_variance += (stat.wins - wins_mean) * (stat.wins - wins_mean);
        loses_variance += (stat.loses - loses_mean) * (stat.loses - loses_mean);
        ties_variance += (stat.ties - ties_mean) * (stat.ties - ties_mean);
        ties_resolved_variance += (stat.ties_resolved - ties_resolved_mean) * (stat.ties_resolved - ties_resolved_mean);
        ties_maintained_variance += (stat.ties_maintained - ties_maintained_mean) * (stat.ties_maintained - ties_maintained_mean);
        ties_essential_vs_non_essential_variance += (stat.ties_essential_vs_non_essential - ties_essential_vs_non_essential_mean) * (stat.ties_essential_vs_non_essential - ties_essential_vs_non_essential_mean);
        ties_both_essential_variance += (stat.ties_both_essential - ties_both_essential_mean) * (stat.ties_both_essential - ties_both_essential_mean);
        ties_neither_essential_variance += (stat.ties_neither_essential - ties_neither_essential_mean) * (stat.ties_neither_essential - ties_neither_essential_mean);
        
        int nodes_in_ties_count = stat.getNodesInTiesCount();
        nodes_in_ties_variance += (nodes_in_ties_count - nodes_in_ties_mean) * (nodes_in_ties_count - nodes_in_ties_mean);
        
        int nodes_in_resolved_ties_count = stat.getNodesInResolvedTiesCount();
        nodes_in_resolved_ties_variance += (nodes_in_resolved_ties_count - nodes_in_resolved_ties_mean) * (nodes_in_resolved_ties_count - nodes_in_resolved_ties_mean);
        
        int nodes_in_maintained_ties_count = stat.getNodesInMaintainedTiesCount();
        nodes_in_maintained_ties_variance += (nodes_in_maintained_ties_count - nodes_in_maintained_ties_mean) * (nodes_in_maintained_ties_count - nodes_in_maintained_ties_mean);
    }
    
    wins_stddev = std::sqrt(wins_variance / total_iterations);
    loses_stddev = std::sqrt(loses_variance / total_iterations);
    ties_stddev = std::sqrt(ties_variance / total_iterations);
    ties_resolved_stddev = std::sqrt(ties_resolved_variance / total_iterations);
    ties_maintained_stddev = std::sqrt(ties_maintained_variance / total_iterations);
    ties_essential_vs_non_essential_stddev = std::sqrt(ties_essential_vs_non_essential_variance / total_iterations);
    ties_both_essential_stddev = std::sqrt(ties_both_essential_variance / total_iterations);
    ties_neither_essential_stddev = std::sqrt(ties_neither_essential_variance / total_iterations);
    nodes_in_ties_stddev = std::sqrt(nodes_in_ties_variance / total_iterations);
    nodes_in_resolved_ties_stddev = std::sqrt(nodes_in_resolved_ties_variance / total_iterations);
    nodes_in_maintained_ties_stddev = std::sqrt(nodes_in_maintained_ties_variance / total_iterations);
}

void TieResolutionDifferencesStatistics::calculateFromVector(const std::vector<TieResolutionDifferences>& differences) {
    if (differences.empty()) {
        return;
    }
    
    total_batches = static_cast<int>(differences.size());
    
    double ties_diff_sum = 0.0, ties_resolved_diff_sum = 0.0, ties_maintained_diff_sum = 0.0;
    double ties_essential_vs_non_essential_diff_sum = 0.0, ties_both_essential_diff_sum = 0.0, ties_neither_essential_diff_sum = 0.0;
    
    ties_difference_min = ties_difference_max = differences[0].ties_difference;
    ties_resolved_difference_min = ties_resolved_difference_max = differences[0].ties_resolved_difference;
    ties_maintained_difference_min = ties_maintained_difference_max = differences[0].ties_maintained_difference;
    ties_essential_vs_non_essential_difference_min = ties_essential_vs_non_essential_difference_max = differences[0].ties_essential_vs_non_essential_difference;
    ties_both_essential_difference_min = ties_both_essential_difference_max = differences[0].ties_both_essential_difference;
    ties_neither_essential_difference_min = ties_neither_essential_difference_max = differences[0].ties_neither_essential_difference;
    
    for (const auto& diff : differences) {
        ties_diff_sum += diff.ties_difference;
        ties_difference_min = std::min(ties_difference_min, diff.ties_difference);
        ties_difference_max = std::max(ties_difference_max, diff.ties_difference);
        
        ties_resolved_diff_sum += diff.ties_resolved_difference;
        ties_resolved_difference_min = std::min(ties_resolved_difference_min, diff.ties_resolved_difference);
        ties_resolved_difference_max = std::max(ties_resolved_difference_max, diff.ties_resolved_difference);
        
        ties_maintained_diff_sum += diff.ties_maintained_difference;
        ties_maintained_difference_min = std::min(ties_maintained_difference_min, diff.ties_maintained_difference);
        ties_maintained_difference_max = std::max(ties_maintained_difference_max, diff.ties_maintained_difference);
        
        ties_essential_vs_non_essential_diff_sum += diff.ties_essential_vs_non_essential_difference;
        ties_essential_vs_non_essential_difference_min = std::min(ties_essential_vs_non_essential_difference_min, diff.ties_essential_vs_non_essential_difference);
        ties_essential_vs_non_essential_difference_max = std::max(ties_essential_vs_non_essential_difference_max, diff.ties_essential_vs_non_essential_difference);
        
        ties_both_essential_diff_sum += diff.ties_both_essential_difference;
        ties_both_essential_difference_min = std::min(ties_both_essential_difference_min, diff.ties_both_essential_difference);
        ties_both_essential_difference_max = std::max(ties_both_essential_difference_max, diff.ties_both_essential_difference);
        
        ties_neither_essential_diff_sum += diff.ties_neither_essential_difference;
        ties_neither_essential_difference_min = std::min(ties_neither_essential_difference_min, diff.ties_neither_essential_difference);
        ties_neither_essential_difference_max = std::max(ties_neither_essential_difference_max, diff.ties_neither_essential_difference);
    }
    
    ties_difference_mean = ties_diff_sum / total_batches;
    ties_resolved_difference_mean = ties_resolved_diff_sum / total_batches;
    ties_maintained_difference_mean = ties_maintained_diff_sum / total_batches;
    ties_essential_vs_non_essential_difference_mean = ties_essential_vs_non_essential_diff_sum / total_batches;
    ties_both_essential_difference_mean = ties_both_essential_diff_sum / total_batches;
    ties_neither_essential_difference_mean = ties_neither_essential_diff_sum / total_batches;
    
    double ties_diff_variance = 0.0, ties_resolved_diff_variance = 0.0, ties_maintained_diff_variance = 0.0;
    double ties_essential_vs_non_essential_diff_variance = 0.0, ties_both_essential_diff_variance = 0.0, ties_neither_essential_diff_variance = 0.0;
    
    for (const auto& diff : differences) {
        ties_diff_variance += (diff.ties_difference - ties_difference_mean) * (diff.ties_difference - ties_difference_mean);
        ties_resolved_diff_variance += (diff.ties_resolved_difference - ties_resolved_difference_mean) * (diff.ties_resolved_difference - ties_resolved_difference_mean);
        ties_maintained_diff_variance += (diff.ties_maintained_difference - ties_maintained_difference_mean) * (diff.ties_maintained_difference - ties_maintained_difference_mean);
        ties_essential_vs_non_essential_diff_variance += (diff.ties_essential_vs_non_essential_difference - ties_essential_vs_non_essential_difference_mean) * (diff.ties_essential_vs_non_essential_difference - ties_essential_vs_non_essential_difference_mean);
        ties_both_essential_diff_variance += (diff.ties_both_essential_difference - ties_both_essential_difference_mean) * (diff.ties_both_essential_difference - ties_both_essential_difference_mean);
        ties_neither_essential_diff_variance += (diff.ties_neither_essential_difference - ties_neither_essential_difference_mean) * (diff.ties_neither_essential_difference - ties_neither_essential_difference_mean);
    }
    
    ties_difference_stddev = std::sqrt(ties_diff_variance / total_batches);
    ties_resolved_difference_stddev = std::sqrt(ties_resolved_diff_variance / total_batches);
    ties_maintained_difference_stddev = std::sqrt(ties_maintained_diff_variance / total_batches);
    ties_essential_vs_non_essential_difference_stddev = std::sqrt(ties_essential_vs_non_essential_diff_variance / total_batches);
    ties_both_essential_difference_stddev = std::sqrt(ties_both_essential_diff_variance / total_batches);
    ties_neither_essential_difference_stddev = std::sqrt(ties_neither_essential_diff_variance / total_batches);
}

void PruningNodeAnalysis::calculateStatistics() {
    total_nodes_eliminated = static_cast<int>(nodes_eliminated_by_pruning.size());
    eliminated_from_ties = static_cast<int>(eliminated_nodes_in_ties.size());
    eliminated_from_resolved_ties = static_cast<int>(eliminated_nodes_in_resolved_ties.size());
    eliminated_from_maintained_ties = static_cast<int>(eliminated_nodes_in_maintained_ties.size());
    
    if (total_nodes_eliminated > 0) {
        percentage_eliminated_from_ties = (static_cast<double>(eliminated_from_ties) / total_nodes_eliminated) * 100.0;
        percentage_eliminated_from_maintained_ties = (static_cast<double>(eliminated_from_maintained_ties) / total_nodes_eliminated) * 100.0;
    } else {
        percentage_eliminated_from_ties = 0.0;
        percentage_eliminated_from_maintained_ties = 0.0;
    }
}
 