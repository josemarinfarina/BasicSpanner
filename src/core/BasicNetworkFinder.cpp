#include "BasicNetworkFinder.h"
#include "ParallelBasicSpannerEngine.h"
#include "PathManager.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <random>

BasicNetworkFinder::BasicNetworkFinder() {
    engine_.setProgressCallback([this](int permutation, int total, const std::string& status) {
        onEngineProgress(permutation, total, status);
    });
    
    engine_.setDetailedProgressCallback([this](int permutation, int totalPermutations, 
                                              const std::string& stage, int stageProgress, int stageTotal) {
        onEngineDetailedProgress(permutation, totalPermutations, stage, stageProgress, stageTotal);
    });
    
    engine_.setPermutationCompletedCallback([this](int permutation, int totalPermutations, int connectorsFound, const std::vector<int>& connectorIds, int preConnectorsFound) {
        onEnginePermutationCompleted(permutation, totalPermutations, connectorsFound, connectorIds, preConnectorsFound);
    });
    
    engine_.setStopRequestCallback([this]() {
        if (stopRequestCallback_) {
            return stopRequestCallback_();
        }
        return false;
    });
    
    parallelEngine_.setThreadSafeProgressCallback([this](int permutation, int total, const std::string& status, int threadId) {
        onEngineProgress(permutation, total, status);
    });
    
    parallelEngine_.setThreadSafeDetailedProgressCallback([this](int permutation, int totalPermutations, 
                                                 const std::string& stage, int stageProgress, int stageTotal, int threadId) {
        onEngineDetailedProgress(permutation, totalPermutations, stage, stageProgress, stageTotal);
    });
    
    parallelEngine_.setThreadSafePermutationCompletedCallback(
        [this](int permutation, int totalPermutations, int connectorsFound, int threadId, const std::vector<int>& connectorIds, int preConnectorsFound) {
            onEnginePermutationCompleted(permutation, totalPermutations, connectorsFound, connectorIds, preConnectorsFound);
        }
    );
}

BasicNetworkFinder::~BasicNetworkFinder() {
}

AnalysisResult BasicNetworkFinder::findBasicNetwork(const std::string& graphFile,
                                                  const std::vector<std::string>& seedNodeNames,
                                                  const AnalysisConfig& config) {
    config_ = config;
    
    AnalysisResult result;
    
    if (!loadGraph(graphFile)) {
        result.errorMessage = "Failed to load graph from file: " + graphFile;
        return result;
    }
    
    auto seedIds = convertSeedNamesToIds(seedNodeNames);
    if (seedIds.empty()) {
        result.errorMessage = "No valid seed nodes found";
        return result;
    }
    
    return findBasicNetwork(graph_, seedIds, config);
}

AnalysisResult BasicNetworkFinder::findBasicNetwork(const Graph& graph,
                                                  const std::vector<int>& seedNodeIds,
                                                  const AnalysisConfig& config) {
    config_ = config;
    graph_ = graph;
    
    AnalysisResult result;
    
    if (!validateSeedNodes(graph_, seedNodeIds)) {
        result.errorMessage = "Invalid seed nodes provided";
        return result;
    }
    
    if (seedNodeIds.size() < 2) {
        result.errorMessage = "At least 2 seed nodes are required";
        return result;
    }
    
    const std::set<int> seedSetKey(seedNodeIds.begin(), seedNodeIds.end());
    const PathManager* pathManagerPtr = nullptr;
    
    auto cacheIt = pathManagerCache_.find(seedSetKey);

    if (cacheIt != pathManagerCache_.end()) {
        pathManagerPtr = &cacheIt->second;
    } else {
        PathManager newPathManager(graph_, seedNodeIds);
        auto result = pathManagerCache_.emplace(seedSetKey, newPathManager);
        pathManagerPtr = &result.first->second;
    }

    return runAnalysisWithPrecalculatedPaths(graph, seedNodeIds, config, const_cast<PathManager&>(*pathManagerPtr));
}

/**
 * @brief Run the analysis once the PathManager is available.
 */
