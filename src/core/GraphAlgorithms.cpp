#include "GraphAlgorithms.h"
#include <queue>
#include <algorithm>
#include <limits>

int GraphAlgorithms::getDistance(const Graph& graph, int source, int target) {
    if (!graph.hasNode(source) || !graph.hasNode(target)) {
        return -1;
    }
    
    if (source == target) {
        return 0;
    }
    
    auto distances = bfsDistances(graph, source);
    auto it = distances.find(target);
    return (it != distances.end()) ? it->second : -1;
}

std::unordered_map<int, int> GraphAlgorithms::getDistancesFromSource(const Graph& graph, int source) {
    return bfsDistances(graph, source);
}

std::unordered_map<std::pair<int, int>, int, PairHash> GraphAlgorithms::getAllPairDistances(
    const Graph& graph, const std::vector<int>& nodes) {
    
    std::unordered_map<std::pair<int, int>, int, PairHash> distances;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto sourceDistances = bfsDistances(graph, nodes[i]);
        for (size_t j = i; j < nodes.size(); ++j) {
            int dist = (i == j) ? 0 : 
                      (sourceDistances.find(nodes[j]) != sourceDistances.end() ? 
                       sourceDistances[nodes[j]] : -1);
            
            distances[{nodes[i], nodes[j]}] = dist;
            distances[{nodes[j], nodes[i]}] = dist;
        }
    }
    
    return distances;
}

std::vector<int> GraphAlgorithms::getShortestPath(const Graph& graph, int source, int target) {
    if (!graph.hasNode(source) || !graph.hasNode(target)) {
        return std::vector<int>();
    }
    
    if (source == target) {
        return std::vector<int>{source};
    }
    
    std::unordered_map<int, int> distances;
    std::unordered_map<int, int> predecessors;
    std::queue<int> queue;
    
    distances[source] = 0;
    queue.push(source);
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        if (current == target) {
            break;
        }
        
        for (int neighbor : graph.getNeighbors(current)) {
            if (distances.find(neighbor) == distances.end()) {
                distances[neighbor] = distances[current] + 1;
                predecessors[neighbor] = current;
                queue.push(neighbor);
            }
        }
    }
    
    if (distances.find(target) == distances.end()) {
        return std::vector<int>();
    }
    
    std::vector<int> path;
    int current = target;
    while (current != source) {
        path.push_back(current);
        current = predecessors[current];
    }
    path.push_back(source);
    
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::vector<int>> GraphAlgorithms::getAllShortestPaths(const Graph& graph, int source, int target) {
    if (!graph.hasNode(source) || !graph.hasNode(target)) {
        return std::vector<std::vector<int>>();
    }
    
    if (source == target) {
        return std::vector<std::vector<int>>{{source}};
    }
    
    auto predecessors = bfsPredecessors(graph, source, target);
    
    if (predecessors.find(target) == predecessors.end()) {
        return std::vector<std::vector<int>>();
    }
    
    std::vector<std::vector<int>> allPaths;
    std::vector<int> currentPath;
    reconstructPaths(predecessors, target, source, currentPath, allPaths);
    
    for (auto& path : allPaths) {
        std::reverse(path.begin(), path.end());
    }
    
    return allPaths;
}

PathInfo GraphAlgorithms::getPathInfo(const Graph& graph, int source, int target) {
    PathInfo info;
    info.distance = getDistance(graph, source, target);
    if (info.distance > 0) {
        info.allPaths = getAllShortestPaths(graph, source, target);
    } else if (info.distance == 0) {
        info.allPaths = {{source}};
    }
    return info;
}

std::unordered_map<std::pair<int, int>, PathInfo, PairHash> GraphAlgorithms::getAllPairPathInfo(
    const Graph& graph, const std::vector<int>& seedNodes) {
    
    std::unordered_map<std::pair<int, int>, PathInfo, PairHash> pathInfoMap;
    
    for (size_t i = 0; i < seedNodes.size(); ++i) {
        for (size_t j = i; j < seedNodes.size(); ++j) {
            PathInfo info = getPathInfo(graph, seedNodes[i], seedNodes[j]);
            pathInfoMap[{seedNodes[i], seedNodes[j]}] = info;
            pathInfoMap[{seedNodes[j], seedNodes[i]}] = info;
        }
    }
    
    return pathInfoMap;
}

std::set<int> GraphAlgorithms::getNodesInGeodesicPaths(const Graph& graph, int source, int target) {
    std::set<int> nodesInPaths;
    auto allPaths = getAllShortestPaths(graph, source, target);
    
    for (const auto& path : allPaths) {
        for (int node : path) {
            nodesInPaths.insert(node);
        }
    }
    
    return nodesInPaths;
}

std::set<int> GraphAlgorithms::getAllNodesInSeedGeodesics(const Graph& graph, const std::vector<int>& seedNodes) {
    std::set<int> allNodes;
    
    for (size_t i = 0; i < seedNodes.size(); ++i) {
        for (size_t j = i + 1; j < seedNodes.size(); ++j) {
            auto nodesInPath = getNodesInGeodesicPaths(graph, seedNodes[i], seedNodes[j]);
            allNodes.insert(nodesInPath.begin(), nodesInPath.end());
        }
    }
    
    return allNodes;
}

bool GraphAlgorithms::preservesDistances(const Graph& originalGraph, const Graph& subgraph, 
                                       const std::vector<int>& seedNodes) {
    
    auto originalDistances = getAllPairDistances(originalGraph, seedNodes);
    auto subgraphDistances = getAllPairDistances(subgraph, seedNodes);
    
    for (const auto& pair : originalDistances) {
        auto it = subgraphDistances.find(pair.first);
        if (it == subgraphDistances.end() || it->second != pair.second) {
            return false;
        }
    }
    
    return true;
}

