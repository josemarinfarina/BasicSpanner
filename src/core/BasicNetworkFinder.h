/**
 * @file BasicNetworkFinder.h
 * @brief Main interface for basic network analysis and finding
 */

#ifndef BASIC_NETWORK_FINDER_H
#define BASIC_NETWORK_FINDER_H

#include "Graph.h"
#include "BasicSpannerEngine.h"
#include "ParallelBasicSpannerEngine.h"
#include "AnalysisConfig.h"
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <fstream>
#include <map>
#include <set>

/**
 * @brief Statistical analysis structures for network properties
 * 
 * Contains statistical measures for a collection of network analysis results
 * 
 */
struct NetworkStatistics {
    double mean;
    double standardDeviation;
    int minimum;
    int maximum;
    std::vector<int> allValues;
    
    /**
     * @brief Default constructor initializing all values to zero
     */
    NetworkStatistics() : 
        mean(0.0), 
        standardDeviation(0.0), 
        minimum(0), 
        maximum(0) {}
};

/**
 * @brief Results of statistical analysis comparing experimental and random networks
 * 
 * Contains statistical comparison between the experimental network (using actual
 * seed nodes) and multiple random networks for significance testing.
 * 
 */
struct StatisticalAnalysisResult {
    int experimentalTiedConnectors;
    int experimentalPrunedConnectors;
    
    NetworkStatistics randomTiedStatistics;
    NetworkStatistics randomPrunedStatistics;
    
    double tiedZScore;
    double prunedZScore;
    
    std::string goTermName;
    std::string goDomain;
    int numSeedNodes;
    int numRandomPermutations;
    
    /**
     * @brief Default constructor initializing all values to zero
     */
    StatisticalAnalysisResult() :
        experimentalTiedConnectors(0),
        experimentalPrunedConnectors(0),
        tiedZScore(0.0),
        prunedZScore(0.0),
        numSeedNodes(0),
        numRandomPermutations(0) {}
};

/**
 * @brief Complete result structure for basic network analysis
 * 
 * Contains all information about the analysis run, including the basic network
 * result, timing information, and optional statistical analysis.
 * 
 */
struct AnalysisResult {
    bool success;
    std::string errorMessage;
    BasicNetworkResult basicNetworkResult;
    
    int originalNodeCount;
    int originalEdgeCount;
    int seedCount;
    int connectorCount;
    double reductionPercentage;
    
    double executionTimeSeconds;
    
    bool usedParallelExecution;
    int threadsUsed;
    double parallelSpeedup;
    
    bool hasStatisticalAnalysis;
    StatisticalAnalysisResult statisticalResult;
    
    std::vector<BasicNetworkResult> allPermutationResults;
    bool hasDetailedResults;
    
    PermutationStatistics permutationStats;
    
    /**
     * @brief Default constructor initializing all values to defaults
     */
    AnalysisResult() : 
        success(false), 
        originalNodeCount(0), 
        originalEdgeCount(0),
        seedCount(0), 
        connectorCount(0),
        reductionPercentage(0.0),
        executionTimeSeconds(0.0),
        usedParallelExecution(false),
        threadsUsed(1),
        parallelSpeedup(1.0),
        hasStatisticalAnalysis(false),
        hasDetailedResults(false) {}

    AnalysisResult(const AnalysisResult& other) = default;

    AnalysisResult& operator=(const AnalysisResult& other) = default;

    AnalysisResult(AnalysisResult&& other) noexcept = default;

    AnalysisResult& operator=(AnalysisResult&& other) noexcept = default;

    ~AnalysisResult() = default;
};

/**
 * @brief Result structure for batch analysis containing statistics from multiple batches
 * 
 * Contains the best result from all batches plus aggregated statistics
 * across all batches.
 * 
 */
struct BatchAnalysisResult {
    AnalysisResult bestResult;
    std::vector<AnalysisResult> allBatchResults;
    
    AnalysisResult bestResultWithoutPruning;
    std::vector<AnalysisResult> allBatchResultsWithoutPruning;
    
    AnalysisResult bestResultWithPruning;
    std::vector<AnalysisResult> allBatchResultsWithPruning;
    
    int totalBatches;
    int successfulBatches;
    int failedBatches;
    
