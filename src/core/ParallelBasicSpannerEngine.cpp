/**
 * @file ParallelBasicSpannerEngine.cpp
 * @brief Implementation of the ParallelBasicSpannerEngine class for multi-threaded basic network analysis
 */

#include "ParallelBasicSpannerEngine.h"
#include "PathManager.h"
#include "PaperAlgorithmHelpers.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>

/**
 * WorkloadBalance implementation
 */
ParallelBasicSpannerEngine::WorkloadBalance::WorkloadBalance(int total_permutations, int num_threads, int ) {
    if (num_threads <= 0 || total_permutations <= 0) {
        total_batches = 0;
        return;
    }
    
    /**
     * Distribute work as evenly as possible using classic n/k + n%k pattern.
     * Example: 100 perms / 30 threads → 10 threads get 4, 20 threads get 3.
     */
    int effective_threads = std::min(num_threads, total_permutations);
    int base_size = total_permutations / effective_threads;
    int extra = total_permutations % effective_threads;
    
    batch_sizes.reserve(effective_threads);
    batch_starts.reserve(effective_threads);
    
    int current_start = 0;
    for (int i = 0; i < effective_threads; ++i) {
        int batch_size = base_size + (i < extra ? 1 : 0);
        batch_starts.push_back(current_start);
        batch_sizes.push_back(batch_size);
        current_start += batch_size;
    }
    
    total_batches = batch_sizes.size();
}

/**
 * ParallelBasicSpannerEngine implementation
 */
ParallelBasicSpannerEngine::ParallelBasicSpannerEngine() 
    : BasicSpannerEngine(), thread_pool_(std::make_shared<ThreadPool>()) {
}

ParallelBasicSpannerEngine::ParallelBasicSpannerEngine(std::shared_ptr<ThreadPool> pool) 
    : BasicSpannerEngine(), thread_pool_(pool), parallel_config_() {
    if (!thread_pool_) {
        thread_pool_ = std::make_shared<ThreadPool>();
    }
}

ParallelBasicSpannerEngine::~ParallelBasicSpannerEngine() {
    /**
     * Ensure all threads are finished before destruction
     */
    if (thread_pool_) {
        thread_pool_->waitForCompletion();
    }
}

void ParallelBasicSpannerEngine::setThreadSafeProgressCallback(ThreadSafeProgressCallback callback) {
    thread_safe_progress_callback_ = callback;
}

void ParallelBasicSpannerEngine::setThreadSafeDetailedProgressCallback(ThreadSafeDetailedProgressCallback callback) {
    thread_safe_detailed_progress_callback_ = callback;
}

void ParallelBasicSpannerEngine::setThreadSafePermutationCompletedCallback(ThreadSafePermutationCompletedCallback callback) {
    thread_safe_permutation_completed_callback_ = callback;
}

void ParallelBasicSpannerEngine::setMaxThreads(int maxThreads) {
    parallel_config_.maxThreads = std::max(1, maxThreads);
}

void ParallelBasicSpannerEngine::setEnableParallelPermutations(bool enable) {
    parallel_config_.enableParallelPermutations = enable;
}

void ParallelBasicSpannerEngine::setEnableParallelCalculations(bool enable) {
    parallel_config_.enableParallelCalculations = enable;
}

void ParallelBasicSpannerEngine::setPermutationBatchSize(int batchSize) {
    parallel_config_.permutationBatchSize = std::max(1, batchSize);
}

ParallelBasicSpannerResult ParallelBasicSpannerEngine::findBasicNetworkParallel(
    const Graph& graph, 
    const std::vector<int>& seedNodes,
    const ParallelBasicSpannerConfig& config) {
    
    PathManager masterPathManager(graph, seedNodes);

    return findBasicNetworkParallel(graph, seedNodes, masterPathManager, config);
}

