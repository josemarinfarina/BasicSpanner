/**
 * @file BasicSpannerEngine.h
 * @brief Core engine for basic network finding algorithm
 */

#ifndef BASICSPANNERENGINE_H
#define BASICSPANNERENGINE_H

#include <vector>
#include <set>
#include <map>
#include <random>
#include <functional>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include "Graph.h"
#include "GraphAlgorithms.h"
#include "PathManager.h"
#include "AnalysisConfig.h"

/**
 * @brief Statistics for wins, loses and ties in permutation analysis
 */
struct PermutationStatistics {
    int wins;
    int loses;
    int ties;
    
    int ties_essential_vs_non_essential;
    int ties_both_essential;
    int ties_neither_essential;
    
    int ties_resolved;
    int ties_maintained;
    
    std::set<int> nodes_in_ties;
    std::set<int> nodes_in_resolved_ties;
    std::set<int> nodes_in_maintained_ties;
    
    /**
     * @brief Default constructor
     */
    PermutationStatistics() : 
        wins(0), loses(0), ties(0),
        ties_essential_vs_non_essential(0),
        ties_both_essential(0),
        ties_neither_essential(0),
        ties_resolved(0),
        ties_maintained(0) {}
    
    /**
     * @brief Get number of unique nodes involved in any tie
     * @return Number of unique nodes in ties
     */
    int getNodesInTiesCount() const {
        return static_cast<int>(nodes_in_ties.size());
    }
    
    /**
     * @brief Get number of unique nodes involved in resolved ties
     * @return Number of unique nodes in resolved ties
     */
    int getNodesInResolvedTiesCount() const {
        return static_cast<int>(nodes_in_resolved_ties.size());
    }
    
    /**
     * @brief Get number of unique nodes involved in maintained ties
     * @return Number of unique nodes in maintained ties
     */
    int getNodesInMaintainedTiesCount() const {
        return static_cast<int>(nodes_in_maintained_ties.size());
    }
    
    /**
     * @brief Calculate total comparisons
     * @return Total number of comparisons made
     */
    int getTotalComparisons() const {
        return wins + loses + ties;
    }
    
    /**
     * @brief Calculate win rate
     * @return Win rate as percentage
     */
    double getWinRate() const {
        int total = getTotalComparisons();
        return total > 0 ? (static_cast<double>(wins) / total) * 100.0 : 0.0;
    }
    
    /**
     * @brief Calculate tie rate
     * @return Tie rate as percentage
     */
    double getTieRate() const {
        int total = getTotalComparisons();
        return total > 0 ? (static_cast<double>(ties) / total) * 100.0 : 0.0;
    }
    
    /**
     * @brief Calculate tie resolution rate
     * @return Tie resolution rate as percentage
     */
    double getTieResolutionRate() const {
        return ties > 0 ? (static_cast<double>(ties_resolved) / ties) * 100.0 : 0.0;
    }
    
    /**
     * @brief Calculate tie maintenance rate
     * @return Tie maintenance rate as percentage
     */
    double getTieMaintenanceRate() const {
        return ties > 0 ? (static_cast<double>(ties_maintained) / ties) * 100.0 : 0.0;
    }
};

/**
 * @brief Descriptive statistics for permutation results
 * 
 * Contains mean, standard deviation, min, max for various permutation metrics
 * across multiple iterations.
 */
struct PermutationDescriptiveStatistics {
    double wins_mean;
    double wins_stddev;
    int wins_min;
    int wins_max;
    
    double loses_mean;
    double loses_stddev;
    int loses_min;
    int loses_max;
    
    double ties_mean;
    double ties_stddev;
    int ties_min;
    int ties_max;
    
    double ties_resolved_mean;
    double ties_resolved_stddev;
    int ties_resolved_min;
    int ties_resolved_max;
    
    double ties_maintained_mean;
    double ties_maintained_stddev;
    int ties_maintained_min;
    int ties_maintained_max;
    
    double ties_essential_vs_non_essential_mean;
    double ties_essential_vs_non_essential_stddev;
    int ties_essential_vs_non_essential_min;
    int ties_essential_vs_non_essential_max;
    
    double ties_both_essential_mean;
    double ties_both_essential_stddev;
    int ties_both_essential_min;
    int ties_both_essential_max;
    