    NetworkStatistics connectorStatistics;
    NetworkStatistics connectorStatisticsWithoutPruning;
    NetworkStatistics connectorStatisticsWithPruning;
    
    NetworkStatistics nodeCountStatistics;
    NetworkStatistics edgeCountStatistics;
    
    std::vector<PermutationStatistics> permutationStatsPerIteration;
    std::vector<PermutationStatistics> permutationStatsPerIterationWithoutPruning;
    std::vector<PermutationStatistics> permutationStatsPerIterationWithPruning;
    
    PermutationDescriptiveStatistics permutationDescriptiveStats;
    PermutationDescriptiveStatistics permutationDescriptiveStatsWithoutPruning;
    PermutationDescriptiveStatistics permutationDescriptiveStatsWithPruning;
    
    std::vector<TieResolutionDifferences> tieResolutionDifferences;
    TieResolutionDifferencesStatistics tieResolutionDifferencesStats;
    
    PruningNodeAnalysis pruningNodeAnalysis;
    
    double wallClockTimeSeconds;
    double cumulativeBatchTime;
    double averageExecutionTime;
    double minExecutionTime;
    double maxExecutionTime;
    
    double successRate;
    
    /**
     * @brief Default constructor initializing all values to defaults
     */
    BatchAnalysisResult() : 
        totalBatches(0), 
        successfulBatches(0),
        failedBatches(0),
        wallClockTimeSeconds(0.0),
        cumulativeBatchTime(0.0),
        averageExecutionTime(0.0),
        minExecutionTime(0.0),
        maxExecutionTime(0.0),
        successRate(0.0) {}
    
    /**
     * @brief Calculate statistics from all batch results
     */
    void calculateStatistics();
    
    /**
     * @brief Calculate only basic statistics (fast version)
     */
    void calculateBasicStatistics();
};

/**
 * @brief Main class for finding basic networks in complex networks
 * 
 * This class provides the main interface for performing basic network analysis.
 * It supports both sequential and parallel processing, statistical analysis,
 * and various configuration options.
 * 
 */
class BasicNetworkFinder {
public:
    /**
     * @brief Callback function type for progress updates
     * 
     * @param progress Current progress value
     * @param total Total work to be done
     * @param status Current status message
     */
    using ProgressCallback = std::function<void(int progress, int total, const std::string& status)>;
    
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
     * @brief Callback function type for permutation completion updates
     * 
     * @param permutation Completed permutation number
     * @param totalPermutations Total number of permutations
     * @param connectorsFound Number of connectors found (or -1 for failure)
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
    BasicNetworkFinder();
    
    /**
     * @brief Destructor
     */
    ~BasicNetworkFinder();
    
    /**
     * @brief Find basic network from a graph file
     * 
     * @param graphFile Path to the graph file
     * @param seedNodeNames Vector of seed node names
     * @param config Analysis configuration options
     * @return Complete analysis result
     */
    AnalysisResult findBasicNetwork(const std::string& graphFile,
                                  const std::vector<std::string>& seedNodeNames,
                                  const AnalysisConfig& config = AnalysisConfig());
    
    /**
     * @brief Find basic network from a Graph object
     * 
     * @param graph Graph object to analyze
     * @param seedNodeIds Vector of seed node IDs
     * @param config Analysis configuration options
     * @return Complete analysis result
     */
    AnalysisResult findBasicNetwork(const Graph& graph,
                                  const std::vector<int>& seedNodeIds,
                                  const AnalysisConfig& config = AnalysisConfig());
    
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
     * @param callback Function to call when permutations complete
     */
    void setPermutationCompletedCallback(PermutationCompletedCallback callback);
    
    /**
     * @brief Set stop request callback
     * @param callback Function to call to check if analysis should stop
     */
    void setStopRequestCallback(StopRequestCallback callback);
    
    /**
     * @brief Set analysis configuration
     * 
     * @param config Configuration to use for analysis
     */
    void setConfig(const AnalysisConfig& config);
    
    /**
     * @brief Load graph from file
     * 
     * @param filename Path to the graph file
     * @return true if loading was successful, false otherwise
     */
    bool loadGraph(const std::string& filename);
    