ParallelBasicSpannerResult ParallelBasicSpannerEngine::findBasicNetworkParallel(
    const Graph& graph, 
    const std::vector<int>& seedNodes,
    PathManager& masterPathManager,
    const ParallelBasicSpannerConfig& config) {
    
    parallel_config_ = config;
    
    if (parallel_config_.enablePerformanceMonitoring) {
        performance_monitor_.startTotal();
    }
    
    ParallelBasicSpannerResult result;
    
    /**
     * Validate input
     */
    if (seedNodes.empty()) {
        logProgress("Error: No seed nodes provided");
        return result;
    }
    
    /**
     * Validate all seed nodes exist in the graph
     */
    for (int seed : seedNodes) {
        if (!graph.hasNode(seed)) {
            logProgress("Error: Seed node " + std::to_string(seed) + " not found in graph");
            return result;
        }
    }
    
    logProgress("Starting Parallel BasicSpanner algorithm with " + std::to_string(seedNodes.size()) + " seeds (using pre-calculated PathManager)");
    
    try {
        AllParallelPermutationResults allResults = findBasicNetworkParallelWithAllResults(
            graph, 
            seedNodes, 
            masterPathManager,
            config
        );
        
        result = allResults.bestResult;
        
        if (parallel_config_.enablePerformanceMonitoring) {
            result.performanceMetrics = performance_monitor_.getMetrics();
            performance_monitor_.printReport("Parallel BasicSpanner Algorithm");
        }
        
        if (result.isValid) {
            logProgress("Parallel paper algorithm converged to basic network with " + 
                       std::to_string(result.totalConnectors) + " connectors (heuristic result) using " +
                       std::to_string(result.threadsUsed) + " threads (speedup: " +
                       std::to_string(result.totalSpeedup) + "x)");
        } else {
            logProgress("Parallel paper algorithm failed to find valid basic network");
        }
        
    } catch (const std::exception& e) {
        logThreadError(-1, -1, "Exception in parallel algorithm: " + std::string(e.what()));
        result.isValid = false;
    }
    
    return result;
}