    double ties_neither_essential_mean;
    double ties_neither_essential_stddev;
    int ties_neither_essential_min;
    int ties_neither_essential_max;
    
    double nodes_in_ties_mean;
    double nodes_in_ties_stddev;
    int nodes_in_ties_min;
    int nodes_in_ties_max;
    
    double nodes_in_resolved_ties_mean;
    double nodes_in_resolved_ties_stddev;
    int nodes_in_resolved_ties_min;
    int nodes_in_resolved_ties_max;
    
    double nodes_in_maintained_ties_mean;
    double nodes_in_maintained_ties_stddev;
    int nodes_in_maintained_ties_min;
    int nodes_in_maintained_ties_max;
    
    int total_iterations;
    
    /**
     * @brief Default constructor
     */
    PermutationDescriptiveStatistics() : 
        wins_mean(0.0), wins_stddev(0.0), wins_min(0), wins_max(0),
        loses_mean(0.0), loses_stddev(0.0), loses_min(0), loses_max(0),
        ties_mean(0.0), ties_stddev(0.0), ties_min(0), ties_max(0),
        ties_resolved_mean(0.0), ties_resolved_stddev(0.0), ties_resolved_min(0), ties_resolved_max(0),
        ties_maintained_mean(0.0), ties_maintained_stddev(0.0), ties_maintained_min(0), ties_maintained_max(0),
        ties_essential_vs_non_essential_mean(0.0), ties_essential_vs_non_essential_stddev(0.0), ties_essential_vs_non_essential_min(0), ties_essential_vs_non_essential_max(0),
        ties_both_essential_mean(0.0), ties_both_essential_stddev(0.0), ties_both_essential_min(0), ties_both_essential_max(0),
        ties_neither_essential_mean(0.0), ties_neither_essential_stddev(0.0), ties_neither_essential_min(0), ties_neither_essential_max(0),
        nodes_in_ties_mean(0.0), nodes_in_ties_stddev(0.0), nodes_in_ties_min(0), nodes_in_ties_max(0),
        nodes_in_resolved_ties_mean(0.0), nodes_in_resolved_ties_stddev(0.0), nodes_in_resolved_ties_min(0), nodes_in_resolved_ties_max(0),
        nodes_in_maintained_ties_mean(0.0), nodes_in_maintained_ties_stddev(0.0), nodes_in_maintained_ties_min(0), nodes_in_maintained_ties_max(0),
        total_iterations(0) {}
    
    /**
     * @brief Calculate descriptive statistics from a vector of PermutationStatistics
     * @param stats Vector of permutation statistics
     */
    void calculateFromVector(const std::vector<PermutationStatistics>& stats);
};

/**
 * @brief Differences in tie statistics between pruned and unpruned versions
 * 
 * Contains the differences in tie counts between the best permutation
 * without pruning and the best permutation with pruning for each batch.
 */
struct TieResolutionDifferences {
    int ties_difference;
    int ties_resolved_difference;
    int ties_maintained_difference;
    
    int ties_essential_vs_non_essential_difference;
    int ties_both_essential_difference;
    int ties_neither_essential_difference;
    
    int batch_index;
    int best_permutation_unpruned;
    int best_permutation_pruned;
    
    /**
     * @brief Default constructor
     */
    TieResolutionDifferences() : 
        ties_difference(0), ties_resolved_difference(0), ties_maintained_difference(0),
        ties_essential_vs_non_essential_difference(0), ties_both_essential_difference(0), ties_neither_essential_difference(0),
        batch_index(-1), best_permutation_unpruned(-1), best_permutation_pruned(-1) {}
    
    /**
     * @brief Constructor with batch information
     */
    TieResolutionDifferences(int batch, int unpruned_perm, int pruned_perm) : 
        ties_difference(0), ties_resolved_difference(0), ties_maintained_difference(0),
        ties_essential_vs_non_essential_difference(0), ties_both_essential_difference(0), ties_neither_essential_difference(0),
        batch_index(batch), best_permutation_unpruned(unpruned_perm), best_permutation_pruned(pruned_perm) {}
};