AnalysisResult BasicNetworkFinder::runAnalysisWithPrecalculatedPaths(
    const Graph& graph,
    const std::vector<int>& seedNodeIds,
    const AnalysisConfig& config,
    PathManager& pathManager)
{
    config_ = config;
    
    AnalysisResult result;
    
    if (!validateSeedNodes(graph, seedNodeIds)) {
        result.errorMessage = "Invalid seed nodes provided";
        return result;
    }
    
    if (seedNodeIds.size() < 2) {
        result.errorMessage = "At least 2 seed nodes are required";
        return result;
    }

    if (!pathManager.getTotalPathCount()) {
        result.errorMessage = "PathManager provided is invalid or has no paths.";
        return result;
    }

    bool useParallel = shouldUseParallelExecution(graph, seedNodeIds, config_);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    BasicNetworkResult basicResult;
    
    bool needAllResults = (config_.enableStatisticalAnalysis && config_.numPermutations > 1);
    
    if (config_.enablePruning && config_.numPermutations == 1) {
        config_.numPermutations = 10;
    }
    
    bool willRunAllResults = (needAllResults && !useParallel);
    bool willRunAllResultsParallel = (needAllResults && useParallel);
    
    if (willRunAllResults) {
        BasicSpannerConfig engineConfig;
        engineConfig.numPermutations = config_.numPermutations;
        engineConfig.enablePruning = config_.enablePruning;
        engineConfig.randomSeed = config_.randomSeed;
        engineConfig.enableProgressCallback = (progressCallback_ != nullptr);
        engineConfig.collectAllResults = true;
        engineConfig.randomPruning = config_.randomPruning;
        
        AllPermutationResults allResults = engine_.findBasicNetworkWithAllResults(graph, seedNodeIds, pathManager, engineConfig);
        
        if (allResults.bestResult.isValid) {
            basicResult.isValid = allResults.bestResult.isValid;
            basicResult.isTiedNetwork = allResults.bestResult.isTiedNetwork;
            basicResult.seedNodes = allResults.bestResult.seedNodes;
            basicResult.connectorNodes = allResults.bestResult.connectorNodes;
            basicResult.allBasicUnits = allResults.bestResult.allBasicUnits;
            basicResult.totalConnectors = allResults.bestResult.totalConnectors;
            basicResult.basicNetwork_ptr = std::move(allResults.bestResult.basicNetwork_ptr);
            basicResult.prunedNetworks = std::move(allResults.bestResult.prunedNetworks);
            
            result.usedParallelExecution = false;
            result.threadsUsed = 1;
            result.parallelSpeedup = 1.0;
            
            if (allResults.hasMultipleResults || config_.numPermutations > 1) {
                result.allPermutationResults = std::move(allResults.allResults);
                result.hasDetailedResults = true;
            }

            if (allResults.hasMultipleResults && config_.enableStatisticalAnalysis) {
                result.statisticalResult = performStatisticalAnalysis(allResults.bestResult, allResults.allResults, config_);
                result.hasStatisticalAnalysis = true;
            }
        }
    }
    else if (willRunAllResultsParallel) {
        ParallelBasicSpannerConfig parallelConfig;
        parallelConfig.numPermutations = config_.numPermutations;
        parallelConfig.enablePruning = config_.enablePruning;
        parallelConfig.randomSeed = config_.randomSeed;
        parallelConfig.enableProgressCallback = (progressCallback_ != nullptr);
        parallelConfig.enableParallelCalculations = config_.enableParallelCalculations;
        parallelConfig.enableParallelPermutations = config_.enableParallelPermutations;
        parallelConfig.maxThreads = config_.maxThreads;
        parallelConfig.enablePerformanceMonitoring = config_.enablePerformanceMonitoring;
        parallelConfig.randomPruning = config_.randomPruning;
        
        AllParallelPermutationResults allParallelResults = parallelEngine_.findBasicNetworkParallelWithAllResults(graph, seedNodeIds, pathManager, parallelConfig);
        
        if (allParallelResults.bestResult.isValid) {
            result.usedParallelExecution = true;
            result.threadsUsed = allParallelResults.bestResult.threadsUsed;
            result.parallelSpeedup = allParallelResults.bestResult.totalSpeedup;

            basicResult = std::move(allParallelResults.bestResult);
            
            if (allParallelResults.hasMultipleResults || config_.numPermutations > 1) {
                result.allPermutationResults = std::move(allParallelResults.allResults);
                result.hasDetailedResults = true;
            }

            if (allParallelResults.hasMultipleResults && config_.enableStatisticalAnalysis) {
                result.statisticalResult = performStatisticalAnalysis(basicResult, allParallelResults.allResults, config_);
                result.hasStatisticalAnalysis = true;
            }
        }
    }
    else if (config_.enablePruning && config_.numPermutations > 1 && !config_.enableStatisticalAnalysis) {
        if (useParallel) {
            ParallelBasicSpannerConfig parallelConfig;
            parallelConfig.numPermutations = config_.numPermutations;
            parallelConfig.enablePruning = config_.enablePruning;
            parallelConfig.randomSeed = config_.randomSeed;
            parallelConfig.enableProgressCallback = (progressCallback_ != nullptr);
            parallelConfig.enableParallelCalculations = config_.enableParallelCalculations;
            parallelConfig.enableParallelPermutations = config_.enableParallelPermutations;
            parallelConfig.maxThreads = config_.maxThreads;
            parallelConfig.enablePerformanceMonitoring = config_.enablePerformanceMonitoring;
            parallelConfig.randomPruning = config_.randomPruning;
            
            AllParallelPermutationResults allParallelResults = parallelEngine_.findBasicNetworkParallelWithAllResults(graph, seedNodeIds, pathManager, parallelConfig);
            
            if (allParallelResults.bestResult.isValid) {
                result.usedParallelExecution = true;
                result.threadsUsed = allParallelResults.bestResult.threadsUsed;
                result.parallelSpeedup = allParallelResults.bestResult.totalSpeedup;

                basicResult = std::move(allParallelResults.bestResult);
                
                if (allParallelResults.hasMultipleResults || config_.numPermutations > 1) {
                    result.allPermutationResults = std::move(allParallelResults.allResults);
                    result.hasDetailedResults = true;
                }
            }
        }
        else {
            BasicSpannerConfig engineConfig;
            engineConfig.numPermutations = config_.numPermutations;
            engineConfig.enablePruning = config_.enablePruning;
            engineConfig.randomSeed = config_.randomSeed;
            engineConfig.enableProgressCallback = (progressCallback_ != nullptr);
            engineConfig.collectAllResults = true;
            engineConfig.randomPruning = config_.randomPruning;
            
            AllPermutationResults allResults = engine_.findBasicNetworkWithAllResults(graph, seedNodeIds, pathManager, engineConfig);
            
            if (allResults.bestResult.isValid) {
                basicResult.isValid = allResults.bestResult.isValid;
                basicResult.isTiedNetwork = allResults.bestResult.isTiedNetwork;
                basicResult.seedNodes = allResults.bestResult.seedNodes;
                basicResult.connectorNodes = allResults.bestResult.connectorNodes;
                basicResult.allBasicUnits = allResults.bestResult.allBasicUnits;
                basicResult.totalConnectors = allResults.bestResult.totalConnectors;
                basicResult.basicNetwork_ptr = std::move(allResults.bestResult.basicNetwork_ptr);
                basicResult.prunedNetworks = std::move(allResults.bestResult.prunedNetworks);
            
                result.usedParallelExecution = false;
                result.threadsUsed = 1;
                result.parallelSpeedup = 1.0;
                
                if (config_.saveDetailedResults || config_.numPermutations > 1) {
                    result.allPermutationResults = std::move(allResults.allResults);
                    result.hasDetailedResults = true;
                }
            }
        }
    }
    else if (useParallel) {
        
        ParallelBasicSpannerConfig parallelConfig;
        parallelConfig.numPermutations = config_.numPermutations;
        parallelConfig.enablePruning = config_.enablePruning;
        parallelConfig.randomSeed = config_.randomSeed;
        parallelConfig.enableProgressCallback = (progressCallback_ != nullptr);
        parallelConfig.enableParallelCalculations = config_.enableParallelCalculations;
        parallelConfig.enableParallelPermutations = config_.enableParallelPermutations;
        parallelConfig.maxThreads = config_.maxThreads;
        parallelConfig.enablePerformanceMonitoring = config_.enablePerformanceMonitoring;
        parallelConfig.randomPruning = config_.randomPruning;
        
        if (config_.numPermutations > 1) {
            AllParallelPermutationResults allParallelResults = parallelEngine_.findBasicNetworkParallelWithAllResults(graph, seedNodeIds, pathManager, parallelConfig);
            
            if (allParallelResults.bestResult.isValid) {
                result.usedParallelExecution = true;
                result.threadsUsed = allParallelResults.bestResult.threadsUsed;
                result.parallelSpeedup = allParallelResults.bestResult.totalSpeedup;
                
                basicResult = std::move(allParallelResults.bestResult);
                
                if (allParallelResults.hasMultipleResults || config_.numPermutations > 1) {
                    result.allPermutationResults = std::move(allParallelResults.allResults);
                    result.hasDetailedResults = true;
                }
            }
        } else {
            ParallelBasicSpannerResult parallelResult = parallelEngine_.findBasicNetworkParallel(graph, seedNodeIds, pathManager, parallelConfig);
            
            basicResult.isValid = parallelResult.isValid;
            basicResult.isTiedNetwork = parallelResult.isTiedNetwork;
            basicResult.seedNodes = parallelResult.seedNodes;
            basicResult.connectorNodes = parallelResult.connectorNodes;
            basicResult.allBasicUnits = parallelResult.allBasicUnits;
            basicResult.basicNetwork_ptr = std::move(parallelResult.basicNetwork_ptr);
            basicResult.totalConnectors = parallelResult.totalConnectors;
            basicResult.prunedNetworks = std::move(parallelResult.prunedNetworks);
            
            result.usedParallelExecution = true;
            result.threadsUsed = parallelResult.threadsUsed;
            result.parallelSpeedup = parallelResult.totalSpeedup;
        }
        
    } else {
        
        BasicSpannerConfig engineConfig;
        engineConfig.numPermutations = config_.numPermutations;
        engineConfig.enablePruning = config_.enablePruning;
        engineConfig.randomSeed = config_.randomSeed;
        engineConfig.enableProgressCallback = (progressCallback_ != nullptr);
        engineConfig.randomPruning = config_.randomPruning;
        
        if (config_.numPermutations > 1) {
            engineConfig.collectAllResults = true;
            
            AllPermutationResults allResults = engine_.findBasicNetworkWithAllResults(graph, seedNodeIds, pathManager, engineConfig);
            
            if (allResults.bestResult.isValid) {
                basicResult.isValid = allResults.bestResult.isValid;
                basicResult.isTiedNetwork = allResults.bestResult.isTiedNetwork;
                basicResult.seedNodes = allResults.bestResult.seedNodes;
                basicResult.connectorNodes = allResults.bestResult.connectorNodes;
                basicResult.allBasicUnits = allResults.bestResult.allBasicUnits;
                basicResult.totalConnectors = allResults.bestResult.totalConnectors;
                basicResult.basicNetwork_ptr = std::move(allResults.bestResult.basicNetwork_ptr);
                basicResult.prunedNetworks = std::move(allResults.bestResult.prunedNetworks);
                
                if (config_.saveDetailedResults || config_.numPermutations > 1) {
                    result.allPermutationResults = std::move(allResults.allResults);
                    result.hasDetailedResults = true;
                }
            }
        } else {
            basicResult = engine_.findBasicNetwork(graph, seedNodeIds, engineConfig);
        }
        
        result.usedParallelExecution = false;
        result.threadsUsed = 1;
        result.parallelSpeedup = 1.0;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    double executionTime = duration.count() / 1000.0;
    
    if (basicResult.isValid) {
        bool hadStatisticalAnalysis = result.hasStatisticalAnalysis;
        StatisticalAnalysisResult savedStatisticalResult = result.statisticalResult;
        bool hadDetailedResults = result.hasDetailedResults;
        std::vector<BasicNetworkResult> savedAllResults = std::move(result.allPermutationResults);
        bool wasParallelExecution = result.usedParallelExecution;
        int usedThreads = result.threadsUsed;
        double achievedSpeedup = result.parallelSpeedup;

        result = calculateStatistics(graph, basicResult, executionTime);
        result.success = true;
        result.basicNetworkResult = basicResult;

        result.hasStatisticalAnalysis = hadStatisticalAnalysis;
        result.statisticalResult = savedStatisticalResult;
        result.hasDetailedResults = hadDetailedResults;
        result.allPermutationResults = std::move(savedAllResults);
        result.usedParallelExecution = wasParallelExecution;
        result.threadsUsed = usedThreads;
        result.parallelSpeedup = achievedSpeedup;

        result.connectorCount = basicResult.connectorNodes.size();
        result.seedCount = basicResult.seedNodes.size();
        
        int totalBasicNodes = basicResult.allBasicUnits.size();
        if (result.originalNodeCount > 0) {
            result.reductionPercentage = 
                ((double)(result.originalNodeCount - totalBasicNodes) / 
                 result.originalNodeCount) * 100.0;
        }
        
        result.permutationStats = basicResult.permutationStats;

        if (config_.saveDetailedResults) {
            createOutputDirectory(config_.outputDirectory);
            std::string timestamp = generateTimestamp();
            saveResults(result, config_.outputDirectory + "/analysis_" + timestamp + ".txt");
            saveBasicNetworkGraph(result, config_.outputDirectory + "/basic_network_" + timestamp + ".txt");

            if (result.hasDetailedResults && !result.allPermutationResults.empty()) {
                std::ofstream detailedFile(config_.outputDirectory + "/detailed_permutations_" + timestamp + ".txt");
                if (detailedFile.is_open()) {
                    formatAndSaveDetailedPermutationResults(result.allPermutationResults, detailedFile);
                    detailedFile.close();
                }
            }
        }
    } else {
        result.errorMessage = "Algorithm failed to find a valid basic network";
    }
    
    return result;
}

void BasicNetworkFinder::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
}