ParallelBasicSpannerResult ParallelBasicSpannerEngine::runParallelPermutations(
    const Graph& graph, 
    const std::vector<int>& seedNodes,
    const ParallelBasicSpannerConfig& config) {
    
    ParallelBasicSpannerResult parallel_result;
    
    if (!config.enableParallelPermutations || config.numPermutations < 1) {
        /**
         * Fall back to serial execution for small workloads
         */
        logProgress("Using serial execution (permutations < 1 or parallel disabled)");
        auto serial_result = BasicSpannerEngine::findBasicNetwork(graph, seedNodes, config);
        
        /**
         * Convert to parallel result
         */
        parallel_result.isValid = serial_result.isValid;
        parallel_result.isTiedNetwork = serial_result.isTiedNetwork;
        parallel_result.seedNodes = serial_result.seedNodes;
        parallel_result.connectorNodes = serial_result.connectorNodes;
        parallel_result.allBasicUnits = serial_result.allBasicUnits;
        parallel_result.basicNetwork_ptr = std::move(serial_result.basicNetwork_ptr);
        parallel_result.totalConnectors = serial_result.totalConnectors;
        parallel_result.prunedNetworks = std::move(serial_result.prunedNetworks);
        parallel_result.threadsUsed = 1;
        parallel_result.totalSpeedup = 1.0;
        
        return parallel_result;
    }
    
    if (config.enablePerformanceMonitoring) {
        performance_monitor_.startParallel(config.maxThreads);
    }
    
    ThreadSafeResultCollector result_collector;
    
    WorkloadBalance workload = calculateWorkloadBalance(config.numPermutations, config.maxThreads);
    std::vector<std::future<std::vector<BasicNetworkResult>>> futures;
    
    logProgress("Distributing " + std::to_string(config.numPermutations) + 
               " permutations across " + std::to_string(workload.total_batches) + " batches.");
    
    logProgress("Creating master PathManager for optimization...");
    PathManager masterPathManager(graph, seedNodes);
    logProgress("Master PathManager created. Using optimized version.");
    
    for (int i = 0; i < workload.total_batches; ++i) {
        futures.push_back(thread_pool_->enqueue(
            static_cast<std::vector<BasicNetworkResult>(ParallelBasicSpannerEngine::*)(const Graph&, const std::vector<int>&, int, int, int, const std::vector<int>*, const PathManager&)>(&ParallelBasicSpannerEngine::runPermutationBatch),
            this,
            std::cref(graph),
            std::cref(seedNodes),
            workload.batch_starts[i],
            workload.batch_starts[i] + workload.batch_sizes[i],
            i,
            nullptr,
            std::cref(masterPathManager)
        ));
    }
    
    /**
     * Collect results from all batches
     */
    for (auto& future : futures) {
        try {
            std::vector<BasicNetworkResult> batch_results = future.get();
            for (auto& result : batch_results) {
                result_collector.addResult(std::move(result));
            }
        } catch (const std::exception& e) {
            logThreadError(-1, -1, "Exception collecting batch results: " + std::string(e.what()));
        }
    }
    
    if (config.enablePerformanceMonitoring) {
        performance_monitor_.endParallel();
    }
    
    /**
     * Process collected results
     */
    BasicNetworkResult best_overall_result = result_collector.getBestResult();
    
    if (best_overall_result.isValid) {
        
        parallel_result.isValid = best_overall_result.isValid;
        parallel_result.isTiedNetwork = best_overall_result.isTiedNetwork;
        parallel_result.seedNodes = best_overall_result.seedNodes;
        parallel_result.connectorNodes = best_overall_result.connectorNodes;
        parallel_result.allBasicUnits = best_overall_result.allBasicUnits;
        parallel_result.basicNetwork_ptr = std::move(best_overall_result.basicNetwork_ptr);
        parallel_result.totalConnectors = best_overall_result.totalConnectors;
        parallel_result.prunedNetworks = std::move(best_overall_result.prunedNetworks);

        parallel_result.threadsUsed = workload.total_batches;
        
        reportProgressThreadSafe(config.numPermutations, config.numPermutations, "Parallel permutations completed", -1);
        } else {
        reportProgressThreadSafe(config.numPermutations, config.numPermutations, "Parallel permutations failed", -1);
    }
    
    return parallel_result;
}

std::vector<BasicNetworkResult> ParallelBasicSpannerEngine::runPermutationBatch(
    const Graph& graph,
    const std::vector<int>& seedNodes,
    int batchStartPermutation, 
    int batchEndPermutation,   
    int threadId,
    const std::vector<int>* pMasterNodeOrderBase) {
    std::vector<BasicNetworkResult> batch_results_vector; 

    PathManager pathManager(graph, seedNodes);

    unsigned int seed_val = parallel_config_.randomSeed; 
    seed_val += static_cast<unsigned int>(threadId) * 10000U; 
    seed_val += static_cast<unsigned int>(batchStartPermutation); 
    std::mt19937 thread_local_random_generator(seed_val); 
    
    for (int p = batchStartPermutation; p < batchEndPermutation; ++p) {
            
            
        BasicNetworkResult iteration_result = runSingleIterationParallel(graph, seedNodes, 
                                                                               p + 1, parallel_config_.numPermutations, threadId, thread_local_random_generator, pathManager);
            
        if (iteration_result.isValid) {
            iteration_result.preTotalConnectors = iteration_result.totalConnectors;

            const bool pruningEnabled = (parallel_config_.enablePruning || parallel_config_.randomPruning);
            if (pruningEnabled && iteration_result.basicNetwork_ptr) {
                auto perPermPruned = pruneBasicNetwork(iteration_result, graph, static_cast<const BasicSpannerConfig&>(parallel_config_), nullptr);
                if (!perPermPruned.empty() && perPermPruned.back().isValid) {
                    int preCount = iteration_result.preTotalConnectors;
                    int permNumber = iteration_result.permutationNumber;
                    iteration_result = std::move(perPermPruned.back());
                    iteration_result.preTotalConnectors = preCount;
                    iteration_result.permutationNumber = permNumber;
                }
            }

            std::vector<int> connectorIds(iteration_result.connectorNodes.begin(), iteration_result.connectorNodes.end());
            reportPermutationCompletedThreadSafe(p + 1, parallel_config_.numPermutations, iteration_result.totalConnectors, threadId, connectorIds, iteration_result.preTotalConnectors);
            batch_results_vector.push_back(std::move(iteration_result));
        } else {
            std::vector<int> emptyConnectorIds;
            reportPermutationCompletedThreadSafe(p + 1, parallel_config_.numPermutations, -1, threadId, emptyConnectorIds, -1);
        }
    }
    return batch_results_vector; 
}