/**
 * @brief Descriptive statistics for tie resolution differences
 * 
 * Contains mean, standard deviation, min, max for tie resolution differences
 * across multiple batches.
 */
struct TieResolutionDifferencesStatistics {
    double ties_difference_mean;
    double ties_difference_stddev;
    int ties_difference_min;
    int ties_difference_max;
    
    double ties_resolved_difference_mean;
    double ties_resolved_difference_stddev;
    int ties_resolved_difference_min;
    int ties_resolved_difference_max;
    
    double ties_maintained_difference_mean;
    double ties_maintained_difference_stddev;
    int ties_maintained_difference_min;
    int ties_maintained_difference_max;
    
    double ties_essential_vs_non_essential_difference_mean;
    double ties_essential_vs_non_essential_difference_stddev;
    int ties_essential_vs_non_essential_difference_min;
    int ties_essential_vs_non_essential_difference_max;
    
    double ties_both_essential_difference_mean;
    double ties_both_essential_difference_stddev;
    int ties_both_essential_difference_min;
    int ties_both_essential_difference_max;
    
    double ties_neither_essential_difference_mean;
    double ties_neither_essential_difference_stddev;
    int ties_neither_essential_difference_min;
    int ties_neither_essential_difference_max;
    
    int total_batches;
    
    /**
     * @brief Default constructor
     */
    TieResolutionDifferencesStatistics() : 
        ties_difference_mean(0.0), ties_difference_stddev(0.0), ties_difference_min(0), ties_difference_max(0),
        ties_resolved_difference_mean(0.0), ties_resolved_difference_stddev(0.0), ties_resolved_difference_min(0), ties_resolved_difference_max(0),
        ties_maintained_difference_mean(0.0), ties_maintained_difference_stddev(0.0), ties_maintained_difference_min(0), ties_maintained_difference_max(0),
        ties_essential_vs_non_essential_difference_mean(0.0), ties_essential_vs_non_essential_difference_stddev(0.0), ties_essential_vs_non_essential_difference_min(0), ties_essential_vs_non_essential_difference_max(0),
        ties_both_essential_difference_mean(0.0), ties_both_essential_difference_stddev(0.0), ties_both_essential_difference_min(0), ties_both_essential_difference_max(0),
        ties_neither_essential_difference_mean(0.0), ties_neither_essential_difference_stddev(0.0), ties_neither_essential_difference_min(0), ties_neither_essential_difference_max(0),
        total_batches(0) {}
    
    /**
     * @brief Calculate descriptive statistics from a vector of TieResolutionDifferences
     * @param differences Vector of tie resolution differences
     */
    void calculateFromVector(const std::vector<TieResolutionDifferences>& differences);
};

/**
 * @brief Analysis of pruning effects on specific nodes
 * 
 * Contains information about which nodes were eliminated by pruning
 * and their relationship to tie statistics.
 */
struct PruningNodeAnalysis {
    std::set<int> nodes_eliminated_by_pruning;
    
    std::set<int> eliminated_nodes_in_ties;
    std::set<int> eliminated_nodes_in_resolved_ties;
    std::set<int> eliminated_nodes_in_maintained_ties;
    
    int total_nodes_eliminated;
    int eliminated_from_ties;
    int eliminated_from_resolved_ties;
    int eliminated_from_maintained_ties;
    
    double percentage_eliminated_from_ties;
    double percentage_eliminated_from_maintained_ties;
    
    /**
     * @brief Default constructor
     */
    PruningNodeAnalysis() : 
        total_nodes_eliminated(0), eliminated_from_ties(0), eliminated_from_resolved_ties(0), eliminated_from_maintained_ties(0),
        percentage_eliminated_from_ties(0.0), percentage_eliminated_from_maintained_ties(0.0) {}
    
    /**
     * @brief Calculate statistics from the sets
     */
    void calculateStatistics();
};

/**
 * @brief Configuration structure for BasicSpanner algorithm
 */
struct BasicSpannerConfig {
    int numPermutations;
    bool enablePruning;
    bool enableProgressCallback;
    bool collectAllResults;
    unsigned int randomSeed;
    bool randomPruning;

