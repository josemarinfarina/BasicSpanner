/**
 * @file Graph.h
 * @brief Graph class definition for network representation and manipulation
 */

#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>
#include <string>
#include <set>
#include <iostream>

/**
 * @brief Graph class for representing and manipulating networks
 * 
 * This class provides a complete interface for creating, modifying, and analyzing
 * graph structures. It supports both numeric and string node identifiers and
 * provides various utility functions for graph operations.
 * 
 */
class Graph {
public:
    /**
     * @brief Default constructor
     * 
     * Creates an empty graph with no nodes or edges
     */
    Graph();
    
    /**
     * @brief Destructor
     * 
     * Cleans up all graph data structures
     */
    ~Graph();
    
    /**
     * @brief Add a node to the graph
     * 
     * @param nodeId Unique identifier for the node
     * @param nodeName Optional human-readable name for the node
     */
    void addNode(int nodeId, const std::string& nodeName = "");
    
    /**
     * @brief Add an edge between two nodes using node IDs
     * 
     * @param node1 ID of the first node
     * @param node2 ID of the second node
     */
    void addEdge(int node1, int node2);
    
    /**
     * @brief Add an edge between two nodes using node names
     * 
     * @param node1Name Name of the first node
     * @param node2Name Name of the second node
     */
    void addEdge(const std::string& node1Name, const std::string& node2Name);
    
    /**
     * @brief Get the number of nodes in the graph
     * 
     * @return Total number of nodes
     */
    int getNodeCount() const;
    
    /**
     * @brief Get the number of edges in the graph
     * 
     * @return Total number of edges
     */
    int getEdgeCount() const;
    
    /**
     * @brief Get the number of self-loops rejected during import
     * 
     * @return Number of self-loops that were rejected
     */
    int getRejectedSelfLoopsCount() const;
    
    /**
     * @brief Get the number of isolated nodes (only connected to themselves)
     * 
     * @return Number of nodes that only had self-loop connections
     */
    int getIsolatedNodesCount() const;
    
    /**
     * @brief Reset self-loop statistics
     */
    void resetSelfLoopStats();
    
    /**
     * @brief Calculate isolated nodes (nodes with degree 0)
     * Should be called after loading is complete
     */
    void calculateIsolatedNodes();
    
    /**
     * @brief Get list of isolated node names
     * 
     * @return Vector of node names that are isolated
     */
    std::vector<std::string> getIsolatedNodeNames() const;
    
    /**
     * @brief Get list of rejected self-loops (node pairs)
     * 
     * @return Vector of node name pairs that were rejected as self-loops
     */
    std::vector<std::pair<std::string, std::string>> getRejectedSelfLoops() const;
    
    /**
     * @brief Get initial connections count (total rows processed)
     * 
     * @return Number of rows processed from input file
     */
    int getInitialConnectionsCount() const;
    
    /**
     * @brief Get non-unique interactions count
     * 
     * @return Sum of edges plus unique self-loops
     */
    int getNonUniqueInteractionsCount() const;
    
    /**
     * @brief Check if a node exists in the graph
     * 
     * @param nodeId ID of the node to check
     * @return true if the node exists, false otherwise
     */
    bool hasNode(int nodeId) const;
    
    /**
     * @brief Check if a node with given name exists in the graph
     * 
     * @param nodeName Name of the node to check
     * @return true if the node exists, false otherwise
     */
    bool hasNode(const std::string& nodeName) const;
    
    /**
     * @brief Check if an edge exists between two nodes
     * 
     * @param node1 ID of the first node
     * @param node2 ID of the second node
     * @return true if the edge exists, false otherwise
     */
    bool hasEdge(int node1, int node2) const;
    
    /**
     * @brief Get all node IDs in the graph
     * 
     * @return Vector containing all node IDs
     */
    std::vector<int> getNodes() const;
    
    /**
     * @brief Get all neighbors of a given node
     * 
     * @param nodeId ID of the node
     * @return Vector containing IDs of all neighboring nodes
     */
    std::vector<int> getNeighbors(int nodeId) const;
    
