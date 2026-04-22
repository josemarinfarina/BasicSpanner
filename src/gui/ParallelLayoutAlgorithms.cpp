#include "ParallelLayoutAlgorithms.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void ParallelLayoutAlgorithms::ThreadSafeForceAccumulator::addForce(int nodeId, const QPointF& force) {
    std::lock_guard<std::mutex> lock(mutex_);
    forces_[nodeId] += force;
}

void ParallelLayoutAlgorithms::ThreadSafeForceAccumulator::addForces(const std::map<int, QPointF>& forces) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : forces) {
        forces_[pair.first] += pair.second;
    }
}

QPointF ParallelLayoutAlgorithms::ThreadSafeForceAccumulator::getForce(int nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = forces_.find(nodeId);
    return (it != forces_.end()) ? it->second : QPointF(0, 0);
}

std::map<int, QPointF> ParallelLayoutAlgorithms::ThreadSafeForceAccumulator::getAllForces() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return forces_;
}

void ParallelLayoutAlgorithms::ThreadSafeForceAccumulator::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    forces_.clear();
}

size_t ParallelLayoutAlgorithms::ThreadSafeForceAccumulator::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return forces_.size();
}

void ParallelLayoutAlgorithms::layoutSpringForceParallel(
    const Graph& graph,
    const std::vector<int>& nodes,
    std::map<int, QPointF>& nodePositions,
    const ParallelLayoutConfig& config) {
    
    if (nodes.empty()) return;
    
    if (nodes.size() < 20) {
        ParallelSpringForce::SpringForceParams params;
        ParallelSpringForce::runParallelLayout(graph, nodes, nodePositions, params, 1);
        return;
    }
    
    LayoutPerformanceMonitor monitor;
    if (config.enablePerformanceMonitoring) {
        monitor.startLayout();
    }
    
    ParallelSpringForce::SpringForceParams params;
    params.maxIterations = config.maxIterations;
    
    ParallelSpringForce::runParallelLayout(graph, nodes, nodePositions, params, config.maxThreads);
    
    if (config.enablePerformanceMonitoring) {
        monitor.printReport("Parallel Spring Force");
    }
}

void ParallelLayoutAlgorithms::layoutForceAtlasParallel(
    const Graph& graph,
    const std::vector<int>& nodes,
    std::map<int, QPointF>& nodePositions,
    const ParallelLayoutConfig& config) {
    
    if (nodes.empty()) return;
    
    LayoutPerformanceMonitor monitor;
    if (config.enablePerformanceMonitoring) {
        monitor.startLayout();
    }
    
    ParallelForceAtlas::ForceAtlasParams params;
    params.maxIterations = config.maxIterations;
    
    ParallelForceAtlas::runParallelLayout(graph, nodes, nodePositions, params, config.maxThreads);
    
    if (config.enablePerformanceMonitoring) {
        monitor.printReport("Parallel Force Atlas");
    }
}

std::vector<ParallelLayoutAlgorithms::ForceCalculationChunk> 
ParallelLayoutAlgorithms::distributeForceCalculation(size_t numNodes, int numThreads) {
    
    std::vector<ForceCalculationChunk> chunks;
    
    if (numThreads <= 0 || numNodes == 0) {
        return chunks;
    }
    
    size_t totalWork = (numNodes * (numNodes - 1)) / 2;
    size_t workPerThread = totalWork / numThreads;
    
    size_t currentNode = 0;
    size_t currentWork = 0;
    
    for (int t = 0; t < numThreads && currentNode < numNodes; ++t) {
        ForceCalculationChunk chunk;
        chunk.startIndex = currentNode;
        chunk.threadId = t;
        
        size_t targetWork = (t == numThreads - 1) ? totalWork : (t + 1) * workPerThread;
        
        while (currentNode < numNodes && currentWork < targetWork) {
            currentWork += (numNodes - currentNode - 1);
            currentNode++;
        }
        
        chunk.endIndex = currentNode;
        chunks.push_back(chunk);
    }
    
    return chunks;
}