std::vector<BasicNetworkResult> ParallelBasicSpannerEngine::runPermutationBatch(
    const Graph& graph,
    const std::vector<int>& seedNodes,
    int batchStartPermutation, 
    int batchEndPermutation,   
    int threadId,
    const std::vector<int>* pMasterNodeOrderBase,
    const PathManager& masterPathManager)
{
    std::vector<BasicNetworkResult> batch_results_vector;

    unsigned int seed_val = parallel_config_.randomSeed; 
    seed_val += static_cast<unsigned int>(threadId) * 10000U; 
    seed_val += static_cast<unsigned int>(batchStartPermutation); 
    std::mt19937 thread_local_random_generator(seed_val); 
    
    for (int p = batchStartPermutation; p < batchEndPermutation; ++p) {
        PathManager pathManagerForPermutation = masterPathManager;
        
        pathManagerForPermutation.randomizeInternalPathOrder(thread_local_random_generator);
        
        std::vector<int> permutedSeedNodes = seedNodes;
        std::shuffle(permutedSeedNodes.begin(), permutedSeedNodes.end(), thread_local_random_generator);
        pathManagerForPermutation.createVirtualView(permutedSeedNodes, thread_local_random_generator);
        
        pathManagerForPermutation.resetAllPathsToActive();
            
            
        BasicNetworkResult iteration_result = runSingleIterationParallel(graph, seedNodes, 
                                                                               p + 1, parallel_config_.numPermutations, threadId, thread_local_random_generator, pathManagerForPermutation);
            
        if (iteration_result.isValid) {
            iteration_result.preTotalConnectors = iteration_result.totalConnectors;

            const bool pruningEnabled = (parallel_config_.enablePruning || parallel_config_.randomPruning);
            if (pruningEnabled && iteration_result.basicNetwork_ptr) {
                auto perPermPruned = pruneBasicNetwork(iteration_result, graph, static_cast<const BasicSpannerConfig&>(parallel_config_), nullptr);
                if (!perPermPruned.empty() && perPermPruned.back().isValid) {
                    int preCount = iteration_result.preTotalConnectors;
                    int permNumber = iteration_result.permutationNumber;
                    iteration_result = std::move(perPermPruned.back());
                    iteration_result.preTotalConnectors = preCount;
                    iteration_result.permutationNumber = permNumber;
                }
            }

            std::vector<int> connectorIds(iteration_result.connectorNodes.begin(), iteration_result.connectorNodes.end());
            reportPermutationCompletedThreadSafe(p + 1, parallel_config_.numPermutations, iteration_result.totalConnectors, threadId, connectorIds, iteration_result.preTotalConnectors);
            batch_results_vector.push_back(std::move(iteration_result));
        } else {
            std::vector<int> emptyConnectorIds;
            reportPermutationCompletedThreadSafe(p + 1, parallel_config_.numPermutations, -1, threadId, emptyConnectorIds, -1);
        }
    }
    return batch_results_vector; 
}

ParallelBasicSpannerEngine::WorkloadBalance ParallelBasicSpannerEngine::calculateWorkloadBalance(
    int totalPermutations, int numThreads) {
    
    /**
     * Ensure we don't create more threads than permutations
     */
    int effective_threads = std::min(numThreads, totalPermutations);
    
    /**
     * Use minimum batch size to prevent too many small batches
     */
    int min_batch_size = std::max(1, totalPermutations / (effective_threads * 4));
    
    return WorkloadBalance(totalPermutations, effective_threads, min_batch_size);
}