    /**
     * @brief Default constructor with sensible defaults
     */
    BasicSpannerConfig() :
        numPermutations(100),
        enablePruning(true),
        enableProgressCallback(true),
        collectAllResults(false),
        randomSeed(12345),
        randomPruning(false) {}
};

/**
 * @brief Result structure for basic network analysis
 * 
 * Contains the complete result of a basic network finding operation,
 * including the identified network components and metadata.
 * 
 */
struct BasicNetworkResult {
    bool isValid;
    bool isTiedNetwork;
    std::set<int> seedNodes;
    std::set<int> connectorNodes;
    std::set<int> allBasicUnits;
    std::unique_ptr<Graph> basicNetwork_ptr;
    int totalConnectors;
    int preTotalConnectors;
    std::vector<BasicNetworkResult> prunedNetworks;
    
    int permutationNumber;
    std::vector<int> seedPairsOrder;
    std::vector<int> seedNodesOrder;
    unsigned int randomSeedUsed;
    std::string debugInfo;
    
    PermutationStatistics permutationStats;
    
    /**
     * @brief Default constructor initializing all values to defaults
     */
    BasicNetworkResult() : 
        isValid(false), 
        isTiedNetwork(false),
        basicNetwork_ptr(nullptr),
        totalConnectors(0),
        preTotalConnectors(0),
        permutationNumber(0),
        randomSeedUsed(0) {}
    
    /**
     * @brief Destructor to ensure proper cleanup
     */
    ~BasicNetworkResult() {
        try {
            basicNetwork_ptr.reset();
        } catch (...) {
        }
    }

    /**
     * @brief Copy constructor (deep copy for graph if present)
     */
    BasicNetworkResult(const BasicNetworkResult& other) :
        isValid(other.isValid),
        isTiedNetwork(other.isTiedNetwork),
        seedNodes(other.seedNodes),
        connectorNodes(other.connectorNodes),
        allBasicUnits(other.allBasicUnits),
        basicNetwork_ptr(nullptr),
        totalConnectors(other.totalConnectors),
        preTotalConnectors(other.preTotalConnectors),
        prunedNetworks(other.prunedNetworks),
        permutationNumber(other.permutationNumber),
        seedPairsOrder(other.seedPairsOrder),
        seedNodesOrder(other.seedNodesOrder),
        randomSeedUsed(other.randomSeedUsed),
        debugInfo(other.debugInfo),
        permutationStats(other.permutationStats) {
        
        try {
            if (other.basicNetwork_ptr) {
                basicNetwork_ptr = std::make_unique<Graph>(*other.basicNetwork_ptr);
            }
        } catch (const std::exception& e) {
            std::cout << "[BasicNetworkResult] Error copying graph: " << e.what() << std::endl;
            basicNetwork_ptr = nullptr;
        } catch (...) {
            std::cout << "[BasicNetworkResult] Unknown error copying graph" << std::endl;
            basicNetwork_ptr = nullptr;
        }
    }

    /**
     * @brief Move constructor
     */
    BasicNetworkResult(BasicNetworkResult&& other) noexcept :
        isValid(other.isValid),
        isTiedNetwork(other.isTiedNetwork),
        seedNodes(std::move(other.seedNodes)),
        connectorNodes(std::move(other.connectorNodes)),
        allBasicUnits(std::move(other.allBasicUnits)),
        basicNetwork_ptr(std::move(other.basicNetwork_ptr)),
        totalConnectors(other.totalConnectors),
        preTotalConnectors(other.preTotalConnectors),
        prunedNetworks(std::move(other.prunedNetworks)),
        permutationNumber(other.permutationNumber),
        seedPairsOrder(std::move(other.seedPairsOrder)),
        seedNodesOrder(std::move(other.seedNodesOrder)),
        randomSeedUsed(other.randomSeedUsed),
        debugInfo(std::move(other.debugInfo)),
        permutationStats(std::move(other.permutationStats)) {
        other.isValid = false;
        other.totalConnectors = 0;
        other.preTotalConnectors = 0;
        other.permutationNumber = 0;
        other.randomSeedUsed = 0;
    }