void ParallelLayoutAlgorithms::calculateRepulsiveForcesChunk(
    const std::vector<int>& nodes,
    const std::map<int, QPointF>& positions,
    ThreadSafeForceAccumulator& forceAccumulator,
    const ForceCalculationChunk& chunk,
    double repulsionStrength,
    double minDistance) {
    
    std::map<int, QPointF> localForces;
    
    for (size_t i = chunk.startIndex; i < chunk.endIndex && i < nodes.size(); ++i) {
        int node1 = nodes[i];
        auto pos1_it = positions.find(node1);
        if (pos1_it == positions.end()) continue;
        
        QPointF pos1 = pos1_it->second;
        QPointF totalForce(0, 0);
        
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            int node2 = nodes[j];
            auto pos2_it = positions.find(node2);
            if (pos2_it == positions.end()) continue;
            
            QPointF pos2 = pos2_it->second;
            QPointF delta = pos1 - pos2;
            
            double distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
            
            if (distance < minDistance) {
                distance = minDistance;
                double offsetAngle = static_cast<double>(i + j) * 0.1;
                delta = QPointF(minDistance * std::cos(offsetAngle), minDistance * std::sin(offsetAngle));
            }
            
            double force = repulsionStrength / (distance * distance);
            QPointF unitVector = delta / distance;
            QPointF forceVector = unitVector * force;
            
            if (!std::isnan(forceVector.x()) && !std::isnan(forceVector.y()) &&
                !std::isinf(forceVector.x()) && !std::isinf(forceVector.y())) {
                
                localForces[node1] += forceVector;
                localForces[node2] -= forceVector;
            }
        }
    }
    
    forceAccumulator.addForces(localForces);
}

