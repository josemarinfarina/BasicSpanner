#include "Graph.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
#include <stdexcept>

Graph::Graph() : nextNodeId_(0), nextAlphaId_(100000), rejectedSelfLoopsCount_(0), initialConnectionsCount_(0) {
}

Graph::~Graph() {
}

void Graph::addNode(int nodeId, const std::string& nodeName) {
    if (adjacencyList_.find(nodeId) == adjacencyList_.end()) {
        adjacencyList_[nodeId] = std::vector<int>();
        
        if (!nodeName.empty()) {
            nodeIdToName_[nodeId] = nodeName;
            nodeNameToId_[nodeName] = nodeId;
        } else {
            nodeIdToName_[nodeId] = std::to_string(nodeId);
        }
        
        if (nodeId >= nextNodeId_) {
            nextNodeId_ = nodeId + 1;
        }
    }
}

void Graph::addEdge(int node1, int node2) {
    if (node1 == node2) {
        rejectedSelfLoopsCount_++;
        ensureNodeExists(node1);
        return;
    }
    
    ensureNodeExists(node1);
    ensureNodeExists(node2);
    
    auto& neighbors1 = adjacencyList_[node1];
    auto& neighbors2 = adjacencyList_[node2];
    
    if (std::find(neighbors1.begin(), neighbors1.end(), node2) == neighbors1.end()) {
        neighbors1.push_back(node2);
    }
    
    if (std::find(neighbors2.begin(), neighbors2.end(), node1) == neighbors2.end()) {
        neighbors2.push_back(node1);
    }
}

void Graph::addEdge(const std::string& node1Name, const std::string& node2Name) {
    if (node1Name == node2Name) {
        rejectedSelfLoops_.push_back({node1Name, node2Name});
        uniqueSelfLoops_.insert({node1Name, node2Name});
    }
    
    int node1Id, node2Id;
    
    if (nodeNameToId_.find(node1Name) == nodeNameToId_.end()) {
        node1Id = nextNodeId_++;
        addNode(node1Id, node1Name);
    } else {
        node1Id = nodeNameToId_[node1Name];
    }
    
    if (nodeNameToId_.find(node2Name) == nodeNameToId_.end()) {
        node2Id = nextNodeId_++;
        addNode(node2Id, node2Name);
    } else {
        node2Id = nodeNameToId_[node2Name];
    }
    
    addEdge(node1Id, node2Id);
}

int Graph::getNodeCount() const {
    return adjacencyList_.size();
}

int Graph::getEdgeCount() const {
    int edgeCount = 0;
    for (const auto& pair : adjacencyList_) {
        edgeCount += pair.second.size();
    }
    return edgeCount / 2;
}

int Graph::getRejectedSelfLoopsCount() const {
    return rejectedSelfLoopsCount_;
}

int Graph::getIsolatedNodesCount() const {
    return nodesWithOnlySelfLoops_.size();
}

void Graph::resetSelfLoopStats() {
    rejectedSelfLoopsCount_ = 0;
    nodesWithOnlySelfLoops_.clear();
    rejectedSelfLoops_.clear();
    uniqueSelfLoops_.clear();
    initialConnectionsCount_ = 0;
}

void Graph::calculateIsolatedNodes() {
    nodesWithOnlySelfLoops_.clear();
    for (const auto& pair : adjacencyList_) {
        if (pair.second.empty()) {
            nodesWithOnlySelfLoops_.insert(pair.first);
        }
    }
}

std::vector<std::string> Graph::getIsolatedNodeNames() const {
    std::vector<std::string> names;
    for (int nodeId : nodesWithOnlySelfLoops_) {
        names.push_back(getNodeName(nodeId));
    }
    return names;
}

std::vector<std::pair<std::string, std::string>> Graph::getRejectedSelfLoops() const {
    return rejectedSelfLoops_;
}

int Graph::getInitialConnectionsCount() const {
    return initialConnectionsCount_;
}

int Graph::getNonUniqueInteractionsCount() const {
    return getEdgeCount() + uniqueSelfLoops_.size();
}

bool Graph::hasNode(int nodeId) const {
    return adjacencyList_.find(nodeId) != adjacencyList_.end();
}

bool Graph::hasNode(const std::string& nodeName) const {
    return nodeNameToId_.find(nodeName) != nodeNameToId_.end();
}

bool Graph::hasEdge(int node1, int node2) const {
    if (!hasNode(node1) || !hasNode(node2)) {
        return false;
    }
    
    const auto& neighbors = adjacencyList_.at(node1);
    return std::find(neighbors.begin(), neighbors.end(), node2) != neighbors.end();
}

std::vector<int> Graph::getNodes() const {
    std::vector<int> nodes;
    for (const auto& pair : adjacencyList_) {
        nodes.push_back(pair.first);
    }
    return nodes;
}

std::vector<int> Graph::getNeighbors(int nodeId) const {
    if (!hasNode(nodeId)) {
        return std::vector<int>();
    }
    return adjacencyList_.at(nodeId);
}