BasicNetworkResult ParallelBasicSpannerEngine::combineTiedResults(
    const std::vector<BasicNetworkResult>& tiedResults,
    const Graph& originalGraph) {
    
    if (tiedResults.empty()) {
        return BasicNetworkResult();
    }
    
    if (tiedResults.size() == 1) {
        return tiedResults[0];
    }
    
    /**
     * Combine all connectors from tied solutions
     */
    BasicNetworkResult combined_result = tiedResults[0];
    std::set<int> all_connectors;
    
    for (const auto& result : tiedResults) {
        all_connectors.insert(result.connectorNodes.begin(), result.connectorNodes.end());
    }
    
    combined_result.connectorNodes = all_connectors;
    combined_result.totalConnectors = all_connectors.size();
    combined_result.isTiedNetwork = true;
    
    /**
     * Reconstruct combined basic network
     */
    std::set<int> all_basic_units = combined_result.seedNodes;
    all_basic_units.insert(all_connectors.begin(), all_connectors.end());
    combined_result.allBasicUnits = all_basic_units;
    combined_result.basicNetwork_ptr = std::make_unique<Graph>(constructBasicNetwork(originalGraph, all_basic_units));
    
    return combined_result;
}

void ParallelBasicSpannerEngine::reportProgressThreadSafe(int permutation, int totalPermutations, 
                                                     const std::string& status, int threadId) {
    if (thread_safe_progress_callback_) {
        thread_safe_progress_callback_(permutation, totalPermutations, status, threadId);
    }
}

void ParallelBasicSpannerEngine::reportDetailedProgressThreadSafe(int permutation, int totalPermutations, 
                                                            const std::string& stage, 
                                                            int stageProgress, int stageTotal, int threadId) {
    if (thread_safe_detailed_progress_callback_) {
        thread_safe_detailed_progress_callback_(permutation, totalPermutations, stage, 
                                               stageProgress, stageTotal, threadId);
    }
}

void ParallelBasicSpannerEngine::reportPermutationCompletedThreadSafe(int permutation, int totalPermutations, 
                                                                 int connectorsFound, int threadId, const std::vector<int>& connectorIds, int preConnectorsFound) {
    if (thread_safe_permutation_completed_callback_) {
        thread_safe_permutation_completed_callback_(permutation, totalPermutations, connectorsFound, threadId, connectorIds, preConnectorsFound);
    }
}

int ParallelBasicSpannerEngine::getThreadLocalSeed(int baseSeed, int threadId, int permutationOffset) {
    return baseSeed + (threadId * parallel_config_.numPermutations) + permutationOffset;
}

void ParallelBasicSpannerEngine::logThreadError(int threadId, int permutation, const std::string& error) {
    std::lock_guard<std::mutex> lock(error_mutex_);
    
    ParallelExecutionError err;
    err.threadId = threadId;
    err.permutation = permutation;
    err.errorMessage = error;
    err.timestamp = std::chrono::high_resolution_clock::now();
    
    execution_errors_.push_back(err);
    
    /**
     * Also log to standard output
     */
    std::cerr << "Thread " << threadId << " (permutation " << permutation << "): " << error << std::endl;
}

std::vector<ParallelBasicSpannerEngine::ParallelExecutionError> ParallelBasicSpannerEngine::getExecutionErrors() const {
    std::lock_guard<std::mutex> lock(error_mutex_);
    return execution_errors_;
}

void ParallelBasicSpannerEngine::clearExecutionErrors() {
    std::lock_guard<std::mutex> lock(error_mutex_);
    execution_errors_.clear();
}