void BasicNetworkFinder::setDetailedProgressCallback(DetailedProgressCallback callback) {
    detailedProgressCallback_ = callback;
}

void BasicNetworkFinder::setPermutationCompletedCallback(PermutationCompletedCallback callback) {
    permutationCompletedCallback_ = callback;
}

void BasicNetworkFinder::setStopRequestCallback(StopRequestCallback callback) {
    stopRequestCallback_ = callback;
}

void BasicNetworkFinder::setConfig(const AnalysisConfig& config) {
    config_ = config;
}

bool BasicNetworkFinder::loadGraph(const std::string& filename) {
    pathManagerCache_.clear();
    return graph_.loadFromFile(filename);
}

bool BasicNetworkFinder::loadGraphWithColumns(const std::string& filename, 
                                             int sourceColumn, int targetColumn,
                                             const std::string& separator,
                                             bool hasHeader, int skipLines,
                                             int nodeNameColumn,
                                             bool applyNamesToTarget,
                                             bool caseSensitive) {
    pathManagerCache_.clear();
    return graph_.loadFromFileWithColumns(filename, sourceColumn, targetColumn, 
                                         separator, hasHeader, skipLines, nodeNameColumn, 
                                         applyNamesToTarget, caseSensitive);
}

bool BasicNetworkFinder::saveResults(const AnalysisResult& result, const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << formatResults(result);
    file.close();
    return true;
}

