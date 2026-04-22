#ifndef GRAPH_ALGORITHMS_H
#define GRAPH_ALGORITHMS_H

#include "Graph.h"
#include <vector>
#include <unordered_map>
#include <set>

struct PathInfo {
    int distance;
    std::vector<std::vector<int>> allPaths;
    
    PathInfo() : distance(-1) {}
    PathInfo(int dist) : distance(dist) {}
};

struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

class GraphAlgorithms {
public:
    static int getDistance(const Graph& graph, int source, int target);
    static std::unordered_map<int, int> getDistancesFromSource(const Graph& graph, int source);
    static std::unordered_map<std::pair<int, int>, int, PairHash> getAllPairDistances(const Graph& graph, const std::vector<int>& nodes);
    
    static std::vector<int> getShortestPath(const Graph& graph, int source, int target);
    static std::vector<std::vector<int>> getAllShortestPaths(const Graph& graph, int source, int target);
    static PathInfo getPathInfo(const Graph& graph, int source, int target);
    
    static std::unordered_map<std::pair<int, int>, PathInfo, PairHash> getAllPairPathInfo(
        const Graph& graph, const std::vector<int>& seedNodes);
    
    static std::set<int> getNodesInGeodesicPaths(const Graph& graph, int source, int target);
    static std::set<int> getAllNodesInSeedGeodesics(const Graph& graph, const std::vector<int>& seedNodes);
    
    static bool preservesDistances(const Graph& originalGraph, const Graph& subgraph, 
                                 const std::vector<int>& seedNodes);
    
    static std::set<int> getSeedPlusOneNodes(const Graph& graph, int seedNode, 
                                           const std::vector<int>& allSeeds);
    static std::set<int> getSeedPlusNNodes(const Graph& graph, int seedNode, 
                                         const std::vector<int>& allSeeds, int n);

    static std::unordered_map<int, double> betweennessCentrality(const Graph& graph);
    
    static std::unordered_map<int, int> bfsDistances(const Graph& graph, int source);
    static std::unordered_map<int, std::vector<int>> bfsPredecessors(const Graph& graph, int source, int target);
    static void reconstructPaths(const std::unordered_map<int, std::vector<int>>& predecessors, 
                               int current, int source, std::vector<int>& currentPath, 
                               std::vector<std::vector<int>>& allPaths);

private:
};

#endif