    /**
     * @brief Load graph from file with column specification
     * 
     * @param filename Path to the graph file
     * @param sourceColumn Column index for source nodes (0-based)
     * @param targetColumn Column index for target nodes (0-based)
     * @param separator Column separator character
     * @param hasHeader Whether file has header row
     * @param skipLines Number of lines to skip at beginning
     * @param nodeNameColumn Column index for node names (-1 if not present)
     * @param applyNamesToTarget Whether to apply names to target nodes
     * @return true if loading was successful, false otherwise
     */
    bool loadGraphWithColumns(const std::string& filename, 
                             int sourceColumn, int targetColumn,
                             const std::string& separator = " ",
                             bool hasHeader = false, int skipLines = 0,
                             int nodeNameColumn = -1,
                             bool applyNamesToTarget = true,
                             bool caseSensitive = true);
    
    /**
     * @brief Save analysis results to file
     * 
     * @param result Analysis result to save
     * @param filename Output file path
     * @return true if saving was successful, false otherwise
     */
    bool saveResults(const AnalysisResult& result, const std::string& filename) const;
    
    /**
     * @brief Save basic network graph to file
     * 
     * @param result Analysis result containing the basic network
     * @param filename Output file path
     * @return true if saving was successful, false otherwise
     */
    bool saveBasicNetworkGraph(const AnalysisResult& result, const std::string& filename) const;
    
    /**
     * @brief Get const reference to internal graph
     * 
     * @return Const reference to the loaded graph
     */
    const Graph& getGraph() const { return graph_; }
    
    /**
     * @brief Get mutable reference to internal graph
     * 
     * @return Mutable reference to the loaded graph
     */
    Graph& getGraph() { return graph_; }
    
    /**
     * @brief Validate seed node IDs against a graph
     * 
     * @param graph Graph to validate against
     * @param seedIds Vector of seed node IDs to validate
     * @return true if all seed IDs are valid, false otherwise
     */
    static bool validateSeedNodes(const Graph& graph, const std::vector<int>& seedIds);
    
    /**
     * @brief Validate seed node names against a graph
     * 
     * @param graph Graph to validate against
     * @param seedNames Vector of seed node names to validate
     * @return true if all seed names are valid, false otherwise
     */
    static bool validateSeedNodes(const Graph& graph, const std::vector<std::string>& seedNames);
    
    /**
     * @brief Convert seed node names to IDs
     * 
     * @param seedNames Vector of seed node names
     * @return Vector of corresponding node IDs
     */
    std::vector<int> convertSeedNamesToIds(const std::vector<std::string>& seedNames) const;
    
    /**
     * @brief Convert seed node IDs to names
     * 
     * @param seedIds Vector of seed node IDs
     * @return Vector of corresponding node names
     */
    std::vector<std::string> convertSeedIdsToNames(const std::vector<int>& seedIds) const;
    
    /**
     * @brief Get the last analysis result
     * 
     * @return Reference to the last analysis result
     */
    const AnalysisResult& getLastResult() const { return lastResult_; }
    
    /**
     * @brief Calculate analysis statistics
     * 
     * @param originalGraph Original network graph
     * @param result Basic network result
     * @param executionTime Time taken for analysis
     * @return Analysis result with calculated statistics
     */
    static AnalysisResult calculateStatistics(const Graph& originalGraph,
                                            const BasicNetworkResult& result,
                                            double executionTime);
    
    /**
     * @brief Calculate network statistics from connector counts
     * 
     * @param connectorCounts Vector of connector counts from multiple runs
     * @return Statistical measures of the connector counts
     */
    static NetworkStatistics calculateNetworkStatistics(const std::vector<int>& connectorCounts);
    
    /**
     * @brief Calculate Z-score for experimental value against random distribution
     * 
     * @param experimentalValue Value from experimental analysis
     * @param randomStats Statistics from random permutations
     * @return Z-score indicating statistical significance
     */
    static double calculateZScore(int experimentalValue, const NetworkStatistics& randomStats);
    
    /**
     * @brief Perform complete statistical analysis
     * 
     * @param experimentalResult Result from experimental analysis
     * @param randomResults Results from random permutations
     * @param config Analysis configuration
     * @return Complete statistical analysis result
     */
    static StatisticalAnalysisResult performStatisticalAnalysis(
        const BasicNetworkResult& experimentalResult,
        const std::vector<BasicNetworkResult>& randomResults,
        const AnalysisConfig& config);
    