std::set<int> GraphAlgorithms::getSeedPlusOneNodes(const Graph& graph, int seedNode, 
                                                  const std::vector<int>& allSeeds) {
    std::set<int> seedPlusOneNodes;
    
    for (int otherSeed : allSeeds) {
        if (otherSeed != seedNode) {
            auto allPaths = getAllShortestPaths(graph, seedNode, otherSeed);
            for (const auto& path : allPaths) {
                if (path.size() >= 2) {
                    int candidate = path[1];
                    if (std::find(allSeeds.begin(), allSeeds.end(), candidate) == allSeeds.end()) {
                        seedPlusOneNodes.insert(candidate);
                    }
                }
            }
        }
    }
    
    return seedPlusOneNodes;
}

std::set<int> GraphAlgorithms::getSeedPlusNNodes(const Graph& graph, int seedNode, 
                                                const std::vector<int>& allSeeds, int n) {
    std::set<int> seedPlusNNodes;
    
    if (n <= 0) {
        return seedPlusNNodes;
    }
    
    for (int otherSeed : allSeeds) {
        if (otherSeed != seedNode) {
            auto allPaths = getAllShortestPaths(graph, seedNode, otherSeed);
            for (const auto& path : allPaths) {
                if (path.size() > static_cast<size_t>(n)) {
                    int candidate = path[n];
                    if (std::find(allSeeds.begin(), allSeeds.end(), candidate) == allSeeds.end()) {
                        seedPlusNNodes.insert(candidate);
                    }
                }
            }
        }
    }
    
    return seedPlusNNodes;
}

std::unordered_map<int, int> GraphAlgorithms::bfsDistances(const Graph& graph, int source) {
    std::unordered_map<int, int> distances;
    std::queue<int> queue;
    
    distances[source] = 0;
    queue.push(source);
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        for (int neighbor : graph.getNeighbors(current)) {
            if (distances.find(neighbor) == distances.end()) {
                distances[neighbor] = distances[current] + 1;
                queue.push(neighbor);
            }
        }
    }
    
    return distances;
}

std::unordered_map<int, std::vector<int>> GraphAlgorithms::bfsPredecessors(
    const Graph& graph, int source, int target) {
    
    std::unordered_map<int, std::vector<int>> predecessors;
    std::unordered_map<int, int> distances;
    std::queue<int> queue;
    
    distances[source] = 0;
    queue.push(source);
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        if (current == target) {
            continue;
        }
        
        for (int neighbor : graph.getNeighbors(current)) {
            if (distances.find(neighbor) == distances.end()) {
                distances[neighbor] = distances[current] + 1;
                predecessors[neighbor].push_back(current);
                queue.push(neighbor);
            } else if (distances[neighbor] == distances[current] + 1) {
                predecessors[neighbor].push_back(current);
            }
        }
    }
    
    return predecessors;
}

void GraphAlgorithms::reconstructPaths(const std::unordered_map<int, std::vector<int>>& predecessors, 
                                     int current, int source, std::vector<int>& currentPath, 
                                     std::vector<std::vector<int>>& allPaths) {
    
    currentPath.push_back(current);
    
    if (current == source) {
        allPaths.push_back(currentPath);
    } else {
        auto it = predecessors.find(current);
        if (it != predecessors.end()) {
            for (int pred : it->second) {
                reconstructPaths(predecessors, pred, source, currentPath, allPaths);
            }
        }
    }
    
    currentPath.pop_back();
}

std::unordered_map<int, double> GraphAlgorithms::betweennessCentrality(const Graph& graph)
{
    std::vector<int> nodes = graph.getNodes();
    
    std::unordered_map<int, double> CB;
    for (int v : nodes) {
        CB[v] = 0.0;
    }
    
    for (int s : nodes) {
        std::vector<int> S;
        std::unordered_map<int, std::vector<int>> P;
        std::unordered_map<int, int> sigma;
        std::unordered_map<int, int> d;
        
        for (int v : nodes) {
            sigma[v] = 0;
            d[v] = -1;
        }
        sigma[s] = 1;
        d[s] = 0;
        
        std::queue<int> Q;
        Q.push(s);
        
        while (!Q.empty()) {
            int v = Q.front();
            Q.pop();
            S.push_back(v);
            
            for (int w : graph.getNeighbors(v)) {
                if (d[w] < 0) {
                    Q.push(w);
                    d[w] = d[v] + 1;
                }
                if (d[w] == d[v] + 1) {
                    sigma[w] += sigma[v];
                    P[w].push_back(v);
                }
            }
        }
        
        std::unordered_map<int, double> delta;
        for (int v : nodes) {
            delta[v] = 0.0;
        }
        
        while (!S.empty()) {
            int w = S.back();
            S.pop_back();
            for (int v : P[w]) {
                delta[v] += (static_cast<double>(sigma[v]) / sigma[w]) * (1.0 + delta[w]);
            }
            if (w != s) {
                CB[w] += delta[w];
            }
        }
    }
    
    for (auto& [v, val] : CB) {
        val /= 2.0;
    }
    
    int n = static_cast<int>(nodes.size());
    if (n > 2) {
        double normFactor = static_cast<double>(n - 1) * (n - 2) / 2.0;
        for (auto& [v, val] : CB) {
            val /= normFactor;
        }
    }
    
    return CB;
}