bool BasicNetworkFinder::saveBasicNetworkGraph(const AnalysisResult& result, const std::string& filename) const {
    if (!result.success) {
        return false;
    }
    
    if (result.basicNetworkResult.basicNetwork_ptr) {
        return result.basicNetworkResult.basicNetwork_ptr->saveToFile(filename);
    } else {
        std::cerr << "Error: Attempted to save basic network graph, but no graph data is available." << std::endl;
        return false;
    }
}

bool BasicNetworkFinder::validateSeedNodes(const Graph& graph, const std::vector<int>& seedIds) {
    for (int seedId : seedIds) {
        if (!graph.hasNode(seedId)) {
            return false;
        }
    }
    return true;
}

bool BasicNetworkFinder::validateSeedNodes(const Graph& graph, const std::vector<std::string>& seedNames) {
    for (const std::string& seedName : seedNames) {
        if (!graph.hasNode(seedName)) {
            return false;
        }
    }
    return true;
}

std::vector<int> BasicNetworkFinder::convertSeedNamesToIds(const std::vector<std::string>& seedNames) const {
    std::vector<int> seedIds;
    
    for (const std::string& name : seedNames) {
        int id = graph_.getNodeId(name);
        if (id != -1) {
            seedIds.push_back(id);
        }
    }
    
    return seedIds;
}

std::vector<std::string> BasicNetworkFinder::convertSeedIdsToNames(const std::vector<int>& seedIds) const {
    std::vector<std::string> seedNames;
    
    for (int id : seedIds) {
        seedNames.push_back(graph_.getNodeName(id));
    }
    
    return seedNames;
}

AnalysisResult BasicNetworkFinder::calculateStatistics(const Graph& originalGraph,
                                                      const BasicNetworkResult& result,
                                                      double executionTime) {
    AnalysisResult analysisResult;
    
    analysisResult.originalNodeCount = originalGraph.getNodeCount();
    analysisResult.originalEdgeCount = originalGraph.getEdgeCount();
    analysisResult.seedCount = result.seedNodes.size();
    analysisResult.connectorCount = result.connectorNodes.size();
    analysisResult.executionTimeSeconds = executionTime;
    
    int totalBasicNodes = result.allBasicUnits.size();
    if (analysisResult.originalNodeCount > 0) {
        analysisResult.reductionPercentage = 
            ((double)(analysisResult.originalNodeCount - totalBasicNodes) / 
             analysisResult.originalNodeCount) * 100.0;
    }
    
    return analysisResult;
}

void BasicNetworkFinder::onEngineProgress(int permutation, int total, const std::string& status) {
    if (progressCallback_) {
        progressCallback_(permutation, total, status);
    }
}

void BasicNetworkFinder::onEngineDetailedProgress(int permutation, int totalPermutations, 
                                                 const std::string& stage, int stageProgress, int stageTotal) {
    if (detailedProgressCallback_) {
        detailedProgressCallback_(permutation, totalPermutations, stage, stageProgress, stageTotal);
    }
}

void BasicNetworkFinder::onEnginePermutationCompleted(int permutation, int totalPermutations, int connectorsFound, const std::vector<int>& connectorIds, int preConnectorsFound) {
    if (permutationCompletedCallback_) {
        permutationCompletedCallback_(permutation, totalPermutations, connectorsFound, connectorIds, preConnectorsFound);
    }
}

bool BasicNetworkFinder::createOutputDirectory(const std::string& directory) const {
    try {
        std::filesystem::create_directories(directory);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create output directory: " << e.what() << std::endl;
        return false;
    }
}

std::string BasicNetworkFinder::generateTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string BasicNetworkFinder::formatResults(const AnalysisResult& result) const {
    std::stringstream ss;
    
    ss << "Basic Network Analysis Results\n";
    ss << "==============================\n\n";
    
    if (result.success) {
        ss << "Analysis: SUCCESS\n";
        ss << "Execution Time: " << std::fixed << std::setprecision(2) 
           << result.executionTimeSeconds << " seconds\n";
        ss << "Execution Mode: " << (result.usedParallelExecution ? "PARALLEL" : "SERIAL");
        if (result.usedParallelExecution) {
            ss << " (" << result.threadsUsed << " threads, " 
               << std::fixed << std::setprecision(2) << result.parallelSpeedup << "x speedup)";
        }
        ss << "\n\n";
        
        ss << "Graph Statistics:\n";
        ss << "  Original Nodes: " << result.originalNodeCount << "\n";
        ss << "  Original Edges: " << result.originalEdgeCount << "\n";
        ss << "  Seed Nodes: " << result.seedCount << "\n";
        ss << "  Connector Nodes: " << result.connectorCount << "\n";
        ss << "  Total Basic Units: " << (result.seedCount + result.connectorCount) << "\n";
        ss << "  Reduction: " << std::fixed << std::setprecision(1) 
           << result.reductionPercentage << "%\n\n";
        
        ss << "Seed Nodes:\n";
        for (int seedId : result.basicNetworkResult.seedNodes) {
            ss << "  " << graph_.getNodeName(seedId) << " (ID: " << seedId << ")\n";
        }
        
        ss << "\nConnector Nodes:\n";
        for (int connectorId : result.basicNetworkResult.connectorNodes) {
            ss << "  " << graph_.getNodeName(connectorId) << " (ID: " << connectorId << ")\n";
        }
        
        if (result.basicNetworkResult.isTiedNetwork) {
            ss << "\nNote: This is a tied basic network with " 
               << result.basicNetworkResult.prunedNetworks.size() 
               << " possible pruned alternatives.\n";
        }
        
        if (result.hasStatisticalAnalysis) {
            ss << formatStatisticalResults(result.statisticalResult);
        }
    } else {
        ss << "Analysis: FAILED\n";
        ss << "Error: " << result.errorMessage << "\n";
    }
    
    return ss.str();
}

bool BasicNetworkFinder::shouldUseParallelExecution(const Graph& graph, const std::vector<int>& seedIds, const AnalysisConfig& config) const {
    if (!config.enableParallelExecution) {
        return false;
    }

    if (graph.getNodeCount() < 30 || seedIds.size() < 2 || config.numPermutations < 1) {
        return false;
    }

    if (config.maxThreads <= 1) {
        return false;
    }

    return true;
}

