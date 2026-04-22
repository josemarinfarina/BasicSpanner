#include "ParallelGraphAlgorithms.h"
#include <chrono>
#include <iostream>
#include <algorithm>

ParallelGraphAlgorithms::ParallelGraphAlgorithms(std::shared_ptr<ThreadPool> pool) 
    : thread_pool_(pool) {
    if (!thread_pool_) {
        thread_pool_ = std::make_shared<ThreadPool>();
    }
}

std::unordered_map<std::pair<int, int>, int, PairHash> 
ParallelGraphAlgorithms::getAllPairDistancesParallel(const Graph& graph, const std::vector<int>& nodes) {
    
    if (nodes.empty()) {
        return std::unordered_map<std::pair<int, int>, int, PairHash>();
    }
    
    if (nodes.size() < 10) {
        return GraphAlgorithms::getAllPairDistances(graph, nodes);
    }
    
    ThreadSafeMap<std::pair<int, int>, int, PairHash> result_map;
    
    ThreadPool pool(ParallelUtils::getOptimalThreadCount(nodes.size()));
    std::vector<std::future<void>> futures;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        futures.push_back(pool.enqueue([&graph, &nodes, &result_map, i]() {
            int source = nodes[i];
            auto sourceDistances = GraphAlgorithms::bfsDistances(graph, source);
            
            std::unordered_map<std::pair<int, int>, int, PairHash> local_distances;
            
            for (size_t j = i; j < nodes.size(); ++j) {
                int target = nodes[j];
                int dist = (i == j) ? 0 : 
                          (sourceDistances.find(target) != sourceDistances.end() ? 
                           sourceDistances[target] : -1);
                
                local_distances[{source, target}] = dist;
                if (source != target) {
                    local_distances[{target, source}] = dist;
                }
            }
            
            result_map.insert(local_distances);
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    return result_map.getMap();
}

std::set<int> ParallelGraphAlgorithms::getAllNodesInSeedGeodesicsParallel(
    const Graph& graph, const std::vector<int>& seedNodes) {
    
    if (seedNodes.size() < 2) {
        return std::set<int>(seedNodes.begin(), seedNodes.end());
    }
    
    size_t num_pairs = (seedNodes.size() * (seedNodes.size() - 1)) / 2;
    if (num_pairs < 10) {
        return GraphAlgorithms::getAllNodesInSeedGeodesics(graph, seedNodes);
    }
    
    ThreadSafeSet result_set;
    ThreadPool pool(ParallelUtils::getOptimalThreadCount(num_pairs));
    std::vector<std::future<void>> futures;
    
    for (size_t i = 0; i < seedNodes.size(); ++i) {
        for (size_t j = i + 1; j < seedNodes.size(); ++j) {
            futures.push_back(pool.enqueue([&graph, &seedNodes, &result_set, i, j]() {
                auto nodesInPath = GraphAlgorithms::getNodesInGeodesicPaths(
                    graph, seedNodes[i], seedNodes[j]);
                result_set.insert(nodesInPath);
            }));
        }
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    return result_set.getSet();
}

std::unordered_map<std::pair<int, int>, PathInfo, PairHash> 
ParallelGraphAlgorithms::getAllPairPathInfoParallel(const Graph& graph, const std::vector<int>& seedNodes) {
    
    if (seedNodes.empty()) {
        return std::unordered_map<std::pair<int, int>, PathInfo, PairHash>();
    }
    
    if (seedNodes.size() < 8) {
        return GraphAlgorithms::getAllPairPathInfo(graph, seedNodes);
    }
    
    ThreadSafeMap<std::pair<int, int>, PathInfo, PairHash> result_map;
    ThreadPool pool(ParallelUtils::getOptimalThreadCount(seedNodes.size()));
    std::vector<std::future<void>> futures;
    
    for (size_t i = 0; i < seedNodes.size(); ++i) {
        for (size_t j = i; j < seedNodes.size(); ++j) {
            futures.push_back(pool.enqueue([&graph, &seedNodes, &result_map, i, j]() {
                PathInfo info = GraphAlgorithms::getPathInfo(graph, seedNodes[i], seedNodes[j]);
                
                result_map.insert({seedNodes[i], seedNodes[j]}, info);
                if (i != j) {
                    result_map.insert({seedNodes[j], seedNodes[i]}, info);
                }
            }));
        }
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    return result_map.getMap();
}

std::unordered_map<int, std::unordered_map<int, int>> 
ParallelGraphAlgorithms::multiSourceBFSParallel(const Graph& graph, const std::vector<int>& sources) {
    
    if (sources.empty()) {
        return std::unordered_map<int, std::unordered_map<int, int>>();
    }
    
    if (sources.size() < 4) {
        std::unordered_map<int, std::unordered_map<int, int>> result;
        for (int source : sources) {
            result[source] = GraphAlgorithms::bfsDistances(graph, source);
        }
        return result;
    }
    
    ThreadSafeMap<int, std::unordered_map<int, int>> result_map;
    ThreadPool pool(ParallelUtils::getOptimalThreadCount(sources.size()));
    std::vector<std::future<void>> futures;
    
    for (int source : sources) {
        futures.push_back(pool.enqueue([&graph, &result_map, source]() {
            auto distances = GraphAlgorithms::bfsDistances(graph, source);
            result_map.insert(source, distances);
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    return result_map.getMap();
}

bool ParallelGraphAlgorithms::preservesDistancesParallel(const Graph& originalGraph, 
                                                       const Graph& subgraph, 
                                                       const std::vector<int>& seedNodes) {
    
    if (seedNodes.size() < 8) {
        return GraphAlgorithms::preservesDistances(originalGraph, subgraph, seedNodes);
    }
    
    auto originalDistances = getAllPairDistancesParallel(originalGraph, seedNodes);
    auto subgraphDistances = getAllPairDistancesParallel(subgraph, seedNodes);
    
    for (const auto& pair : originalDistances) {
        auto it = subgraphDistances.find(pair.first);
        if (it == subgraphDistances.end() || it->second != pair.second) {
            return false;
        }
    }
    
    return true;
}

namespace BasicSpannerParallel {
    
    std::unordered_map<std::pair<int, int>, int, PairHash> 
    calculateSeedDistancesParallel(const Graph& graph, const std::vector<int>& seedNodes) {
        return ParallelGraphAlgorithms::getAllPairDistancesParallel(graph, seedNodes);
    }
    
    std::set<int> identifySeedPlusOneUnitsParallel(const Graph& graph, const std::vector<int>& seedNodes) {
        
        if (seedNodes.size() < 2) {
            return std::set<int>();
        }
        
        ParallelGraphAlgorithms::ThreadSafeSet result_set;
        ThreadPool pool(ParallelUtils::getOptimalThreadCount(seedNodes.size()));
        std::vector<std::future<void>> futures;
        
        for (int seedNode : seedNodes) {
            futures.push_back(pool.enqueue([&graph, &seedNodes, &result_set, seedNode]() {
                auto seedPlusOneNodes = GraphAlgorithms::getSeedPlusOneNodes(graph, seedNode, seedNodes);
                result_set.insert(seedPlusOneNodes);
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        return result_set.getSet();
    }
    
    std::set<int> identifySeedPlusNUnitsParallel(const Graph& graph, 
                                               const std::vector<int>& seedNodes,
                                               const std::set<int>& basicUnits, 
                                               int n) {
        
        if (seedNodes.size() < 2) {
            return std::set<int>();
        }
        
        ParallelGraphAlgorithms::ThreadSafeSet result_set;
        ThreadPool pool(ParallelUtils::getOptimalThreadCount(seedNodes.size()));
        std::vector<std::future<void>> futures;
        
        for (int seedNode : seedNodes) {
            futures.push_back(pool.enqueue([&graph, &seedNodes, &result_set, seedNode, n]() {
                auto seedPlusNNodes = GraphAlgorithms::getSeedPlusNNodes(graph, seedNode, seedNodes, n);
                result_set.insert(seedPlusNNodes);
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        return result_set.getSet();
    }
    
    PathValidationResult validatePathsParallel(const Graph& graph,
                                             const std::vector<int>& seedNodes,
                                             const std::set<int>& candidateNodes) {
        
        PathValidationResult result;
        result.isValid = true;
        
        if (seedNodes.size() < 2 || candidateNodes.empty()) {
            return result;
        }
        
        std::vector<int> allNodes(seedNodes.begin(), seedNodes.end());
        allNodes.insert(allNodes.end(), candidateNodes.begin(), candidateNodes.end());
        
        auto distances = ParallelGraphAlgorithms::getAllPairDistancesParallel(graph, allNodes);
        
        for (size_t i = 0; i < seedNodes.size(); ++i) {
            for (size_t j = i + 1; j < seedNodes.size(); ++j) {
                auto key = std::make_pair(seedNodes[i], seedNodes[j]);
                auto it = distances.find(key);
                
                if (it == distances.end() || it->second == -1) {
                    result.isValid = false;
                    result.errorMessage = "Path not found between seeds " + 
                                        std::to_string(seedNodes[i]) + " and " + 
                                        std::to_string(seedNodes[j]);
                    return result;
                }
            }
        }
        
        return result;
    }

    std::set<int> determineBasicSeedPlusOneUnitsParallel(
        const Graph& graph,
        const std::vector<int>& seedNodes,
        const std::set<int>& seedPlusOneUnits,
        int max_threads_for_task,
        int currentPermutation,
        int totalPermutations,
        ParallelSubStageDetailedProgress detailed_progress_callback) {

        if (seedPlusOneUnits.empty() || seedNodes.size() < 2) {
            if (detailed_progress_callback) {
                detailed_progress_callback(0, 0, "Determining basic seed+1 units (parallel - no work)", currentPermutation, totalPermutations, -1);
            }
            return std::set<int>();
        }

        ParallelGraphAlgorithms::ThreadSafeSet basic_units_collector;
        std::vector<int> candidate_units_vec(seedPlusOneUnits.begin(), seedPlusOneUnits.end());
        const int total_candidate_units = candidate_units_vec.size();

        if (detailed_progress_callback) {
            detailed_progress_callback(0, total_candidate_units, "Determining basic seed+1 units (parallel)", currentPermutation, totalPermutations, -1);
        }

        std::atomic<int> processed_units_count = 0;
        const int update_frequency = (total_candidate_units > 500) ? 100 : std::max(1, total_candidate_units / 10);

        size_t num_threads_to_use = (max_threads_for_task > 0) ? 
                                    static_cast<size_t>(max_threads_for_task) :
                                    ParallelUtils::getOptimalThreadCount(total_candidate_units);
        if (num_threads_to_use == 0) num_threads_to_use = 1;

        ThreadPool internal_pool(num_threads_to_use);

        std::vector<std::future<void>> futures;
        futures.reserve(total_candidate_units);

        for (int unit_candidate : candidate_units_vec) {
            futures.emplace_back(internal_pool.enqueue([&graph, &seedNodes, unit_candidate, &basic_units_collector, 
                                                         &processed_units_count, total_candidate_units, 
                                                         update_frequency, currentPermutation, totalPermutations, 
                                                         &detailed_progress_callback]() {

                bool isBasic = false;
                
                for (size_t i = 0; i < seedNodes.size() && !isBasic; ++i) {
                    for (size_t j = i + 1; j < seedNodes.size(); ++j) {
                        int seed1 = seedNodes[i];
                        int seed2 = seedNodes[j];
                        
                        auto allPaths = GraphAlgorithms::getAllShortestPaths(graph, seed1, seed2);

                        bool inAnyPath = false;
                        if (!allPaths.empty()) {
                            for (const auto& path : allPaths) {
                                if (std::find(path.begin(), path.end(), unit_candidate) != path.end()) {
                                    inAnyPath = true;
                                    break;
                                }
                            }
                        }
                        if (inAnyPath) {
                            isBasic = true;
                            break;
                        }
                    }
                }
                if (isBasic) {
                    basic_units_collector.insert(unit_candidate);
                }

                int current_processed = processed_units_count.fetch_add(1) + 1;
                if (detailed_progress_callback && (current_processed % update_frequency == 0 || current_processed == total_candidate_units)) {
                    detailed_progress_callback(current_processed, total_candidate_units, "Determining basic seed+1 units (parallel)", currentPermutation, totalPermutations, -2);
                }
            }));
        }

        for (auto& fut : futures) {
            try {
                fut.get();
            } catch (const std::exception& e) {
                if (detailed_progress_callback) {
                     detailed_progress_callback(processed_units_count.load(), total_candidate_units, "Error in determineBasicSeedPlusOneUnitsParallel: " + std::string(e.what()), currentPermutation, totalPermutations, -2);
                }
            }
        }
        
        if (detailed_progress_callback && (processed_units_count.load() % update_frequency != 0 && processed_units_count.load() == total_candidate_units) ) {
             detailed_progress_callback(total_candidate_units, total_candidate_units, "Determining basic seed+1 units (parallel)", currentPermutation, totalPermutations, -2);
        }

        return basic_units_collector.getSet();
    }
}

namespace ParallelPerformance {
    
    void PerformanceMonitor::startTotal() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    void PerformanceMonitor::startParallel(size_t threads) {
        parallel_start_ = std::chrono::high_resolution_clock::now();
        threads_used_ = threads;
    }
    
    void PerformanceMonitor::endParallel() {
        parallel_end_ = std::chrono::high_resolution_clock::now();
    }
    
    PerformanceMetrics PerformanceMonitor::getMetrics() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        
        PerformanceMetrics metrics;
        metrics.total_time_ms = std::chrono::duration<double, std::milli>(
            end_time - start_time_).count();
        metrics.parallel_time_ms = std::chrono::duration<double, std::milli>(
            parallel_end_ - parallel_start_).count();
        metrics.serial_time_ms = metrics.total_time_ms - metrics.parallel_time_ms;
        metrics.overhead_time_ms = metrics.serial_time_ms;
        metrics.threads_used = threads_used_;
        
        if (metrics.total_time_ms > 0) {
            double parallel_fraction = metrics.parallel_time_ms / metrics.total_time_ms;
            metrics.speedup_factor = 1.0 / ((1.0 - parallel_fraction) + 
                                          (parallel_fraction / threads_used_));
            metrics.efficiency = metrics.speedup_factor / threads_used_;
        } else {
            metrics.speedup_factor = 1.0;
            metrics.efficiency = 1.0;
        }
        
        return metrics;
    }
    
    void PerformanceMonitor::printReport(const std::string& operation_name) const {
        auto metrics = getMetrics();
        
        std::cout << "\n=== Performance Report: " << operation_name << " ===" << std::endl;
        std::cout << "Total Time: " << metrics.total_time_ms << " ms" << std::endl;
        std::cout << "Parallel Time: " << metrics.parallel_time_ms << " ms" << std::endl;
        std::cout << "Serial/Overhead Time: " << metrics.serial_time_ms << " ms" << std::endl;
        std::cout << "Threads Used: " << metrics.threads_used << std::endl;
        std::cout << "Speedup Factor: " << metrics.speedup_factor << "x" << std::endl;
        std::cout << "Efficiency: " << (metrics.efficiency * 100) << "%" << std::endl;
        std::cout << "=========================================" << std::endl;
    }
} 