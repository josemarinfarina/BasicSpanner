#ifndef PARALLEL_BASIC_SPANNER_ENGINE_H
#define PARALLEL_BASIC_SPANNER_ENGINE_H

#include "BasicSpannerEngine.h"
#include "ThreadPool.h"
#include "ParallelGraphAlgorithms.h"
#include "PathManager.h"
#include <atomic>
#include <mutex>
#include <chrono>

struct ParallelBasicSpannerConfig : public BasicSpannerConfig {
    int maxThreads;
    bool enableParallelPermutations;
    bool enableParallelCalculations;
    bool enablePerformanceMonitoring;
    int permutationBatchSize;
    
    ParallelBasicSpannerConfig() : BasicSpannerConfig() {
        maxThreads = std::thread::hardware_concurrency();
        enableParallelPermutations = true;
        enableParallelCalculations = true;
        enablePerformanceMonitoring = false;
        permutationBatchSize = std::max(1, numPermutations / (maxThreads * 2));
    }
};

struct ParallelBasicSpannerResult : public BasicNetworkResult {
    ParallelPerformance::PerformanceMetrics performanceMetrics;
    double totalSpeedup;
    size_t threadsUsed;
    
    ParallelBasicSpannerResult() : BasicNetworkResult() {
        totalSpeedup = 1.0;
        threadsUsed = 1;
    }
};

struct AllParallelPermutationResults {
    ParallelBasicSpannerResult bestResult;
    std::vector<BasicNetworkResult> allResults;
    std::vector<BasicNetworkResult> tiedResults;
    bool hasMultipleResults;
    
    AllParallelPermutationResults() : hasMultipleResults(false) {}
};

class ParallelBasicSpannerEngine : public BasicSpannerEngine {
public:
    struct ParallelExecutionError {
        int threadId;
        int permutation;
        std::string errorMessage;
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    using ThreadSafeProgressCallback = std::function<void(int permutation, int totalPermutations, 
                                                          const std::string& status, int threadId)>;
    using ThreadSafeDetailedProgressCallback = std::function<void(int permutation, int totalPermutations, 
                                                                 const std::string& stage, 
                                                                 int stageProgress, int stageTotal, int threadId)>;
    
        using ThreadSafePermutationCompletedCallback = std::function<void(int permutation, int totalPermutations, 
                                                                     int connectorsFound, int threadId, const std::vector<int>& connectorIds, int preConnectorsFound)>;
    
    ParallelBasicSpannerEngine();
    explicit ParallelBasicSpannerEngine(std::shared_ptr<ThreadPool> pool);
    ~ParallelBasicSpannerEngine();
    
    void setThreadSafeProgressCallback(ThreadSafeProgressCallback callback);
    void setThreadSafeDetailedProgressCallback(ThreadSafeDetailedProgressCallback callback);
    void setThreadSafePermutationCompletedCallback(ThreadSafePermutationCompletedCallback callback);
    void setMaxThreads(int maxThreads);
    void setEnableParallelPermutations(bool enable);
    void setEnableParallelCalculations(bool enable);
    void setPermutationBatchSize(int batchSize);
    
    std::vector<ParallelExecutionError> getExecutionErrors() const;
    void clearExecutionErrors();
    
    ParallelBasicSpannerResult findBasicNetworkParallel(const Graph& graph, 
                                                   const std::vector<int>& seedNodes,
                                                   const ParallelBasicSpannerConfig& config = ParallelBasicSpannerConfig());
    
    ParallelBasicSpannerResult findBasicNetworkParallel(const Graph& graph, 
                                                   const std::vector<int>& seedNodes,
                                                   PathManager& masterPathManager,
                                                   const ParallelBasicSpannerConfig& config = ParallelBasicSpannerConfig());
    
    AllParallelPermutationResults findBasicNetworkParallelWithAllResults(const Graph& graph, 
                                                                         const std::vector<int>& seedNodes,
                                                                         PathManager& masterPathManager,
                                                                         const ParallelBasicSpannerConfig& config = ParallelBasicSpannerConfig());
    
    AllParallelPermutationResults findBasicNetworkParallelWithAllResults(const Graph& graph, 
                                                                         const std::vector<int>& seedNodes,
                                                                         const ParallelBasicSpannerConfig& config = ParallelBasicSpannerConfig());
    
    using BasicSpannerEngine::findBasicNetwork;

private:
    std::shared_ptr<ThreadPool> thread_pool_;
    ParallelBasicSpannerConfig parallel_config_;
    ThreadSafeProgressCallback thread_safe_progress_callback_;
    ThreadSafeDetailedProgressCallback thread_safe_detailed_progress_callback_;
    ThreadSafePermutationCompletedCallback thread_safe_permutation_completed_callback_;
    
    class ThreadSafeResultCollector {
    private:
        std::vector<BasicNetworkResult> results_;
        mutable std::mutex mutex_;
        BasicNetworkResult best_result_;
        int best_connector_count_;
        bool best_result_initialized_ = false;
        
    public:
        ThreadSafeResultCollector() : best_connector_count_(std::numeric_limits<int>::max()) {}
        