namespace {
    std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t start = 0, end = 0;
        
        std::string line = s;
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), line.end());

        while ((end = line.find(delimiter, start)) != std::string::npos) {
            tokens.push_back(line.substr(start, end - start));
            start = end + delimiter.length();
        }
        tokens.push_back(line.substr(start));
        return tokens;
    }
}

std::string Graph::getNodeName(int nodeId) const {
    auto it = nodeIdToName_.find(nodeId);
    if (it != nodeIdToName_.end()) {
        return it->second;
    }
    
    return getOriginalId(nodeId);
}

int Graph::getNodeId(const std::string& nodeName) const {
    auto it = nodeNameToId_.find(nodeName);
    if (it != nodeNameToId_.end()) {
        return it->second;
    }
    return -1;
}

Graph Graph::getSubgraph(const std::set<int>& nodeIds) const {
    Graph subgraph;
    
    for (int nodeId : nodeIds) {
        if (hasNode(nodeId)) {
            subgraph.addNode(nodeId, getNodeName(nodeId));
        }
    }
    
    for (int nodeId : nodeIds) {
        if (hasNode(nodeId)) {
            for (int neighbor : getNeighbors(nodeId)) {
                if (nodeIds.find(neighbor) != nodeIds.end() && nodeId <= neighbor) {
                    subgraph.addEdge(nodeId, neighbor);
                }
            }
        }
    }
    
    return subgraph;
}

void Graph::clear() {
    adjacencyList_.clear();
    nodeIdToName_.clear();
    nodeNameToId_.clear();
    alphaIdToIntId_.clear();
    intIdToAlphaId_.clear();
    nextNodeId_ = 0;
    nextAlphaId_ = 100000;
    resetSelfLoopStats();
}

bool Graph::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    clear();
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        auto nodes = parseEdgeListLine(line);
        if (nodes.size() >= 2) {
            addEdge(nodes[0], nodes[1]);
        }
    }
    
    file.close();
    return true;
}

/**
 * @brief Load graph from file with specific column configuration
 * 
 * @param filename Path to the file containing graph data
 * @param sourceColumn Column index for source nodes (0-based)
 * @param targetColumn Column index for target nodes (0-based)
 * @param separator Character used to separate columns
 * @param hasHeader Whether the file has a header row
 * @param skipLines Number of lines to skip at the beginning
 * @param nodeNameColumn Column index for node names (-1 if not present)
 * @param applyNamesToTarget Whether to apply names to target nodes as well
 * @return true if loading was successful, false otherwise
 * 
 */
bool Graph::loadFromFileWithColumns(const std::string& filename, 
                                   int sourceColumn, int targetColumn,
                                   const std::string& separator,
                                   bool hasHeader, int skipLines,
                                   int nodeNameColumn,
                                   bool applyNamesToTarget,
                                   bool caseSensitive) {
    
    std::cout << "=== IMPORT START (Line-by-Line Processing) ===" << std::endl;
    std::cout << "File: " << filename << std::endl;
    std::cout << "Source column: " << sourceColumn << std::endl;
    std::cout << "Target column: " << targetColumn << std::endl;
    std::cout << "Case sensitive: " << (caseSensitive ? "Yes" : "No") << std::endl;
    std::cout << "Names column: " << nodeNameColumn << std::endl;
    std::cout << "Separator: '" << separator << "'" << std::endl;
    std::cout << "Has header: " << (hasHeader ? "Yes" : "No") << std::endl;
    std::cout << "Lines to skip: " << skipLines << std::endl;
    std::cout << "==============================================" << std::endl;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "ERROR: Could not open file" << std::endl;
        return false;
    }
    
    clear();
    
    std::string line;
    int lineCount = 0;
    
    for (int i = 0; i < skipLines; ++i) {
        if (!std::getline(file, line)) {
            return true;
        }
        lineCount++;
    }
    
    if (hasHeader) {
        if (!std::getline(file, line)) {
            return true;
        }
        lineCount++;
    }

    while (std::getline(file, line)) {
        lineCount++;
        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto columns = split(line, separator);

        if (columns.size() > sourceColumn && columns.size() > targetColumn) {
            std::string sourceName = columns[sourceColumn];
            std::string targetName = columns[targetColumn];

            auto trim_whitespace = [](std::string& s) {
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), s.end());
            };

            trim_whitespace(sourceName);
            trim_whitespace(targetName);
            
            if (!caseSensitive) {
                std::transform(sourceName.begin(), sourceName.end(), sourceName.begin(), ::toupper);
                std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::toupper);
            }
            
            static int debugCount = 0;
            if (!caseSensitive && debugCount < 5) {
                std::cout << "DEBUG: Normalized - " << sourceName << " <-> " << targetName << std::endl;
                debugCount++;
            }

            if (!sourceName.empty() && !targetName.empty()) {
                initialConnectionsCount_++;
                addEdge(sourceName, targetName);
            }
        }
    }

    file.close();
    
    calculateIsolatedNodes();
    
    std::cout << "=== IMPORT COMPLETE ===" << std::endl;
    std::cout << "Total lines processed: " << lineCount << std::endl;
    std::cout << "Initial connections: " << getInitialConnectionsCount() << std::endl;
    std::cout << "Non-unique interactions: " << getNonUniqueInteractionsCount() 
              << " (edges: " << getEdgeCount() << " + unique self-loops: " << uniqueSelfLoops_.size() << ")" << std::endl;
    std::cout << "Graph contains " << getNodeCount() << " nodes and " 
              << getEdgeCount() << " edges." << std::endl;
    std::cout << "Rejected self-loops: " << getRejectedSelfLoopsCount() 
              << " (" << uniqueSelfLoops_.size() << " unique)" << std::endl;
    std::cout << "Isolated nodes: " << getIsolatedNodesCount() << std::endl;
              
    return true;
}