AllParallelPermutationResults ParallelBasicSpannerEngine::findBasicNetworkParallelWithAllResults(
    const Graph& graph, 
    const std::vector<int>& seedNodes,
    PathManager& masterPathManager,
    const ParallelBasicSpannerConfig& config)
{
    parallel_config_ = config; 

    AllParallelPermutationResults finalResultsCollection;

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

    logProgress("Starting Parallel BasicSpanner algorithm with " + std::to_string(seedNodes.size()) + 
               " seeds (collecting all results)");

    if (parallel_config_.enablePerformanceMonitoring) {
        performance_monitor_.startTotal();
        performance_monitor_.startParallel(parallel_config_.maxThreads);
    }

    std::set<int> masterCandidateNodeSet;
    if (parallel_config_.enableParallelCalculations && seedNodes.size() >= 2) {
        logProgress("Calculating master candidate nodes for ordering (using ParallelGraphAlgorithms)...");
        masterCandidateNodeSet = ParallelGraphAlgorithms::getAllNodesInSeedGeodesicsParallel(graph, seedNodes);
    } else if (seedNodes.size() >= 2) {
        logProgress("Calculating master candidate nodes for ordering (using GraphAlgorithms)...");
        masterCandidateNodeSet = GraphAlgorithms::getAllNodesInSeedGeodesics(graph, seedNodes);
    } else {
        logProgress("Not enough seed nodes to calculate geodesic candidates. Ordering will use all non-seeds.");
    }

    for (int seedNodeVal : seedNodes) { 
        masterCandidateNodeSet.erase(seedNodeVal);
    }
    std::vector<int> masterNodeOrderBase(masterCandidateNodeSet.begin(), masterCandidateNodeSet.end());
    logProgress("Master candidate node set for ordering calculated. Size: " + std::to_string(masterNodeOrderBase.size()));

    ThreadSafeResultCollector all_permutations_collector;
    std::vector<BasicNetworkResult> all_tied_results_with_graphs;
    std::mutex tied_results_mutex;
    int overall_best_connector_count = std::numeric_limits<int>::max();
    bool overall_best_initialized = false;

    WorkloadBalance workload = calculateWorkloadBalance(config.numPermutations, config.maxThreads);
    std::vector<std::future<std::vector<BasicNetworkResult>>> futures;
    
    logProgress("Distributing " + std::to_string(config.numPermutations) + 
               " permutations across " + std::to_string(workload.total_batches) + " batches for all results collection.");
    
    for (int i = 0; i < workload.total_batches; ++i) {
        futures.push_back(thread_pool_->enqueue(
            static_cast<std::vector<BasicNetworkResult>(ParallelBasicSpannerEngine::*)(const Graph&, const std::vector<int>&, int, int, int, const std::vector<int>*, const PathManager&)>(&ParallelBasicSpannerEngine::runPermutationBatch),
            this,
            std::cref(graph),
            std::cref(seedNodes),
            workload.batch_starts[i],
            workload.batch_starts[i] + workload.batch_sizes[i],
            i,
            nullptr,
            std::cref(masterPathManager)
        ));
    }

    for (auto& future : futures) {
        try {
            std::vector<BasicNetworkResult> batch_results = future.get();
            for (auto& iteration_result : batch_results) {
                if (iteration_result.isValid) {
                    BasicNetworkResult summary_for_collector = iteration_result;
                    summary_for_collector.basicNetwork_ptr.reset();
                    all_permutations_collector.addResult(std::move(summary_for_collector)); 

                    std::lock_guard<std::mutex> lock(tied_results_mutex);
                    if (!overall_best_initialized || iteration_result.totalConnectors < overall_best_connector_count) {
                        overall_best_connector_count = iteration_result.totalConnectors;
                        all_tied_results_with_graphs.clear();
                        all_tied_results_with_graphs.push_back(std::move(iteration_result));
                        overall_best_initialized = true;
                    } else if (iteration_result.totalConnectors == overall_best_connector_count) {
                        all_tied_results_with_graphs.push_back(std::move(iteration_result));
                    }
                }
            }
        } catch (const std::exception& e) {
            logThreadError(0, 0, "Exception in batch processing: " + std::string(e.what()));
        }
    }

    if (parallel_config_.enablePerformanceMonitoring) {
        performance_monitor_.endParallel();
    }

    BasicNetworkResult best_overall_result = all_permutations_collector.getBestResult();
    
    if (overall_best_initialized && !all_tied_results_with_graphs.empty()) {
        BasicNetworkResult best_from_tied = std::move(all_tied_results_with_graphs[0]);
        
        finalResultsCollection.bestResult.isValid = best_from_tied.isValid;
        finalResultsCollection.bestResult.isTiedNetwork = best_from_tied.isTiedNetwork; 
        finalResultsCollection.bestResult.seedNodes = best_from_tied.seedNodes;
        finalResultsCollection.bestResult.connectorNodes = best_from_tied.connectorNodes;
        finalResultsCollection.bestResult.allBasicUnits = best_from_tied.allBasicUnits;
        finalResultsCollection.bestResult.basicNetwork_ptr = std::move(best_from_tied.basicNetwork_ptr);
        finalResultsCollection.bestResult.totalConnectors = best_from_tied.totalConnectors;
        finalResultsCollection.bestResult.preTotalConnectors = best_from_tied.preTotalConnectors;
        finalResultsCollection.bestResult.prunedNetworks.clear();
        
        finalResultsCollection.bestResult.permutationStats = best_from_tied.permutationStats; 

        finalResultsCollection.tiedResults.clear();
        for (size_t i = 0; i < all_tied_results_with_graphs.size(); ++i) {
            if (all_tied_results_with_graphs[i].isValid) { 
                 finalResultsCollection.tiedResults.push_back(std::move(all_tied_results_with_graphs[i]));
            }
        }
        all_tied_results_with_graphs.clear();

        finalResultsCollection.hasMultipleResults = (!finalResultsCollection.allResults.empty() || finalResultsCollection.tiedResults.size() > 1);

        if (finalResultsCollection.tiedResults.size() > 1) {
            reportProgressThreadSafe(config.numPermutations, config.numPermutations, "Found tied solutions - using best individual result (parallel)", -1);
            logProgress("Found " + std::to_string(finalResultsCollection.tiedResults.size()) + " tied solutions with " + 
                       std::to_string(finalResultsCollection.bestResult.totalConnectors) + " connectors each (parallel). Using first tied result.");
            
            finalResultsCollection.bestResult.isTiedNetwork = true;
        }
        
        /**
         * PAPER ALGORITHM INTEGRATION: Apply final pruning as integral part of algorithm
         * This resolves tied networks that the heuristic phase could not simplify further
         */
        if ((config.enablePruning || config.randomPruning) && finalResultsCollection.bestResult.basicNetwork_ptr) {
            logProgress("Per-permutation pruning already applied during parallel heuristic phase. Skipping redundant final pruning pass. Final connectors: " +
                       std::to_string(finalResultsCollection.bestResult.totalConnectors));
        }
    } else if (overall_best_initialized && !all_permutations_collector.getBestResult().basicNetwork_ptr && !all_permutations_collector.getAllResults().empty()) {
        BasicNetworkResult best_summary_from_collector = all_permutations_collector.getBestResult();
        if (best_summary_from_collector.isValid && !finalResultsCollection.bestResult.isValid) {
            finalResultsCollection.bestResult.isValid = best_summary_from_collector.isValid;
            finalResultsCollection.bestResult.isTiedNetwork = best_summary_from_collector.isTiedNetwork;
            finalResultsCollection.bestResult.seedNodes = best_summary_from_collector.seedNodes;
            finalResultsCollection.bestResult.connectorNodes = best_summary_from_collector.connectorNodes;
            finalResultsCollection.bestResult.allBasicUnits = best_summary_from_collector.allBasicUnits;
            finalResultsCollection.bestResult.totalConnectors = best_summary_from_collector.totalConnectors;
        }
    } else if (!finalResultsCollection.bestResult.isValid && !all_permutations_collector.getAllResults().empty()) {
        logProgress("No single best network with graph was finalized from tied results, and collector's best is also without graph or invalid.");
    }

    if (finalResultsCollection.bestResult.isValid) {
        finalResultsCollection.bestResult.threadsUsed = workload.total_batches > 0 ? workload.total_batches : 1;
        if (config.enablePerformanceMonitoring) {
            const auto& metrics = performance_monitor_.getMetrics();
            if (metrics.parallel_time_ms > 0) {
                double serial_equivalent_time_ms = metrics.serial_time_ms;
                if (serial_equivalent_time_ms <= 0) {
                    serial_equivalent_time_ms = metrics.parallel_time_ms * finalResultsCollection.bestResult.threadsUsed;
                    logProgress("Note: Serial equivalent time for speedup calculation was estimated based on parallel time and threads used.");
                }
                finalResultsCollection.bestResult.totalSpeedup = serial_equivalent_time_ms / metrics.parallel_time_ms;
            } else {
                finalResultsCollection.bestResult.totalSpeedup = (finalResultsCollection.bestResult.threadsUsed > 1) ? static_cast<double>(finalResultsCollection.bestResult.threadsUsed) : 1.0;
            }
            finalResultsCollection.bestResult.performanceMetrics = metrics;
        } else {
            finalResultsCollection.bestResult.totalSpeedup = (finalResultsCollection.bestResult.threadsUsed > 1) ? static_cast<double>(finalResultsCollection.bestResult.threadsUsed) : 1.0;
        }
    } else {
        finalResultsCollection.bestResult.threadsUsed = workload.total_batches > 0 ? workload.total_batches : 1;
        finalResultsCollection.bestResult.totalSpeedup = 1.0;
    }
    
    if (thread_pool_) {
        thread_pool_->waitForCompletion();
    }
    
    if (config.enablePerformanceMonitoring && finalResultsCollection.bestResult.isValid) {
         finalResultsCollection.bestResult.performanceMetrics = performance_monitor_.getMetrics(); 
    }

    finalResultsCollection.allResults = all_permutations_collector.getAllResults();

    logProgress("Parallel algorithm (all results) completed.");
    if (finalResultsCollection.bestResult.isValid) {
        logProgress("Best result found with " + 
                   std::to_string(finalResultsCollection.bestResult.totalConnectors) + " connectors using " +
                   std::to_string(finalResultsCollection.bestResult.threadsUsed) + " threads (speedup: " +
                   std::to_string(finalResultsCollection.bestResult.totalSpeedup) + "x)");
    } else {
        logProgress("Parallel algorithm (all results) failed to find a valid overall best network.");
    }
    
    return finalResultsCollection;
}

