#ifndef PARALLEL_LAYOUT_ALGORITHMS_H
#define PARALLEL_LAYOUT_ALGORITHMS_H

#include "../core/Graph.h"
#include "../core/ThreadPool.h"
#include <QPointF>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <chrono>

class ParallelLayoutAlgorithms {
public:
    struct ParallelLayoutConfig {
        int maxThreads;
        bool enableThreadOptimization;
        bool enablePerformanceMonitoring;
        double convergenceThreshold;
        int maxIterations;
        
        ParallelLayoutConfig() {
            maxThreads = std::thread::hardware_concurrency();
            enableThreadOptimization = true;
            enablePerformanceMonitoring = false;
            convergenceThreshold = 0.1;
            maxIterations = 150;
        }
    };
    
    class ThreadSafeForceAccumulator {
    private:
        std::map<int, QPointF> forces_;
        mutable std::mutex mutex_;
        
    public:
        void addForce(int nodeId, const QPointF& force);
        void addForces(const std::map<int, QPointF>& forces);
        QPointF getForce(int nodeId) const;
        std::map<int, QPointF> getAllForces() const;
        void clear();
        size_t size() const;
    };
    
    static void layoutSpringForceParallel(
        const Graph& graph,
        const std::vector<int>& nodes,
        std::map<int, QPointF>& nodePositions,
        const ParallelLayoutConfig& config = ParallelLayoutConfig()
    );
    
    static void layoutForceAtlasParallel(
        const Graph& graph,
        const std::vector<int>& nodes,
        std::map<int, QPointF>& nodePositions,
        const ParallelLayoutConfig& config = ParallelLayoutConfig()
    );
    
    static void layoutYifanHuParallel(
        const Graph& graph,
        const std::vector<int>& nodes,
        std::map<int, QPointF>& nodePositions,
        const ParallelLayoutConfig& config = ParallelLayoutConfig()
    );
    
    struct ForceCalculationChunk {
        size_t startIndex;
        size_t endIndex;
        int threadId;
    };
    
    static void calculateRepulsiveForcesChunk(
        const std::vector<int>& nodes,
        const std::map<int, QPointF>& positions,
        ThreadSafeForceAccumulator& forceAccumulator,
        const ForceCalculationChunk& chunk,
        double repulsionStrength,
        double minDistance
    );
    
    static void calculateAttractiveForcesParallel(
        const Graph& graph,
        const std::map<int, QPointF>& positions,
        ThreadSafeForceAccumulator& forceAccumulator,
        double attractionStrength,
        double optimalDistance,
        int numThreads
    );
    
    static void updatePositionsParallel(
        const std::vector<int>& nodes,
        std::map<int, QPointF>& positions,
        const std::map<int, QPointF>& forces,
        double dampening,
        double maxDisplacement,
        double maxRadius,
        int numThreads
    );
    
    static double calculateMaxDisplacement(
        const std::map<int, QPointF>& forces,
        double dampening
    );
    
    static bool hasConverged(
        double maxDisplacement,
        double threshold
    );
    
    struct LayoutPerformanceMetrics {
        double totalTime;
        double forceCalculationTime;
        double positionUpdateTime;
        int iterationsCompleted;
        double finalConvergence;
        size_t threadsUsed;
        double speedupFactor;
    };
    
    class LayoutPerformanceMonitor {
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::chrono::high_resolution_clock::time_point force_calc_start_;
        std::chrono::high_resolution_clock::time_point force_calc_end_;
        size_t threads_used_;
        int iterations_completed_;
        
    public:
        void startLayout();
        void startForceCalculation(size_t threads);
        void endForceCalculation();
        void setIterationsCompleted(int iterations);
        LayoutPerformanceMetrics getMetrics() const;
        void printReport(const std::string& algorithm_name) const;
    };
    
    static std::vector<ForceCalculationChunk> distributeForceCalculation(
        size_t numNodes,
        int numThreads
    );

private:
    static void optimizeMemoryLayout(
        std::vector<int>& nodes,
        std::map<int, QPointF>& positions
    );
    
    static void optimizeThreadAffinity(int threadId, int totalThreads);
};

namespace ParallelSpringForce {
    
    struct SpringForceParams {
        double springConstant = 80.0;
        double area = 600.0 * 600.0;
        double repulsionMultiplier = 1.5;
        int maxIterations = 150;
        double minDistance = 45.0;
        double maxForce = 50.0;
        double dampening = 0.85;
    };
    
    void runParallelLayout(
        const Graph& graph,
        const std::vector<int>& nodes,
        std::map<int, QPointF>& nodePositions,
        const SpringForceParams& params,
        int numThreads
    );
}

namespace ParallelForceAtlas {
    
    struct ForceAtlasParams {
        double gravity = 1.0;
        double scalingRatio = 2.0;
        int maxIterations = 200;
        double minDistance = 40.0;
        double maxForce = 30.0;
        double maxRadius = 500.0;
        double cooling = 0.9;
    };
    
    void runParallelLayout(
        const Graph& graph,
        const std::vector<int>& nodes,
        std::map<int, QPointF>& nodePositions,
        const ForceAtlasParams& params,
        int numThreads
    );
}

namespace ParallelYifanHu {
    
    struct YifanHuParams {
        double initialStep = 50.0;
        double minStep = 0.01;
        double stepRatio = 0.95;
        double initialAdaptive = 1.0;
        double adaptiveDecay = 0.99;
        int maxIterations = 300;
        double convergenceThreshold = 0.1;
        double minDistance = 30.0;
        double optimalDistance = 80.0;
        double repulsionStrength = 20000.0;
        double attractionStrength = 1.0;
    };
    
    void runParallelLayout(
        const Graph& graph,
        const std::vector<int>& nodes,
        std::map<int, QPointF>& nodePositions,
        const YifanHuParams& params,
        int numThreads
    );
}

namespace ParallelLayoutOptimizations {
    
    struct SOAPositions {
        std::vector<double> x;
        std::vector<double> y;
        std::vector<int> nodeIds;
        
        SOAPositions(const std::vector<int>& nodes, const std::map<int, QPointF>& positions);
        void copyBack(std::map<int, QPointF>& positions) const;
    };
    
    struct SOAForces {
        std::vector<double> fx;
        std::vector<double> fy;
        
        explicit SOAForces(size_t size);
        void clear();
        void addForce(size_t index, double fx, double fy);
        std::pair<double, double> getForce(size_t index) const;
    };
    
    void calculateRepulsiveForcesVectorized(
        const SOAPositions& positions,
        SOAForces& forces,
        double repulsionStrength,
        double minDistance,
        size_t startIndex,
        size_t endIndex
    );
    
    struct CacheOptimalChunk {
        size_t startIndex;
        size_t endIndex;
        size_t cacheLineAlignment;
    };
    
    std::vector<CacheOptimalChunk> createCacheOptimalChunks(
        size_t totalItems,
        int numThreads,
        size_t cacheLineSize = 64
    );
}

#endif