NetworkStatistics BasicNetworkFinder::calculateNetworkStatistics(const std::vector<int>& connectorCounts) {
    NetworkStatistics stats;
    
    if (connectorCounts.empty()) {
        return stats;
    }
    
    stats.allValues = connectorCounts;
    
    stats.minimum = *std::min_element(connectorCounts.begin(), connectorCounts.end());
    stats.maximum = *std::max_element(connectorCounts.begin(), connectorCounts.end());
    
    double sum = 0.0;
    for (int count : connectorCounts) {
        sum += count;
    }
    stats.mean = sum / connectorCounts.size();
    
    double variance = 0.0;
    for (int count : connectorCounts) {
        variance += (count - stats.mean) * (count - stats.mean);
    }
    variance /= connectorCounts.size();
    stats.standardDeviation = std::sqrt(variance);
    
    return stats;
}

double BasicNetworkFinder::calculateZScore(int experimentalValue, const NetworkStatistics& randomStats) {
    if (randomStats.standardDeviation == 0.0) {
        return 0.0;
    }
    
    return std::abs(experimentalValue - randomStats.mean) / randomStats.standardDeviation;
}

StatisticalAnalysisResult BasicNetworkFinder::performStatisticalAnalysis(
    const BasicNetworkResult& experimentalResult,
    const std::vector<BasicNetworkResult>& randomResults,
    const AnalysisConfig& config) {
    
    StatisticalAnalysisResult stats;
    
    stats.goTermName = config.goTermName;
    stats.goDomain = config.goDomain;
    stats.numSeedNodes = experimentalResult.seedNodes.size();
    stats.numRandomPermutations = randomResults.size();
    
    stats.experimentalTiedConnectors = experimentalResult.totalConnectors;
    stats.experimentalPrunedConnectors = experimentalResult.prunedNetworks.empty() ? 
        experimentalResult.totalConnectors : 
        (experimentalResult.prunedNetworks[0].basicNetwork_ptr ? experimentalResult.prunedNetworks[0].totalConnectors : experimentalResult.totalConnectors);
    
    std::vector<int> randomTiedCounts;
    std::vector<int> randomPrunedCounts;
    
    for (const auto& result : randomResults) {
        if (result.isValid) {
            randomTiedCounts.push_back(result.totalConnectors);
            
            int prunedCount = result.prunedNetworks.empty() ? 
                result.totalConnectors : 
                (result.prunedNetworks[0].basicNetwork_ptr ? result.prunedNetworks[0].totalConnectors : result.totalConnectors);
            randomPrunedCounts.push_back(prunedCount);
        }
    }
    
    stats.randomTiedStatistics = calculateNetworkStatistics(randomTiedCounts);
    
    stats.randomPrunedStatistics = calculateNetworkStatistics(randomPrunedCounts);
    
    stats.tiedZScore = calculateZScore(stats.experimentalTiedConnectors, stats.randomTiedStatistics);
    stats.prunedZScore = calculateZScore(stats.experimentalPrunedConnectors, stats.randomPrunedStatistics);
    
    return stats;
}

std::string BasicNetworkFinder::formatStatisticalResults(const StatisticalAnalysisResult& stats) const {
    std::stringstream ss;
    
    ss << "\n=== Statistical Analysis Results ===\n";
    ss << "GO Term: " << stats.goTermName << "\n";
    ss << "Domain: " << stats.goDomain << "\n";
    ss << "Number of Seeds: " << stats.numSeedNodes << "\n";
    ss << "Random Permutations: " << stats.numRandomPermutations << "\n\n";
    
    ss << "Experimental Network:\n";
    ss << "  Tied Connectors: " << stats.experimentalTiedConnectors << "\n";
    ss << "  Pruned Connectors: " << stats.experimentalPrunedConnectors << "\n\n";
    
    ss << "Random Networks (Tied):\n";
    ss << "  Mean ± SD: " << std::fixed << std::setprecision(2) 
       << stats.randomTiedStatistics.mean << " ± " << stats.randomTiedStatistics.standardDeviation;
    ss << " (Min: " << stats.randomTiedStatistics.minimum << ")\n";
    ss << "  Z-score: " << std::fixed << std::setprecision(3) << stats.tiedZScore << "\n\n";
    
    ss << "Random Networks (Pruned):\n";
    ss << "  Mean ± SD: " << std::fixed << std::setprecision(2) 
       << stats.randomPrunedStatistics.mean << " ± " << stats.randomPrunedStatistics.standardDeviation;
    ss << " (Min: " << stats.randomPrunedStatistics.minimum << ")\n";
    ss << "  Z-score: " << std::fixed << std::setprecision(3) << stats.prunedZScore << "\n\n";
    
    ss << "=== Summary Table ===\n";
    ss << "Term\tDomain\tSeeds\tExp_Tied\tExp_Pruned\tRandom_Tied\tRandom_Pruned\tZ_Tied\tZ_Pruned\n";
    ss << stats.goTermName << "\t" << stats.goDomain << "\t" << stats.numSeedNodes << "\t"
       << stats.experimentalTiedConnectors << "\t" << stats.experimentalPrunedConnectors << "\t"
       << std::fixed << std::setprecision(1) << stats.randomTiedStatistics.mean 
       << "±" << stats.randomTiedStatistics.standardDeviation 
       << "(" << stats.randomTiedStatistics.minimum << ")\t"
       << stats.randomPrunedStatistics.mean 
       << "±" << stats.randomPrunedStatistics.standardDeviation 
       << "(" << stats.randomPrunedStatistics.minimum << ")\t"
       << std::fixed << std::setprecision(2) << stats.tiedZScore << "\t" << stats.prunedZScore << "\n";
    
    return ss.str();
}