    /**
     * @brief Copy assignment operator (deep copy for graph if present)
     */
    BasicNetworkResult& operator=(const BasicNetworkResult& other) {
        if (this == &other) return *this;
        
        try {
            isValid = other.isValid;
            isTiedNetwork = other.isTiedNetwork;
            seedNodes = other.seedNodes;
            connectorNodes = other.connectorNodes;
            allBasicUnits = other.allBasicUnits;
            
            basicNetwork_ptr.reset();
            if (other.basicNetwork_ptr) {
                basicNetwork_ptr = std::make_unique<Graph>(*other.basicNetwork_ptr);
            }
            
            totalConnectors = other.totalConnectors;
            preTotalConnectors = other.preTotalConnectors;
            prunedNetworks = other.prunedNetworks;
            permutationNumber = other.permutationNumber;
            seedPairsOrder = other.seedPairsOrder;
            seedNodesOrder = other.seedNodesOrder;
            randomSeedUsed = other.randomSeedUsed;
            debugInfo = other.debugInfo;
            permutationStats = other.permutationStats;
        } catch (const std::exception& e) {
            std::cout << "[BasicNetworkResult] Error in assignment operator: " << e.what() << std::endl;
            isValid = false;
            basicNetwork_ptr.reset();
        } catch (...) {
            std::cout << "[BasicNetworkResult] Unknown error in assignment operator" << std::endl;
            isValid = false;
            basicNetwork_ptr.reset();
        }
        
        return *this;
    }

    /**
     * @brief Move assignment operator
     */
    BasicNetworkResult& operator=(BasicNetworkResult&& other) noexcept {
        if (this == &other) return *this;
        isValid = other.isValid;
        isTiedNetwork = other.isTiedNetwork;
        seedNodes = std::move(other.seedNodes);
        connectorNodes = std::move(other.connectorNodes);
        allBasicUnits = std::move(other.allBasicUnits);
        basicNetwork_ptr = std::move(other.basicNetwork_ptr);
        totalConnectors = other.totalConnectors;
        preTotalConnectors = other.preTotalConnectors;
        prunedNetworks = std::move(other.prunedNetworks);
        permutationNumber = other.permutationNumber;
        seedPairsOrder = std::move(other.seedPairsOrder);
        seedNodesOrder = std::move(other.seedNodesOrder);
        randomSeedUsed = other.randomSeedUsed;
        debugInfo = std::move(other.debugInfo);
        permutationStats = std::move(other.permutationStats);
        other.isValid = false;
        other.totalConnectors = 0;
        other.preTotalConnectors = 0;
        other.permutationNumber = 0;
        other.randomSeedUsed = 0;
        return *this;
    }
};

/**
 * @brief Collection of results from all permutations
 * 
 * Contains results from all permutations when collecting detailed results,
 * including the best result and information about ties.
 * 
 */
struct AllPermutationResults {
    BasicNetworkResult bestResult;
    std::vector<BasicNetworkResult> allResults;
    std::vector<BasicNetworkResult> tiedResults;
    bool hasMultipleResults;
    
    /**
     * @brief Default constructor
     */
    AllPermutationResults() : hasMultipleResults(false) {}
};

/**
 * @brief Result of path elimination algorithm
 * 
 * Contains information about the convergence and final state of
 * the path elimination phase of the algorithm.
 * 
 */
struct PathEliminationResult {
    bool converged;
    std::set<int> remainingConnectors;
    int iterations;
    
    /**
     * @brief Default constructor
     */
    PathEliminationResult() : 
        converged(false), 
        iterations(0) {}
};

/**
 * @brief Core engine for basic network finding algorithm
 * 
 * This class implements the main algorithm for finding basic networks
 * in complex networks. It supports single and multiple permutation
 * analysis with optional pruning.
 * 
 */
class BasicSpannerEngine {
public:
    /**
     * @brief Callback function type for progress updates
     * 
     * @param permutation Current permutation number
     * @param totalPermutations Total number of permutations
     * @param status Current status message
     */
    using ProgressCallback = std::function<void(int permutation, int totalPermutations, const std::string& status)>;
    
    /**
     * @brief Callback function type for detailed progress updates
     * 
     * @param permutation Current permutation number
     * @param totalPermutations Total number of permutations
     * @param stage Current processing stage
     * @param stageProgress Progress within current stage
     * @param stageTotal Total work in current stage
     */
    using DetailedProgressCallback = std::function<void(int permutation, int totalPermutations, 
                                                       const std::string& stage, int stageProgress, int stageTotal)>;
    