void ParallelLayoutAlgorithms::calculateAttractiveForcesParallel(
    const Graph& graph,
    const std::map<int, QPointF>& positions,
    ThreadSafeForceAccumulator& forceAccumulator,
    double attractionStrength,
    double optimalDistance,
    int numThreads) {
    
    auto edges = graph.getEdges();
    if (edges.empty()) return;
    
    ThreadPool pool(numThreads);
    std::vector<std::future<void>> futures;
    
    size_t edgesPerThread = std::max(size_t(1), edges.size() / numThreads);
    
    for (int t = 0; t < numThreads; ++t) {
        size_t startEdge = t * edgesPerThread;
        size_t endEdge = (t == numThreads - 1) ? edges.size() : (t + 1) * edgesPerThread;
        
        if (startEdge >= edges.size()) break;
        
        futures.push_back(pool.enqueue([&graph, &positions, &forceAccumulator, &edges,
                                       attractionStrength, optimalDistance, startEdge, endEdge]() {
            std::map<int, QPointF> localForces;
            
            for (size_t i = startEdge; i < endEdge; ++i) {
                const auto& edge = edges[i];
                int node1 = edge.first;
                int node2 = edge.second;
                
                auto pos1_it = positions.find(node1);
                auto pos2_it = positions.find(node2);
                
                if (pos1_it == positions.end() || pos2_it == positions.end()) continue;
                
                QPointF pos1 = pos1_it->second;
                QPointF pos2 = pos2_it->second;
                QPointF delta = pos2 - pos1;
                
                double distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
                
                if (distance > 0) {
                    double displacement = distance - optimalDistance;
                    double force = attractionStrength * displacement;
                    QPointF unitVector = delta / distance;
                    QPointF forceVector = unitVector * force;
                    
                    if (!std::isnan(forceVector.x()) && !std::isnan(forceVector.y()) &&
                        !std::isinf(forceVector.x()) && !std::isinf(forceVector.y())) {
                        
                        localForces[node1] += forceVector;
                        localForces[node2] -= forceVector;
                    }
                }
            }
            
            forceAccumulator.addForces(localForces);
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
}

void ParallelLayoutAlgorithms::updatePositionsParallel(
    const std::vector<int>& nodes,
    std::map<int, QPointF>& positions,
    const std::map<int, QPointF>& forces,
    double dampening,
    double maxDisplacement,
    double maxRadius,
    int numThreads) {
    
    if (nodes.empty()) return;
    
    ThreadPool pool(numThreads);
    std::vector<std::future<void>> futures;
    
    size_t nodesPerThread = std::max(size_t(1), nodes.size() / numThreads);
    
    for (int t = 0; t < numThreads; ++t) {
        size_t startNode = t * nodesPerThread;
        size_t endNode = (t == numThreads - 1) ? nodes.size() : (t + 1) * nodesPerThread;
        
        if (startNode >= nodes.size()) break;
        
        futures.push_back(pool.enqueue([&nodes, &positions, &forces, dampening, 
                                       maxDisplacement, maxRadius, startNode, endNode]() {
            for (size_t i = startNode; i < endNode; ++i) {
                int nodeId = nodes[i];
                
                auto force_it = forces.find(nodeId);
                if (force_it == forces.end()) continue;
                
                QPointF force = force_it->second;
                
                double forceMagnitude = std::sqrt(force.x() * force.x() + force.y() * force.y());
                if (forceMagnitude > maxDisplacement) {
                    force = force * (maxDisplacement / forceMagnitude);
                }
                
                QPointF displacement = force * dampening;
                
                if (!std::isnan(displacement.x()) && !std::isnan(displacement.y()) &&
                    !std::isinf(displacement.x()) && !std::isinf(displacement.y())) {
                    
                    positions[nodeId] += displacement;
                    
                    QPointF& pos = positions[nodeId];
                    double currentRadius = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
                    
                    if (currentRadius > maxRadius) {
                        double angle = std::atan2(pos.y(), pos.x());
                        pos.setX(maxRadius * std::cos(angle));
                        pos.setY(maxRadius * std::sin(angle));
                    }
                }
            }
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
}

namespace ParallelSpringForce {
    
    void runParallelLayout(
        const Graph& graph,
        const std::vector<int>& nodes,
        std::map<int, QPointF>& nodePositions,
        const SpringForceParams& params,
        int numThreads) {
        
        if (nodes.empty()) return;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-150.0, 150.0);
        
        for (int nodeId : nodes) {
            if (nodePositions.find(nodeId) == nodePositions.end()) {
                nodePositions[nodeId] = QPointF(dis(gen), dis(gen));
            }
        }
        
        double repulsionStrength = std::sqrt(params.area / static_cast<double>(nodes.size())) * params.repulsionMultiplier;
        double maxRadius = std::sqrt(static_cast<double>(nodes.size())) * 50.0;
        
        std::cout << "Starting Parallel Spring Force with " << numThreads << " threads" << std::endl;
        
        for (int iter = 0; iter < params.maxIterations; ++iter) {
            ParallelLayoutAlgorithms::ThreadSafeForceAccumulator forceAccumulator;
            
            auto chunks = ParallelLayoutAlgorithms::distributeForceCalculation(nodes.size(), numThreads);
            
            ThreadPool pool(numThreads);
            std::vector<std::future<void>> futures;
            
            for (const auto& chunk : chunks) {
                futures.push_back(pool.enqueue([&nodes, &nodePositions, &forceAccumulator, &chunk, 
                                               repulsionStrength, &params]() {
                    ParallelLayoutAlgorithms::calculateRepulsiveForcesChunk(
                        nodes, nodePositions, forceAccumulator, chunk, 
                        repulsionStrength * repulsionStrength, params.minDistance);
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            ParallelLayoutAlgorithms::calculateAttractiveForcesParallel(
                graph, nodePositions, forceAccumulator, 
                params.springConstant * 0.3, params.springConstant, numThreads);
            
            double cooling = params.dampening * (1.0 - static_cast<double>(iter) / params.maxIterations);
            cooling = std::max(0.1, cooling);
            
            auto allForces = forceAccumulator.getAllForces();
            ParallelLayoutAlgorithms::updatePositionsParallel(
                nodes, nodePositions, allForces, cooling, params.maxForce, maxRadius, numThreads);
            
            if (iter % 10 == 0) {
                double maxDisplacement = ParallelLayoutAlgorithms::calculateMaxDisplacement(allForces, cooling);
                if (maxDisplacement < 0.1) {
                    std::cout << "Spring Force converged after " << iter << " iterations" << std::endl;
                    break;
                }
            }
        }
        
        std::cout << "Parallel Spring Force layout completed" << std::endl;
    }
}

namespace ParallelForceAtlas {
    
    void runParallelLayout(
        const Graph& graph,
        const std::vector<int>& nodes,
        std::map<int, QPointF>& nodePositions,
        const ForceAtlasParams& params,
        int numThreads) {
        
        if (nodes.empty()) return;
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            double angle = (2 * M_PI * i) / nodes.size();
            double radius = std::sqrt(static_cast<double>(nodes.size())) * 20.0;
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> noiseDis(-20.0, 20.0);
            
            double x = radius * std::cos(angle) + noiseDis(gen);
            double y = radius * std::sin(angle) + noiseDis(gen);
            
            nodePositions[nodes[i]] = QPointF(x, y);
        }
        
        std::cout << "Starting Parallel Force Atlas with " << numThreads << " threads" << std::endl;
        
        for (int iter = 0; iter < params.maxIterations; ++iter) {
            ParallelLayoutAlgorithms::ThreadSafeForceAccumulator forceAccumulator;
            
            auto chunks = ParallelLayoutAlgorithms::distributeForceCalculation(nodes.size(), numThreads);
            
            ThreadPool pool(numThreads);
            std::vector<std::future<void>> futures;
            
            for (const auto& chunk : chunks) {
                futures.push_back(pool.enqueue([&nodes, &nodePositions, &forceAccumulator, &chunk, &params]() {
                    ParallelLayoutAlgorithms::calculateRepulsiveForcesChunk(
                        nodes, nodePositions, forceAccumulator, chunk, 
                        params.scalingRatio, params.minDistance);
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            ParallelLayoutAlgorithms::calculateAttractiveForcesParallel(
                graph, nodePositions, forceAccumulator, 
                0.01, 0.0, numThreads);
            
            for (int nodeId : nodes) {
                QPointF pos = nodePositions[nodeId];
                double distFromCenter = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
                
                if (distFromCenter > 0) {
                    QPointF gravityForce = -pos * (params.gravity / distFromCenter);
                    forceAccumulator.addForce(nodeId, gravityForce);
                }
            }
            
            double cooling = params.cooling * (1.0 - static_cast<double>(iter) / params.maxIterations);
            cooling = std::max(0.05, cooling);
            
            auto allForces = forceAccumulator.getAllForces();
            ParallelLayoutAlgorithms::updatePositionsParallel(
                nodes, nodePositions, allForces, cooling, params.maxForce, params.maxRadius, numThreads);
        }
        
        std::cout << "Parallel Force Atlas layout completed" << std::endl;
    }
}

double ParallelLayoutAlgorithms::calculateMaxDisplacement(
    const std::map<int, QPointF>& forces,
    double dampening) {
    
    double maxDisplacement = 0.0;
    for (const auto& pair : forces) {
        QPointF displacement = pair.second * dampening;
        double magnitude = std::sqrt(displacement.x() * displacement.x() + displacement.y() * displacement.y());
        maxDisplacement = std::max(maxDisplacement, magnitude);
    }
    return maxDisplacement;
}

bool ParallelLayoutAlgorithms::hasConverged(double maxDisplacement, double threshold) {
    return maxDisplacement < threshold;
}

void ParallelLayoutAlgorithms::LayoutPerformanceMonitor::startLayout() {
    start_time_ = std::chrono::high_resolution_clock::now();
    iterations_completed_ = 0;
}

void ParallelLayoutAlgorithms::LayoutPerformanceMonitor::startForceCalculation(size_t threads) {
    force_calc_start_ = std::chrono::high_resolution_clock::now();
    threads_used_ = threads;
}

void ParallelLayoutAlgorithms::LayoutPerformanceMonitor::endForceCalculation() {
    force_calc_end_ = std::chrono::high_resolution_clock::now();
}

void ParallelLayoutAlgorithms::LayoutPerformanceMonitor::setIterationsCompleted(int iterations) {
    iterations_completed_ = iterations;
}

ParallelLayoutAlgorithms::LayoutPerformanceMetrics 
ParallelLayoutAlgorithms::LayoutPerformanceMonitor::getMetrics() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    
    LayoutPerformanceMetrics metrics;
    metrics.totalTime = std::chrono::duration<double, std::milli>(end_time - start_time_).count();
    metrics.forceCalculationTime = std::chrono::duration<double, std::milli>(force_calc_end_ - force_calc_start_).count();
    metrics.positionUpdateTime = metrics.totalTime - metrics.forceCalculationTime;
    metrics.iterationsCompleted = iterations_completed_;
    metrics.finalConvergence = 0.0;
    metrics.threadsUsed = threads_used_;
    
    metrics.speedupFactor = std::min(static_cast<double>(threads_used_), metrics.totalTime / 100.0);
    
    return metrics;
}

void ParallelLayoutAlgorithms::LayoutPerformanceMonitor::printReport(const std::string& algorithm_name) const {
    auto metrics = getMetrics();
    
    std::cout << "\n=== Layout Performance Report: " << algorithm_name << " ===" << std::endl;
    std::cout << "Total Time: " << metrics.totalTime << " ms" << std::endl;
    std::cout << "Force Calculation Time: " << metrics.forceCalculationTime << " ms" << std::endl;
    std::cout << "Position Update Time: " << metrics.positionUpdateTime << " ms" << std::endl;
    std::cout << "Iterations Completed: " << metrics.iterationsCompleted << std::endl;
    std::cout << "Threads Used: " << metrics.threadsUsed << std::endl;
    std::cout << "Estimated Speedup: " << metrics.speedupFactor << "x" << std::endl;
    std::cout << "===============================================" << std::endl;
} 