bool BasicNetworkFinder::formatAndSaveDetailedPermutationResults(
    const std::vector<BasicNetworkResult>& allResults, 
    std::ofstream& detailedFile) const 
{
    if (!detailedFile.is_open()) {
        std::cerr << "ERROR: Detailed results file stream not open in formatAndSaveDetailedPermutationResults." << std::endl;
        return false;
    }

    detailedFile << "\\n=== DETAILED PERMUTATION RESULTS ===\\n";
    detailedFile << "Total permutations executed: " << allResults.size() << "\\n\\n";
    
    for (size_t i = 0; i < allResults.size(); ++i) {
        const auto& result = allResults[i];
        detailedFile << "--- Permutation " << (i + 1) << " ---\\n";
        
        detailedFile << "Permutation Number: " << result.permutationNumber << "\\n";
        detailedFile << "Random Seed Used: " << result.randomSeedUsed << "\\n";
        detailedFile << "Valid: " << (result.isValid ? "YES" : "NO") << "\\n";
        
        detailedFile << "Seed Nodes Order: ";
        bool first = true;
        for (int seedId : result.seedNodesOrder) {
            if (!first) detailedFile << ", ";
            detailedFile << (graph_.hasNode(seedId) ? graph_.getNodeName(seedId) : "N/A") << "(" << seedId << ")";
            first = false;
        }
        detailedFile << "\\n";
        
        detailedFile << "Seed Pairs Order: ";
        first = true;
        for (const int& pairIndex : result.seedPairsOrder) {
            if (!first) detailedFile << ", ";
            detailedFile << "Index_" << pairIndex;
            first = false;
        }
        detailedFile << "\\n";
        
        if (result.isValid) {
            detailedFile << "Total Connectors: " << result.totalConnectors << "\\n";
            detailedFile << "Tied Network: " << (result.isTiedNetwork ? "YES" : "NO") << "\\n";
            
            detailedFile << "Connector Nodes: ";
            first = true;
            for (int connectorId : result.connectorNodes) {
                if (!first) detailedFile << ", ";
                detailedFile << (graph_.hasNode(connectorId) ? graph_.getNodeName(connectorId) : "N/A") << "(" << connectorId << ")";
                first = false;
            }
            detailedFile << "\\n";
            
            if (!result.prunedNetworks.empty()) {
                detailedFile << "Pruned alternatives: " << result.prunedNetworks.size() << "\\n";
                detailedFile << "Best pruned connectors: " << result.prunedNetworks[0].totalConnectors << "\\n";
            }
        }
        
        if (!result.debugInfo.empty()) {
            detailedFile << "Debug Info: " << result.debugInfo << "\\n";
        }
        
        detailedFile << "\\n";

        if ((i + 1) % 500 == 0) {
            detailedFile.flush();
        }
    }
    
    if (!allResults.empty()) {
        std::vector<int> connectorCounts;
        for (const auto& result : allResults) {
            if (result.isValid) {
                connectorCounts.push_back(result.totalConnectors);
            }
        }
        
        if (!connectorCounts.empty()) {
            int minConnectors = *std::min_element(connectorCounts.begin(), connectorCounts.end());
            int maxConnectors = *std::max_element(connectorCounts.begin(), connectorCounts.end());
            double avgConnectors = 0.0;
            for (int count : connectorCounts) {
                avgConnectors += count;
            }
            avgConnectors /= connectorCounts.size();
            
            detailedFile << "=== PERMUTATION STATISTICS ===\\n";
            detailedFile << "Valid permutations: " << connectorCounts.size() << "/" << allResults.size() << "\\n";
            detailedFile << "Connectors - Min: " << minConnectors << ", Max: " << maxConnectors 
                       << ", Average: " << std::fixed << std::setprecision(2) << avgConnectors << "\\n";
            
            int optimalCount = 0;
            for (int count : connectorCounts) {
                if (count == minConnectors) optimalCount++;
            }
            detailedFile << "Permutations finding optimal solution: " << optimalCount << "/" << connectorCounts.size() 
                       << " (" << std::fixed << std::setprecision(1) << (100.0 * optimalCount / connectorCounts.size()) << "%)\\n";
        }
    }
    return true;
}

std::string BasicNetworkFinder::formatDetailedPermutationResults(const std::vector<BasicNetworkResult>& allResults) const {
    std::stringstream ss;
    
    ss << "\\n=== DETAILED PERMUTATION RESULTS ===\\n";
    ss << "Total permutations executed: " << allResults.size() << "\\n\\n";
    
    for (size_t i = 0; i < allResults.size(); ++i) {
        const auto& result = allResults[i];
        ss << "--- Permutation " << (i + 1) << " ---\\n";
        ss << "Valid: " << (result.isValid ? "YES" : "NO") << "\\n";
        
        if (result.isValid) {
            ss << "Total Connectors: " << result.totalConnectors << "\\n";
            ss << "Tied Network: " << (result.isTiedNetwork ? "YES" : "NO") << "\\n";
            
            ss << "Connector Nodes: ";
            bool first = true;
            for (int connectorId : result.connectorNodes) {
                if (!first) ss << ", ";
                ss << (graph_.hasNode(connectorId) ? graph_.getNodeName(connectorId) : "N/A") << "(" << connectorId << ")";
                first = false;
            }
            ss << "\\n";
            
            if (!result.prunedNetworks.empty()) {
                ss << "Pruned alternatives: " << result.prunedNetworks.size() << "\\n";
                ss << "Best pruned connectors: " << result.prunedNetworks[0].totalConnectors << "\\n";
            }
        }
        ss << "\\n";
    }
    
    if (!allResults.empty()) {
        std::vector<int> connectorCounts;
        for (const auto& result : allResults) {
            if (result.isValid) {
                connectorCounts.push_back(result.totalConnectors);
            }
        }
        
        if (!connectorCounts.empty()) {
            int minConnectors = *std::min_element(connectorCounts.begin(), connectorCounts.end());
            int maxConnectors = *std::max_element(connectorCounts.begin(), connectorCounts.end());
            double avgConnectors = 0.0;
            for (int count : connectorCounts) {
                avgConnectors += count;
            }
            avgConnectors /= connectorCounts.size();
            
            ss << "=== PERMUTATION STATISTICS ===\\n";
            ss << "Valid permutations: " << connectorCounts.size() << "/" << allResults.size() << "\\n";
            ss << "Connectors - Min: " << minConnectors << ", Max: " << maxConnectors 
               << ", Average: " << std::fixed << std::setprecision(2) << avgConnectors << "\\n";
            
            int optimalCount = 0;
            for (int count : connectorCounts) {
                if (count == minConnectors) optimalCount++;
            }
            ss << "Permutations finding optimal solution: " << optimalCount << "/" << connectorCounts.size() 
               << " (" << std::fixed << std::setprecision(1) << (100.0 * optimalCount / connectorCounts.size()) << "%)\\n";
        }
    }
    
    return ss.str();
} 