    /**
     * @brief Callback function type for permutation completion
     * 
     * @param permutation Current permutation number
     * @param totalPermutations Total number of permutations
     * @param connectorsFound Number of connectors found (or -1 if failed)
     * @param connectorIds Vector of connector node IDs
     */
    using PermutationCompletedCallback = std::function<void(int permutation, int totalPermutations, int connectorsFound, const std::vector<int>& connectorIds, int preConnectorsFound)>;
    
    /**
     * @brief Callback function type for stop request checking
     * 
     * @return True if analysis should stop
     */
    using StopRequestCallback = std::function<bool()>;
    
    /**
     * @brief Default constructor
     */
    BasicSpannerEngine();
    
    /**
     * @brief Destructor
     */
    ~BasicSpannerEngine();
    
    /**
     * @brief Set progress callback function
     * 
     * @param callback Function to call for progress updates
     */
    void setProgressCallback(ProgressCallback callback);
    
    /**
     * @brief Set detailed progress callback function
     * 
     * @param callback Function to call for detailed progress updates
     */
    void setDetailedProgressCallback(DetailedProgressCallback callback);
    
    /**
     * @brief Set permutation completed callback function
     * 
     * @param callback Function to call when a permutation is completed
     */
    void setPermutationCompletedCallback(PermutationCompletedCallback callback);
    
    /**
     * @brief Set stop request callback function
     * 
     * @param callback Function to call to check if analysis should stop
     */
    void setStopRequestCallback(StopRequestCallback callback);
    
    /**
     * @brief Set number of permutations to perform
     * 
     * @param numPermutations Number of permutations
     */
    void setNumPermutations(int numPermutations);
    
    /**
     * @brief Enable or disable network pruning
     * 
     * @param enable Whether to enable pruning
     */
    void setEnablePruning(bool enable);
    
    /**
     * @brief Set random seed for reproducible results
     * 
     * @param seed Random seed value
     */
    void setRandomSeed(unsigned int seed);
    
    /**
     * @brief Find basic network using the main algorithm
     * 
     * @param graph Input graph to analyze
     * @param seedNodes Vector of seed node IDs
     * @param config Configuration parameters
     * @return Basic network result
     */
    BasicNetworkResult findBasicNetwork(const Graph& graph, 
                                       const std::vector<int>& seedNodes,
                                       const BasicSpannerConfig& config = BasicSpannerConfig());
    
    /**
     * @brief Find basic network using pre-calculated PathManager
     * 
     * @param graph Input graph to analyze
     * @param seedNodes Vector of seed node IDs
     * @param masterPathManager Pre-calculated PathManager with all geodesic paths
     * @param config Configuration parameters
     * @return Basic network result
     */
    BasicNetworkResult findBasicNetwork(const Graph& graph, 
                                       const std::vector<int>& seedNodes,
                                       PathManager& masterPathManager,
                                       const BasicSpannerConfig& config = BasicSpannerConfig());
    
    /**
     * @brief Find basic network and collect results from all permutations
     * 
     * @param graph Input graph to analyze
     * @param seedNodes Vector of seed node IDs
     * @param masterPathManager Pre-calculated PathManager with all geodesic paths
     * @param config Configuration parameters
     * @return Collection of all permutation results
     */
    AllPermutationResults findBasicNetworkWithAllResults(const Graph& graph, 
                                                        const std::vector<int>& seedNodes,
                                                        PathManager& masterPathManager,
                                                        const BasicSpannerConfig& config = BasicSpannerConfig());

protected:
    BasicSpannerConfig config_;
    std::mt19937 randomGenerator_;
    ProgressCallback progressCallback_;
    DetailedProgressCallback detailedProgressCallback_;
    PermutationCompletedCallback permutationCompletedCallback_;
    StopRequestCallback stopRequestCallback_;
    