bool Graph::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::set<std::pair<int, int>> writtenEdges;
    
    for (const auto& pair : adjacencyList_) {
        int node1 = pair.first;
        for (int node2 : pair.second) {
            std::pair<int, int> edge = {std::min(node1, node2), std::max(node1, node2)};
            if (writtenEdges.find(edge) == writtenEdges.end()) {
                file << getNodeName(node1) << "\t" << getNodeName(node2) << std::endl;
                writtenEdges.insert(edge);
            }
        }
    }
    
    file.close();
    return true;
}

void Graph::printGraph() const {
    std::cout << "Graph with " << getNodeCount() << " nodes and " << getEdgeCount() << " edges:" << std::endl;
    
    for (const auto& pair : adjacencyList_) {
        int nodeId = pair.first;
        std::cout << getNodeName(nodeId) << " (" << nodeId << "): ";
        for (int neighbor : pair.second) {
            std::cout << getNodeName(neighbor) << " ";
        }
        std::cout << std::endl;
    }
}

std::vector<std::pair<int, int>> Graph::getEdges() const {
    std::vector<std::pair<int, int>> edges;
    std::set<std::pair<int, int>> addedEdges;
    
    for (const auto& pair : adjacencyList_) {
        int node1 = pair.first;
        for (int node2 : pair.second) {
            std::pair<int, int> edge = {std::min(node1, node2), std::max(node1, node2)};
            if (addedEdges.find(edge) == addedEdges.end()) {
                edges.push_back(edge);
                addedEdges.insert(edge);
            }
        }
    }
    
    return edges;
}

bool Graph::isConnected() const {
    if (adjacencyList_.empty()) {
        return true;
    }
    
    std::set<int> visited;
    std::queue<int> queue;
    
    int startNode = adjacencyList_.begin()->first;
    queue.push(startNode);
    visited.insert(startNode);
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        for (int neighbor : getNeighbors(current)) {
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                queue.push(neighbor);
            }
        }
    }
    
    return visited.size() == adjacencyList_.size();
}

std::vector<std::vector<int>> Graph::getConnectedComponents() const {
    std::vector<std::vector<int>> components;
    std::set<int> visited;
    
    for (const auto& pair : adjacencyList_) {
        int startNode = pair.first;
        if (visited.find(startNode) == visited.end()) {
            std::vector<int> component;
            std::queue<int> queue;
            
            queue.push(startNode);
            visited.insert(startNode);
            
            while (!queue.empty()) {
                int current = queue.front();
                queue.pop();
                component.push_back(current);
                
                for (int neighbor : getNeighbors(current)) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        queue.push(neighbor);
                    }
                }
            }
            
            components.push_back(component);
        }
    }
    
    return components;
}

void Graph::ensureNodeExists(int nodeId) {
    if (adjacencyList_.find(nodeId) == adjacencyList_.end()) {
        addNode(nodeId);
    }
}

bool Graph::isValidNodeId(int nodeId) const {
    return nodeId >= 0;
}

std::vector<std::string> Graph::parseEdgeListLine(const std::string& line) const {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

int Graph::convertToIntId(const std::string& id) {
    if (isNumericId(id)) {
        return std::stoi(id);
    }
    
    auto it = alphaIdToIntId_.find(id);
    if (it != alphaIdToIntId_.end()) {
        return it->second;
    }
    
    int newIntId = nextAlphaId_++;
    alphaIdToIntId_[id] = newIntId;
    intIdToAlphaId_[newIntId] = id;
    
    std::cout << "  Mapping created: '" << id << "' -> " << newIntId << std::endl;
    
    return newIntId;
}

std::string Graph::getOriginalId(int intId) const {
    auto it = intIdToAlphaId_.find(intId);
    if (it != intIdToAlphaId_.end()) {
        return it->second;
    }
    return std::to_string(intId);
}

bool Graph::isNumericId(const std::string& id) const {
    if (id.empty()) return false;
    
    size_t start = 0;
    if (id[0] == '+' || id[0] == '-') {
        start = 1;
    }
    
    for (size_t i = start; i < id.length(); ++i) {
        if (!std::isdigit(id[i])) {
            return false;
        }
    }
    
    return start < id.length();
} 