    /**
     * @brief Format analysis results as human-readable string
     * 
     * @param result Analysis result to format
     * @return Formatted string representation
     */
    std::string formatResults(const AnalysisResult& result) const;
    
    /**
     * @brief Format statistical results as human-readable string
     * 
     * @param stats Statistical analysis result to format
     * @return Formatted string representation
     */
    std::string formatStatisticalResults(const StatisticalAnalysisResult& stats) const;
    
    /**
     * @brief Format detailed permutation results as human-readable string
     * 
     * @param allResults Vector of all permutation results
     * @return Formatted string representation
     */
    std::string formatDetailedPermutationResults(const std::vector<BasicNetworkResult>& allResults) const;

    /**
     * @brief Format and save detailed permutation results directly to an output file stream
     * 
     * @param allResults Vector of all permutation results (summaries)
     * @param detailedFile Output file stream to write to
     * @return true if formatting and saving was successful, false otherwise
     */
    bool formatAndSaveDetailedPermutationResults(const std::vector<BasicNetworkResult>& allResults, std::ofstream& detailedFile) const;

    /**
     * @brief Run analysis using a pre-calculated PathManager
     * 
     * @param graph The graph to analyze.
     * @param seedNodeIds The vector of seed node IDs for this specific batch.
     * @param config The analysis configuration.
     * @param pathManager A reference to the already computed PathManager for this batch.
     * @return The result of the analysis for the batch.
     */
    AnalysisResult runAnalysisWithPrecalculatedPaths(
        const Graph& graph,
        const std::vector<int>& seedNodeIds,
        const AnalysisConfig& config,
        PathManager& pathManager);

private:
    Graph graph_;
    BasicSpannerEngine engine_;
    ParallelBasicSpannerEngine parallelEngine_;
    AnalysisConfig config_;
    ProgressCallback progressCallback_;
    DetailedProgressCallback detailedProgressCallback_;
    PermutationCompletedCallback permutationCompletedCallback_;
    StopRequestCallback stopRequestCallback_;
    AnalysisResult lastResult_;
    
    /**
     * @brief Cache for pre-calculated PathManager objects.
     * Key is the seed set (std::set for canonical ordering).
     * Value is the PathManager with all initial geodesic paths.
     */
    std::map<std::set<int>, PathManager> pathManagerCache_;
    
    /**
     * @brief Create output directory if it doesn't exist
     * 
     * @param directory Path to directory to create
     * @return true if directory exists or was created successfully
     */
    bool createOutputDirectory(const std::string& directory) const;
    
    /**
     * @brief Generate timestamp string for file naming
     * 
     * @return Timestamp string in YYYY-MM-DD_HH-MM-SS format
     */
    std::string generateTimestamp() const;
    
    /**
     * @brief Determine whether to use parallel execution
     * 
     * @param graph Graph to analyze
     * @param seedIds Vector of seed node IDs
     * @param config Analysis configuration
     * @return true if parallel execution should be used
     */
    bool shouldUseParallelExecution(const Graph& graph, const std::vector<int>& seedIds, const AnalysisConfig& config) const;
    
    /**
     * @brief Handle progress updates from analysis engine
     * 
     * @param permutation Current permutation number
     * @param total Total permutations
     * @param status Current status message
     */
    void onEngineProgress(int permutation, int total, const std::string& status);
    
    /**
     * @brief Handle detailed progress updates from analysis engine
     * 
     * @param permutation Current permutation number
     * @param totalPermutations Total number of permutations
     * @param stage Current processing stage
     * @param stageProgress Progress within current stage
     * @param stageTotal Total work in current stage
     */
    void onEngineDetailedProgress(int permutation, int totalPermutations, 
                                 const std::string& stage, int stageProgress, int stageTotal);
    
    /**
     * @brief Handle permutation completion from analysis engine
     * 
     * @param permutation Completed permutation number
     * @param totalPermutations Total number of permutations
     * @param connectorsFound Number of connectors found (or -1 for failure)
     * @param connectorIds Vector of connector node IDs
     */
    void onEnginePermutationCompleted(int permutation, int totalPermutations, int connectorsFound, const std::vector<int>& connectorIds, int preConnectorsFound = -1);
};

#endif