    /**
     * @brief Core implementation of the basic network finding algorithm for a single permutation.
     * @param graph The input graph.
     * @param seedNodes The list of seed nodes for this permutation.
     * @param currentPermutation The index of the current permutation.
     * @param rng The random number generator for this permutation.
     * @param pathManager The PathManager instance containing all paths.
     * @param threadId The ID of the thread executing this task (-1 for main thread).
     * @return A BasicNetworkResult containing the found network and its metadata.
     */
    BasicNetworkResult runSingleIteration_impl(const Graph& graph,
                                               const std::vector<int>& seedNodes,
                                               int currentPermutation,
                                               std::mt19937& rng,
                                               PathManager& pathManager,
                                               int threadId = -1);

    
    /**
     * @brief Run a single iteration of the paper algorithm
     * 
     * @param graph Input graph
     * @param seedNodes Vector of seed nodes
     * @param currentPermutation Current permutation number
     * @param totalPermutations Total number of permutations
     * @param rng Random number generator for tie-breaking
     * @param pathManager Pre-calculated PathManager with all geodesic paths
     * @return Result of this iteration
     */
    BasicNetworkResult runSingleIteration(const Graph& graph, 
                                         const std::vector<int>& seedNodes,
                                         int currentPermutation, int totalPermutations,
                                         std::mt19937& rng,
                                         PathManager& pathManager);
    
    /**
     * @brief Calculate shortest distances between all seed node pairs
     * 
     * @param graph Input graph
     * @param seedNodes Vector of seed nodes
     * @return Map of node pairs to their shortest distances
     */
    std::unordered_map<std::pair<int, int>, int, PairHash> calculateSeedDistances(
        const Graph& graph, const std::vector<int>& seedNodes);
    
    /**
     * @brief Identify seed+1 units (nodes adjacent to seeds)
     * 
     * @param graph Input graph
     * @param seedNodes Vector of seed nodes
     * @return Set of seed+1 unit nodes
     */
    std::set<int> identifySeedPlusOneUnits(const Graph& graph, 
                                          const std::vector<int>& seedNodes);
    

    

    
    /**
     * @brief Identify seed+N units (nodes at distance N from seeds)
     * 
     * @param graph Input graph
     * @param seedNodes Vector of seed nodes
     * @param existingUnits Set of already identified units
     * @param n Distance from seeds
     * @return Set of seed+N units
     */
    std::set<int> identifySeedPlusNUnits(const Graph& graph,
                                        const std::vector<int>& seedNodes,
                                        const std::set<int>& existingUnits,
                                        int n);
    
    /**
     * @brief Validate that the basic network satisfies all requirements
     * 
     * @param originalGraph Original input graph
     * @param basicNetwork Constructed basic network
     * @param seedNodes Vector of seed nodes
     * @return true if validation passes, false otherwise
     */
    bool validateBasicNetwork(const Graph& originalGraph,
                             const Graph& basicNetwork,
                             const std::vector<int>& seedNodes);

    /**
     * @brief Validate a trial graph in parallel by comparing against pre-calculated distances
     * 
     * @param trialNetwork The trial graph after removing a connector
     * @param seedNodes Vector of seed nodes
     * @param originalDistances Map of pre-calculated geodesic distances from the original graph
     * @return true if all distances are preserved, false otherwise
     */
    bool validateBasicNetworkParallel(
        const Graph& trialNetwork,
        const std::vector<int>& seedNodes,
        const std::unordered_map<std::pair<int, int>, int, PairHash>& originalDistances);
    
    /**
     * @brief Prune tied network to find minimal solutions
     * 
     * @param tiedNetworkInput The result from the main algorithm, potentially with tied connectors.
     * @param originalGraph The full, original graph.
     * @param pruningAnalysis Optional pointer to a struct for detailed pruning analysis.
     * @return A vector of possible pruned network results. Currently, returns one minimal solution.
     */
    std::vector<BasicNetworkResult> pruneBasicNetwork(
        const BasicNetworkResult& tiedNetworkInput, 
        const Graph& originalGraph,
        const AnalysisConfig& config,
        PruningNodeAnalysis* pruningAnalysis = nullptr
    );
    