void BatchAnalysisResult::calculateStatistics() {
    try {
        if (allBatchResults.empty()) {
            return;
        }
    
    totalBatches = allBatchResults.size();
    successfulBatches = 0;
    failedBatches = 0;
    cumulativeBatchTime = 0.0;
    minExecutionTime = std::numeric_limits<double>::max();
    maxExecutionTime = 0.0;
    
    std::vector<int> connectorCounts;
    std::vector<int> nodeCounts;
    std::vector<int> edgeCounts;
    std::vector<double> executionTimes;
    
    int bestConnectorCount = std::numeric_limits<int>::max();
    int bestResultIndex = -1;
    
    for (size_t i = 0; i < allBatchResults.size(); ++i) {
        try {
            const auto& result = allBatchResults[i];
            
            if (result.success && result.basicNetworkResult.isValid) {
            successfulBatches++;
            
            int connectorCount = result.basicNetworkResult.connectorNodes.size();
            connectorCounts.push_back(connectorCount);
            
            if (connectorCount < bestConnectorCount) {
                bestConnectorCount = connectorCount;
                bestResultIndex = i;
            }
            
            
            executionTimes.push_back(result.executionTimeSeconds);
            cumulativeBatchTime += result.executionTimeSeconds;
            
            if (result.executionTimeSeconds < minExecutionTime) {
                minExecutionTime = result.executionTimeSeconds;
            }
            if (result.executionTimeSeconds > maxExecutionTime) {
                maxExecutionTime = result.executionTimeSeconds;
            }
        } else {
            failedBatches++;
        }
        } catch (...) {
            failedBatches++;
        }
    }
    
    if (bestResultIndex >= 0) {
        bestResult = std::move(allBatchResults[bestResultIndex]);
    }
    
    successRate = (totalBatches > 0) ? (static_cast<double>(successfulBatches) / totalBatches) * 100.0 : 0.0;
    
    averageExecutionTime = (successfulBatches > 0) ? cumulativeBatchTime / successfulBatches : 0.0;
    
    if (!connectorCounts.empty()) {
        try {
            connectorStatistics = BasicNetworkFinder::calculateNetworkStatistics(connectorCounts);
        } catch (...) {
        }
    }
    
    
    
    if (!allBatchResultsWithoutPruning.empty()) {
        std::vector<int> connectorCountsWithoutPruning;
        int bestConnectorCountWithoutPruning = std::numeric_limits<int>::max();
        int bestResultIndexWithoutPruning = -1;
        
        for (size_t i = 0; i < allBatchResultsWithoutPruning.size(); ++i) {
            const auto& result = allBatchResultsWithoutPruning[i];
            if (result.success && result.basicNetworkResult.isValid) {
                int connectorCount = result.basicNetworkResult.connectorNodes.size();
                connectorCountsWithoutPruning.push_back(connectorCount);
                
                if (connectorCount < bestConnectorCountWithoutPruning) {
                    bestConnectorCountWithoutPruning = connectorCount;
                    bestResultIndexWithoutPruning = i;
                }
            }
        }
        
        if (!connectorCountsWithoutPruning.empty()) {
            connectorStatisticsWithoutPruning = BasicNetworkFinder::calculateNetworkStatistics(connectorCountsWithoutPruning);
        }
        
        if (bestResultIndexWithoutPruning >= 0) {
            bestResultWithoutPruning = std::move(allBatchResultsWithoutPruning[bestResultIndexWithoutPruning]);
        }
    }
    
    if (!allBatchResultsWithPruning.empty()) {
        std::vector<int> connectorCountsWithPruning;
        int bestConnectorCountWithPruning = std::numeric_limits<int>::max();
        int bestResultIndexWithPruning = -1;
        
        for (size_t i = 0; i < allBatchResultsWithPruning.size(); ++i) {
            const auto& result = allBatchResultsWithPruning[i];
            if (result.success && result.basicNetworkResult.isValid) {
                int connectorCount = result.basicNetworkResult.connectorNodes.size();
                connectorCountsWithPruning.push_back(connectorCount);
                
                if (connectorCount < bestConnectorCountWithPruning) {
                    bestConnectorCountWithPruning = connectorCount;
                    bestResultIndexWithPruning = i;
                }
            }
        }
        
        if (!connectorCountsWithPruning.empty()) {
            connectorStatisticsWithPruning = BasicNetworkFinder::calculateNetworkStatistics(connectorCountsWithPruning);
        }
        
        if (bestResultIndexWithPruning >= 0) {
            bestResultWithPruning = std::move(allBatchResultsWithPruning[bestResultIndexWithPruning]);
        }
    }
    
    if (!allBatchResultsWithoutPruning.empty()) {
        for (size_t i = 0; i < allBatchResultsWithoutPruning.size(); ++i) {
            const auto& result = allBatchResultsWithoutPruning[i];
            if (result.success) {
                permutationStatsPerIterationWithoutPruning.push_back(result.permutationStats);
            }
        }
        permutationDescriptiveStatsWithoutPruning.calculateFromVector(permutationStatsPerIterationWithoutPruning);
    }
    
    if (!allBatchResultsWithPruning.empty()) {
        for (size_t i = 0; i < allBatchResultsWithPruning.size(); ++i) {
            const auto& result = allBatchResultsWithPruning[i];
            if (result.success) {
                permutationStatsPerIterationWithPruning.push_back(result.permutationStats);
            }
        }
        permutationDescriptiveStatsWithPruning.calculateFromVector(permutationStatsPerIterationWithPruning);
    }
    
    for (size_t i = 0; i < allBatchResults.size(); ++i) {
        const auto& result = allBatchResults[i];
        if (result.success) {
            permutationStatsPerIteration.push_back(result.permutationStats);
        }
    }
    
    permutationDescriptiveStats.calculateFromVector(permutationStatsPerIteration);
    
    if (!allBatchResultsWithoutPruning.empty() && !allBatchResultsWithPruning.empty()) {
        for (size_t i = 0; i < allBatchResultsWithoutPruning.size() && i < allBatchResultsWithPruning.size(); ++i) {
            const auto& unprunedResult = allBatchResultsWithoutPruning[i];
            const auto& prunedResult = allBatchResultsWithPruning[i];
            
            if (unprunedResult.success && prunedResult.success) {
                TieResolutionDifferences diff(i, unprunedResult.basicNetworkResult.permutationNumber, prunedResult.basicNetworkResult.permutationNumber);
                
                diff.ties_difference = prunedResult.permutationStats.ties - unprunedResult.permutationStats.ties;
                diff.ties_resolved_difference = prunedResult.permutationStats.ties_resolved - unprunedResult.permutationStats.ties_resolved;
                diff.ties_maintained_difference = prunedResult.permutationStats.ties_maintained - unprunedResult.permutationStats.ties_maintained;
                diff.ties_essential_vs_non_essential_difference = prunedResult.permutationStats.ties_essential_vs_non_essential - unprunedResult.permutationStats.ties_essential_vs_non_essential;
                diff.ties_both_essential_difference = prunedResult.permutationStats.ties_both_essential - unprunedResult.permutationStats.ties_both_essential;
                diff.ties_neither_essential_difference = prunedResult.permutationStats.ties_neither_essential - unprunedResult.permutationStats.ties_neither_essential;
                
                tieResolutionDifferences.push_back(diff);
            }
        }
        
        tieResolutionDifferencesStats.calculateFromVector(tieResolutionDifferences);
    }
    
    if (!allBatchResultsWithoutPruning.empty() && !allBatchResultsWithPruning.empty()) {
        
        for (size_t i = 0; i < allBatchResultsWithoutPruning.size() && i < allBatchResultsWithPruning.size(); ++i) {
            const auto& unprunedResult = allBatchResultsWithoutPruning[i];
            const auto& prunedResult = allBatchResultsWithPruning[i];
            
            if (unprunedResult.success && prunedResult.success) {
                std::set<int> unprunedConnectors = unprunedResult.basicNetworkResult.connectorNodes;
                std::set<int> prunedConnectors = prunedResult.basicNetworkResult.connectorNodes;
                
                for (int node : unprunedConnectors) {
                    if (prunedConnectors.find(node) == prunedConnectors.end()) {
                        pruningNodeAnalysis.nodes_eliminated_by_pruning.insert(node);
                        
                        if (unprunedResult.permutationStats.nodes_in_ties.count(node)) {
                            pruningNodeAnalysis.eliminated_nodes_in_ties.insert(node);
                        }
                        if (unprunedResult.permutationStats.nodes_in_resolved_ties.count(node)) {
                            pruningNodeAnalysis.eliminated_nodes_in_resolved_ties.insert(node);
                        }
                        if (unprunedResult.permutationStats.nodes_in_maintained_ties.count(node)) {
                            pruningNodeAnalysis.eliminated_nodes_in_maintained_ties.insert(node);
                        }
                    }
                }
            }
        }
        
        pruningNodeAnalysis.calculateStatistics();
    }
    
    } catch (const std::exception& e) {
        totalBatches = 0;
        successfulBatches = 0;
        failedBatches = 0;
        successRate = 0.0;
        wallClockTimeSeconds = 0.0;
        cumulativeBatchTime = 0.0;
        averageExecutionTime = 0.0;
        minExecutionTime = 0.0;
        maxExecutionTime = 0.0;
    } catch (...) {
        totalBatches = 0;
        successfulBatches = 0;
        failedBatches = 0;
        successRate = 0.0;
        wallClockTimeSeconds = 0.0;
        cumulativeBatchTime = 0.0;
        averageExecutionTime = 0.0;
        minExecutionTime = 0.0;
        maxExecutionTime = 0.0;
    }
}