AllParallelPermutationResults ParallelBasicSpannerEngine::findBasicNetworkParallelWithAllResults(
    const Graph& graph, 
    const std::vector<int>& seedNodes,
    const ParallelBasicSpannerConfig& config)
{
    PathManager masterPathManager(graph, seedNodes);
    return findBasicNetworkParallelWithAllResults(graph, seedNodes, masterPathManager, config);
}

std::vector<int> ParallelBasicSpannerEngine::createRandomOrderThreadSafe(const std::set<int>& nodes, int ) {
    std::vector<int> orderedNodes(nodes.begin(), nodes.end());
    

    std::shuffle(orderedNodes.begin(), orderedNodes.end(), randomGenerator_);
    return orderedNodes;
}

std::unordered_map<std::pair<int, int>, int, PairHash> ParallelBasicSpannerEngine::calculateSeedDistancesParallel(
    const Graph& graph,
    const std::vector<int>& seedNodes) {
    return std::unordered_map<std::pair<int, int>, int, PairHash>();
} 

BasicNetworkResult ParallelBasicSpannerEngine::runSingleIterationParallel(
    const Graph& graph, 
    const std::vector<int>& seedNodes,
    int currentPermutation, int ,
    int threadId,
    std::mt19937& rng,
    PathManager& pathManager) {
    
    return runSingleIteration_impl(graph, seedNodes, currentPermutation, rng, pathManager, threadId);
}