    /**
     * @brief Get the name of a node given its ID
     * 
     * @param nodeId ID of the node
     * @return Name of the node, or empty string if not found
     */
    std::string getNodeName(int nodeId) const;
    
    /**
     * @brief Get the ID of a node given its name
     * 
     * @param nodeName Name of the node
     * @return ID of the node, or -1 if not found
     */
    int getNodeId(const std::string& nodeName) const;
    
    /**
     * @brief Create a subgraph containing only specified nodes
     * 
     * @param nodeIds Set of node IDs to include in the subgraph
     * @return New Graph object containing only the specified nodes and their edges
     */
    Graph getSubgraph(const std::set<int>& nodeIds) const;
    
    /**
     * @brief Remove all nodes and edges from the graph
     */
    void clear();
    
    /**
     * @brief Load graph from a file
     * 
     * @param filename Path to the file containing graph data
     * @return true if loading was successful, false otherwise
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Load graph from a file with specific column configuration
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
     */
    bool loadFromFileWithColumns(const std::string& filename, 
                                int sourceColumn, int targetColumn,
                                const std::string& separator = " ",
                                bool hasHeader = false, int skipLines = 0,
                                int nodeNameColumn = -1,
                                bool applyNamesToTarget = true,
                                bool caseSensitive = true);
    
    /**
     * @brief Save graph to a file
     * 
     * @param filename Path where to save the graph data
     * @return true if saving was successful, false otherwise
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Print graph structure to console for debugging
     */
    void printGraph() const;
    
    /**
     * @brief Get all edges in the graph
     * 
     * @return Vector of pairs representing all edges
     */
    std::vector<std::pair<int, int>> getEdges() const;
    
    /**
     * @brief Check if the graph is connected
     * 
     * @return true if all nodes are reachable from any other node, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Get all connected components in the graph
     * 
     * @return Vector of vectors, each containing node IDs of a connected component
     */
    std::vector<std::vector<int>> getConnectedComponents() const;

private:
    std::unordered_map<int, std::vector<int>> adjacencyList_;
    
    std::unordered_map<int, std::string> nodeIdToName_;
    std::unordered_map<std::string, int> nodeNameToId_;
    
    int nextNodeId_;
    
    std::unordered_map<std::string, int> alphaIdToIntId_;
    std::unordered_map<int, std::string> intIdToAlphaId_;
    int nextAlphaId_;
    
    int rejectedSelfLoopsCount_;
    std::set<int> nodesWithOnlySelfLoops_;
    std::vector<std::pair<std::string, std::string>> rejectedSelfLoops_;
    
    int initialConnectionsCount_;
    std::set<std::pair<std::string, std::string>> uniqueSelfLoops_;
    
    /**
     * @brief Ensure that a node exists in the graph
     * 
     * @param nodeId ID of the node to ensure exists
     */
    void ensureNodeExists(int nodeId);
    
    /**
     * @brief Check if a node ID is valid
     * 
     * @param nodeId ID to validate
     * @return true if the ID is valid, false otherwise
     */
    bool isValidNodeId(int nodeId) const;
    
    /**
     * @brief Parse a line from an edge list file
     * 
     * @param line Line to parse
     * @return Vector of strings representing the parsed components
     */
    std::vector<std::string> parseEdgeListLine(const std::string& line) const;
    
    /**
     * @brief Convert string ID to integer ID
     * 
     * @param id String identifier to convert
     * @return Integer representation of the ID
     */
    int convertToIntId(const std::string& id);
    
    /**
     * @brief Get original string ID from integer ID
     * 
     * @param intId Integer ID to convert back
     * @return Original string representation
     */
    std::string getOriginalId(int intId) const;
    
    /**
     * @brief Check if an ID string is numeric
     * 
     * @param id String to check
     * @return true if the string represents a number, false otherwise
     */
    bool isNumericId(const std::string& id) const;
};

#endif