        void addResult(BasicNetworkResult&& result) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (result.isValid) {
                BasicNetworkResult summary = result;
                summary.basicNetwork_ptr.reset();
                results_.push_back(std::move(summary));

                if (!best_result_initialized_ || result.totalConnectors < best_connector_count_) {
                    best_connector_count_ = result.totalConnectors;
                    best_result_ = std::move(result);
                    best_result_initialized_ = true;
                } else if (result.totalConnectors == best_connector_count_) {
                }
            }
        }
        
        BasicNetworkResult getBestResult() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return best_result_;
        }
        
        std::vector<BasicNetworkResult> getAllResults() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return results_;
        }
        
        size_t getResultCount() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return results_.size();
        }
        void clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            results_.clear();
            best_result_ = BasicNetworkResult();
            best_connector_count_ = std::numeric_limits<int>::max();
            best_result_initialized_ = false;
        }
    };
    
    ParallelBasicSpannerResult runParallelPermutations(const Graph& graph, 
                                                  const std::vector<int>& seedNodes,
                                                  const ParallelBasicSpannerConfig& config);
    
    std::vector<BasicNetworkResult> runPermutationBatch(const Graph& graph,
                                                       const std::vector<int>& seedNodes,
                                                       int batchStart, int batchEnd,
                                                       int threadId,
                                                       const std::vector<int>* pMasterNodeOrderBase);
    
    std::vector<BasicNetworkResult> runPermutationBatch(const Graph& graph,
                                                       const std::vector<int>& seedNodes,
                                                       int batchStart, int batchEnd,
                                                       int threadId,
                                                       const std::vector<int>* pMasterNodeOrderBase,
                                                       const PathManager& masterPathManager);
    
    BasicNetworkResult runSingleIterationParallel(const Graph& graph, 
                                                 const std::vector<int>& seedNodes,
                                                 int currentPermutation, int totalPermutations,
                                                 int threadId,
                                                 std::mt19937& rng,
                                                 PathManager& pathManager);
    
    std::unordered_map<std::pair<int, int>, int, PairHash> calculateSeedDistancesParallel(
        const Graph& graph, const std::vector<int>& seedNodes);
    
    std::set<int> identifySeedPlusOneUnitsParallel(const Graph& graph, 
                                                  const std::vector<int>& seedNodes);
    
    std::set<int> identifySeedPlusNUnitsParallel(const Graph& graph,
                                                const std::set<int>& existingUnits,
                                                int n);
    
    PathEliminationResult eliminatePathsParallel(const Graph& graph,
                                                const std::vector<int>& seedNodes,
                                                const std::set<int>& candidateConnectors,
                                                const std::vector<int>& nodeOrder);
    
    ParallelPerformance::PerformanceMonitor performance_monitor_;
    
    void reportProgressThreadSafe(int permutation, int totalPermutations, 
                                 const std::string& status, int threadId);
    void reportDetailedProgressThreadSafe(int permutation, int totalPermutations, 
                                        const std::string& stage, 
                                        int stageProgress, int stageTotal, int threadId);
    void reportPermutationCompletedThreadSafe(int permutation, int totalPermutations,
                                             int connectorsFound, int threadId, const std::vector<int>& connectorIds = {}, int preConnectorsFound = -1);
    
    std::vector<int> createRandomOrderThreadSafe(const std::set<int>& nodes, int threadId);
    int getThreadLocalSeed(int baseSeeed, int threadId, int permutation);
    
    struct WorkloadBalance {
        std::vector<int> batch_sizes;
        std::vector<int> batch_starts;
        int total_batches;
        
        WorkloadBalance(int total_permutations, int num_threads, int min_batch_size = 1);
    };
    
    WorkloadBalance calculateWorkloadBalance(int totalPermutations, int numThreads);
    
    BasicNetworkResult combineTiedResults(const std::vector<BasicNetworkResult>& tiedResults,
                                        const Graph& originalGraph);
    
    void optimizeMemoryUsage();
    size_t estimateMemoryUsage(const Graph& graph, const ParallelBasicSpannerConfig& config);
    
    std::vector<ParallelExecutionError> execution_errors_;
    mutable std::mutex error_mutex_;
    
    void logThreadError(int threadId, int permutation, const std::string& error);
};

namespace ParallelBasicSpannerUtils {
    
    ParallelBasicSpannerConfig detectOptimalConfiguration(const Graph& graph, 
                                                     const std::vector<int>& seedNodes);
    
    size_t estimateGraphMemoryUsage(const Graph& graph);
    size_t estimatePermutationMemoryUsage(const Graph& graph, int numPermutations);
    
    struct PerformancePrediction {
        double estimatedSerialTime;
        double estimatedParallelTime;
        double estimatedSpeedup;
        int recommendedThreads;
        bool recommendParallelization;
    };
    
    PerformancePrediction predictPerformance(const Graph& graph, 
                                           const std::vector<int>& seedNodes,
                                           const ParallelBasicSpannerConfig& config);
    
    void optimizeThreadAffinity(int threadId, int totalThreads);
    
    void optimizeCacheUsage();
}

#endif