/**
 * @file SplitGraphView.h
 * @brief Split view widget for comparing graph attributes
 * 
 * Provides a side-by-side view of the same graph with different
 * color attributes (betweenness, degree, closeness, etc.)
 */

#ifndef SPLITGRAPHVIEW_H
#define SPLITGRAPHVIEW_H

#include <QWidget>
#include <QSplitter>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <map>
#include <vector>
#include <string>

#include "GraphVisualization.h"
#include "Graph.h"

/**
 * @brief Enumeration of available node coloring attributes
 */
enum class NodeColorAttribute {
    None = 0,
    Degree,
    Betweenness,
    Closeness,
    Eigenvector,
    ClusteringCoeff,
    PageRank
};

/**
 * @brief Widget providing split view comparison of graph attributes
 * 
 * This widget shows two synchronized views of the same graph,
 * each colorable by different network metrics. Useful for
 * comparing betweenness vs degree, for example.
 */
class SplitGraphView : public QWidget
{
    Q_OBJECT

public:
    explicit SplitGraphView(QWidget* parent = nullptr);
    ~SplitGraphView();

    /**
     * @brief Set the graph to visualize in both views
     */
    void setGraph(const Graph& graph);
    
    /**
     * @brief Display a graph (slot for signal connection)
     */
    void displayGraph(const Graph& graph);
    
    /**
     * @brief Set seeds for highlighting
     */
    void setCurrentSeeds(const std::vector<std::string>& seeds);
    
    /**
     * @brief Set connector frequency data for FWTL
     */
    void setConnectorFrequencyData(const std::map<int, int>& freq, int totalPerm);
    
    /**
     * @brief Highlight specific nodes (seeds)
     */
    void highlightSeedsByName(const std::vector<std::string>& seedNames);
    
    /**
     * @brief Highlight connectors with frequency
     */
    void highlightConnectorsWithFrequency(const std::map<int, int>& connectorFrequency,
                                          int totalPermutations,
                                          const std::set<int>& bestConnectors,
                                          const QColor& connectorColor,
                                          const QColor& extraConnectorColor);
    
    /**
     * @brief Get the primary (left) graph visualization
     */
    GraphVisualization* primaryView() { return primaryGraph_; }
    
    /**
     * @brief Get the secondary (right) graph visualization
     */
    GraphVisualization* secondaryView() { return secondaryGraph_; }
    
    /**
     * @brief Enable or disable split view mode
     */
    void setSplitViewEnabled(bool enabled);
    bool isSplitViewEnabled() const { return splitEnabled_; }
    
    /**
     * @brief Set layout parameters for both views
     */
    void setLayoutParameters(const LayoutParameters& params);
    
    /**
     * @brief Apply layout to both views (synchronized)
     */
    void applyLayout();
    
    /**
     * @brief Set layout type for both views
     */
    void setLayoutType(LayoutType type);

signals:
    /**
     * @brief Emitted when a node is clicked in either view
     */
    void nodeClicked(const std::string& nodeName);

public slots:
    /**
     * @brief Toggle split view on/off
     */
    void toggleSplitView();
    
    /**
     * @brief Sync positions from primary to secondary view
     */
    void syncPositions();
    
    /**
     * @brief Sync camera (zoom/pan) from primary to secondary view
     */
    void syncCamera();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onPrimaryAttributeChanged(int index);
    void onSecondaryAttributeChanged(int index);
    void onSyncLayoutToggled(bool sync);

private:
    void setupUI();
    void setupConnections();
    
    /**
     * @brief Compute the specified attribute for all nodes
     */
    std::map<int, double> computeAttribute(NodeColorAttribute attr);
    
    /**
     * @brief Apply color gradient based on attribute values
     */
    void applyAttributeColoring(GraphVisualization* view, 
                                 NodeColorAttribute attr,
                                 const std::map<int, double>& values);
    
    /**
     * @brief Get color from value using gradient (blue -> yellow -> red)
     */
    QColor valueToColor(double normalizedValue);
    
    std::map<int, double> computeDegree();
    std::map<int, double> computeBetweenness();
    std::map<int, double> computeCloseness();
    std::map<int, double> computeEigenvector();
    std::map<int, double> computeClusteringCoeff();
    std::map<int, double> computePageRank();
    
    QSplitter* splitter_;
    QWidget* primaryContainer_;
    QWidget* secondaryContainer_;
    GraphVisualization* primaryGraph_;
    GraphVisualization* secondaryGraph_;
    
    QGroupBox* controlPanel_;
    QCheckBox* splitViewCheckBox_;
    QComboBox* primaryAttributeCombo_;
    QComboBox* secondaryAttributeCombo_;
    QCheckBox* syncLayoutCheckBox_;
    QPushButton* syncNowButton_;
    QLabel* primaryLabel_;
    QLabel* secondaryLabel_;
    
    QWidget* primaryLegend_;
    QWidget* secondaryLegend_;
    
    Graph graph_;
    bool splitEnabled_;
    bool syncLayout_;
    bool isSyncing_ = false;
    NodeColorAttribute primaryAttribute_;
    NodeColorAttribute secondaryAttribute_;
    
    std::map<NodeColorAttribute, std::map<int, double>> cachedAttributes_;
    
    std::map<int, int> storedConnectorFrequency_;
    int storedTotalPermutations_ = 0;
    std::set<int> storedBestConnectors_;
    QColor storedConnectorColor_;
    QColor storedExtraConnectorColor_;
    bool hasConnectorData_ = false;
};

#endif