    /**
     * @brief Prune tied network to find minimal solutions (BasicSpannerConfig overload)
     * 
     * @param tiedNetworkInput The result from the main algorithm, potentially with tied connectors.
     * @param originalGraph The full, original graph.
     * @param config Configuration parameters
     * @param pruningAnalysis Optional pointer to a struct for detailed pruning analysis.
     * @return A vector of possible pruned network results. Currently, returns one minimal solution.
     */
    std::vector<BasicNetworkResult> pruneBasicNetwork(
        const BasicNetworkResult& tiedNetworkInput, 
        const Graph& originalGraph,
        const BasicSpannerConfig& config,
        PruningNodeAnalysis* pruningAnalysis = nullptr
    );
    
    /**
     * @brief Construct basic network graph from identified units
     * 
     * @param originalGraph Original input graph
     * @param basicUnits Set of nodes to include in basic network
     * @return Constructed basic network graph
     */
    Graph constructBasicNetwork(const Graph& originalGraph,
                               const std::set<int>& basicUnits);
    
    /**
     * @brief Create random processing order for nodes
     * 
     * @param nodes Set of nodes to order randomly
     * @return Vector of nodes in random order
     */
    std::vector<int> createRandomOrder(const std::set<int>& nodes);
    
    /**
     * @brief Count paths between source and target that pass through a specific node
     * 
     * @param graph Input graph
     * @param source Source node
     * @param target Target node
     * @param throughNode Intermediate node
     * @return Number of paths passing through the intermediate node
     */
    int countPathsThrough(const Graph& graph, int source, int target, int throughNode);
    
    /**
     * @brief Count all paths through each candidate node
     * 
     * @param graph Input graph
     * @param seedNodes Vector of seed nodes
     * @param candidateNodes Set of candidate intermediate nodes
     * @return Map of nodes to path counts
     */
    std::unordered_map<int, int> countAllPathsThrough(
        const Graph& graph,
        const std::vector<int>& seedNodes,
        const std::set<int>& candidateNodes);
    
    /**
     * @brief Report progress to callback function
     * 
     * @param permutation Current permutation number
     * @param totalPermutations Total number of permutations
     * @param status Current status message
     */
    void reportProgress(int permutation, int totalPermutations, const std::string& status);
    
    /**
     * @brief Report detailed progress to callback function
     * 
     * @param permutation Current permutation number
     * @param totalPermutations Total number of permutations
     * @param stage Current processing stage
     * @param stageProgress Progress within current stage
     * @param stageTotal Total work in current stage
     */
    void reportDetailedProgress(int permutation, int totalPermutations, 
                               const std::string& stage, int stageProgress, int stageTotal);
    
    /**
     * @brief Report permutation completion to callback function
     * 
     * @param permutation Current permutation number
     * @param totalPermutations Total number of permutations
     * @param connectorsFound Number of connectors found (or -1 if failed)
     */
    void reportPermutationCompleted(int permutation, int totalPermutations, int connectorsFound, const std::vector<int>& connectorIds, int preConnectorsFound = -1);
    
    /**
     * @brief Log progress message for debugging
     * 
     * @param message Message to log
     */
    void logProgress(const std::string& message);
    
    /**
     * @brief Check if debug logging is enabled
     * 
     * @return true if debug logging is enabled, false otherwise
     */
    bool isDebugEnabled() const;
    

private:
    mutable std::unordered_map<std::pair<int, int>, std::vector<std::vector<int>>, PairHash> pathCache_;
    
    std::vector<std::vector<int>> getCachedShortestPaths(const Graph& graph, int source, int target) const;
    
    void clearPathCache();
    
    std::set<int> getSeedPlusNUnits(const std::vector<std::vector<int>>& paths, 
                                   const std::vector<int>& seedNodes, int level);
    
    void markEssentialNodes(std::set<int>& essentialNodes, 
                           const std::vector<std::vector<int>>& paths,
                           const std::vector<int>& seedNodes,
                           const std::set<int>& candidates);
    
    std::vector<int> findPathsToEliminate(const std::vector<std::vector<int>>& paths,
                                         const std::vector<int>& seedNodes,
                                         const std::set<int>& candidates,
                                         const std::set<int>& essentialNodes,
                                         int level);
    
    std::vector<std::vector<int>> eliminateSelectedPaths(const std::vector<std::vector<int>>& paths,
                                                        const std::vector<int>& pathsToEliminate);
    
    bool isNodeInPaths(int node, const std::vector<std::vector<int>>& paths);
};

#endif