void BatchAnalysisResult::calculateBasicStatistics() {
    if (allBatchResults.empty()) {
        return;
    }
    
    totalBatches = allBatchResults.size();
    successfulBatches = 0;
    failedBatches = 0;
    cumulativeBatchTime = 0.0;
    minExecutionTime = std::numeric_limits<double>::max();
    maxExecutionTime = 0.0;
    
    std::vector<int> connectorCounts;
    std::vector<int> connectorCountsWithoutPruning;
    std::vector<int> connectorCountsWithPruning;
    
    int bestConnectorCount = std::numeric_limits<int>::max();
    int bestResultIndex = -1;
    int bestConnectorCountWithoutPruning = std::numeric_limits<int>::max();
    int bestResultIndexWithoutPruning = -1;
    int bestConnectorCountWithPruning = std::numeric_limits<int>::max();
    int bestResultIndexWithPruning = -1;
    
    for (size_t i = 0; i < allBatchResults.size(); ++i) {
        const auto& result = allBatchResults[i];
        
        if (result.success && result.basicNetworkResult.isValid) {
            successfulBatches++;
            
            int connectorCount = result.basicNetworkResult.connectorNodes.size();
            connectorCounts.push_back(connectorCount);
            
            if (connectorCount < bestConnectorCount) {
                bestConnectorCount = connectorCount;
                bestResultIndex = i;
            }
            
            cumulativeBatchTime += result.executionTimeSeconds;
            if (result.executionTimeSeconds < minExecutionTime) {
                minExecutionTime = result.executionTimeSeconds;
            }
            if (result.executionTimeSeconds > maxExecutionTime) {
                maxExecutionTime = result.executionTimeSeconds;
            }
        } else {
            failedBatches++;
        }
    }
    
    for (size_t i = 0; i < allBatchResultsWithoutPruning.size(); ++i) {
        const auto& result = allBatchResultsWithoutPruning[i];
        if (result.success && result.basicNetworkResult.isValid) {
            int connectorCount = result.basicNetworkResult.connectorNodes.size();
            connectorCountsWithoutPruning.push_back(connectorCount);
            if (connectorCount < bestConnectorCountWithoutPruning) {
                bestConnectorCountWithoutPruning = connectorCount;
                bestResultIndexWithoutPruning = i;
            }
        }
    }
    
    for (size_t i = 0; i < allBatchResultsWithPruning.size(); ++i) {
        const auto& result = allBatchResultsWithPruning[i];
        if (result.success && result.basicNetworkResult.isValid) {
            int connectorCount = result.basicNetworkResult.connectorNodes.size();
            connectorCountsWithPruning.push_back(connectorCount);
            if (connectorCount < bestConnectorCountWithPruning) {
                bestConnectorCountWithPruning = connectorCount;
                bestResultIndexWithPruning = i;
            }
        }
    }
    
    if (bestResultIndex >= 0) {
        bestResult = allBatchResults[bestResultIndex];
    }
    if (bestResultIndexWithoutPruning >= 0) {
        bestResultWithoutPruning = allBatchResultsWithoutPruning[bestResultIndexWithoutPruning];
    }
    if (bestResultIndexWithPruning >= 0) {
        bestResultWithPruning = allBatchResultsWithPruning[bestResultIndexWithPruning];
    }
    
    successRate = (totalBatches > 0) ? (static_cast<double>(successfulBatches) / totalBatches) * 100.0 : 0.0;
    averageExecutionTime = (successfulBatches > 0) ? cumulativeBatchTime / successfulBatches : 0.0;
    
    if (!connectorCounts.empty()) {
        connectorStatistics = BasicNetworkFinder::calculateNetworkStatistics(connectorCounts);
    }
    if (!connectorCountsWithoutPruning.empty()) {
        connectorStatisticsWithoutPruning = BasicNetworkFinder::calculateNetworkStatistics(connectorCountsWithoutPruning);
    }
    if (!connectorCountsWithPruning.empty()) {
        connectorStatisticsWithPruning = BasicNetworkFinder::calculateNetworkStatistics(connectorCountsWithPruning);
    }
} 