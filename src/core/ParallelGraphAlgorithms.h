#ifndef PARALLEL_GRAPH_ALGORITHMS_H
#define PARALLEL_GRAPH_ALGORITHMS_H

#include "GraphAlgorithms.h"
#include "ThreadPool.h"
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <chrono>
#include <functional>

class ParallelGraphAlgorithms {
public:
    explicit ParallelGraphAlgorithms(std::shared_ptr<ThreadPool> pool = nullptr);
    
    template<typename KeyType, typename ValueType, typename HashType = std::hash<KeyType>>
    class ThreadSafeMap {
    private:
        std::unordered_map<KeyType, ValueType, HashType> map_;
        mutable std::mutex mutex_;
        
    public:
        void insert(const KeyType& key, const ValueType& value) {
            std::lock_guard<std::mutex> lock(mutex_);
            map_[key] = value;
        }
        
        void insert(const std::unordered_map<KeyType, ValueType, HashType>& other) {
            std::lock_guard<std::mutex> lock(mutex_);
            map_.insert(other.begin(), other.end());
        }
        
        std::unordered_map<KeyType, ValueType, HashType> getMap() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return map_;
        }
        
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return map_.size();
        }
    };
    
    class ThreadSafeSet {
    private:
        std::unordered_set<int> set_;
        mutable std::mutex mutex_;
        
    public:
        void insert(int value) {
            std::lock_guard<std::mutex> lock(mutex_);
            set_.insert(value);
        }
        
        void insert(const std::set<int>& other) {
            std::lock_guard<std::mutex> lock(mutex_);
            set_.insert(other.begin(), other.end());
        }
        
        void insert(const std::unordered_set<int>& other) {
            std::lock_guard<std::mutex> lock(mutex_);
            set_.insert(other.begin(), other.end());
        }
        
        std::set<int> getSet() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return std::set<int>(set_.begin(), set_.end());
        }
        
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return set_.size();
        }
    };
    
    static std::unordered_map<std::pair<int, int>, int, PairHash> 
    getAllPairDistancesParallel(const Graph& graph, const std::vector<int>& nodes);
    
    static std::unordered_map<std::pair<int, int>, PathInfo, PairHash> 
    getAllPairPathInfoParallel(const Graph& graph, const std::vector<int>& seedNodes);
    
    static std::set<int> getAllNodesInSeedGeodesicsParallel(
        const Graph& graph, const std::vector<int>& seedNodes);
    
    static std::unordered_map<int, std::unordered_map<int, int>> 
    multiSourceBFSParallel(const Graph& graph, const std::vector<int>& sources);
    
    static std::vector<std::vector<int>> getConnectedComponentsParallel(const Graph& graph);
    
    static bool preservesDistancesParallel(const Graph& originalGraph, 
                                         const Graph& subgraph, 
                                         const std::vector<int>& seedNodes);

private:
    std::shared_ptr<ThreadPool> thread_pool_;
    
    template<typename ContainerType, typename FunctionType>
    static void distributeWork(const ContainerType& container, 
                              FunctionType func,
                              size_t num_threads = 0);
};

namespace BasicSpannerParallel {
    
    std::unordered_map<std::pair<int, int>, int, PairHash> 
    calculateSeedDistancesParallel(const Graph& graph, const std::vector<int>& seedNodes);
    
    std::set<int> identifySeedPlusOneUnitsParallel(const Graph& graph, const std::vector<int>& seedNodes);
    
    std::set<int> identifySeedPlusNUnitsParallel(const Graph& graph, 
                                                const std::vector<int>& seedNodes,
                                                const std::set<int>& basicUnits, 
                                                int n);
    
    struct PathValidationResult {
        bool isValid;
        std::vector<int> invalidNodes;
        std::string errorMessage;
    };
    
    PathValidationResult validatePathsParallel(const Graph& graph,
                                             const std::vector<int>& seedNodes,
                                             const std::set<int>& candidateNodes);

    using ParallelSubStageDetailedProgress = std::function<void(int stageProgress, 
                                                               int stageTotal, 
                                                               const std::string& stageMessage,
                                                               int overallPermutation, 
                                                               int totalOverallPermutations, 
                                                               int threadId)>;

    /**
     * @brief Identifies seed+1 units in parallel.
     * @param seedPlusOneUnits Set of candidate seed+1 units from a previous (possibly serial) step.
     *                         This function determines which of these are "basic" for shortest paths.
     * @param max_threads_for_task Max threads to use for this specific parallel task.
     * @param currentPermutation Current main permutation number (for global progress context).
     * @param totalPermutations Total main permutations (for global progress context).
     * @param detailed_progress_callback Callback for reporting detailed progress of this stage.
     * @return Set of basic seed+1 units.
     */
    std::set<int> determineBasicSeedPlusOneUnitsParallel(
        const Graph& graph,
        const std::vector<int>& seedNodes,
        const std::set<int>& seedPlusOneUnits,
        int max_threads_for_task,
        int currentPermutation,
        int totalPermutations,
        ParallelSubStageDetailedProgress detailed_progress_callback
    );

    /**
     * @brief Identifies seed+N units in parallel.
     */
    std::set<int> determineSeedPlusNUnitsParallel(const Graph& graph, 
                                                const std::vector<int>& seedNodes,
                                                const std::set<int>& basicUnits, 
                                                int n);
}

namespace ParallelPerformance {
    struct PerformanceMetrics {
        double total_time_ms;
        double parallel_time_ms;
        double serial_time_ms;
        double overhead_time_ms;
        size_t threads_used;
        double speedup_factor;
        double efficiency;
    };
    
    class PerformanceMonitor {
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::chrono::high_resolution_clock::time_point parallel_start_;
        std::chrono::high_resolution_clock::time_point parallel_end_;
        size_t threads_used_;
        
    public:
        void startTotal();
        void startParallel(size_t threads);
        void endParallel();
        PerformanceMetrics getMetrics() const;
        void printReport(const std::string& operation_name) const;
    };
}

#endif