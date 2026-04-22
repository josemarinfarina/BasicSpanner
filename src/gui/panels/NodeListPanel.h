/**
 * @file NodeListPanel.h
 * @brief Panel showing a list of nodes with hover highlighting
 */

#ifndef NODELISTPANEL_H
#define NODELISTPANEL_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QString>
#include <vector>
#include <string>
#include <map>

/**
 * @brief Node data structure for display
 */
struct NodeData {
    std::string name;
    int degreeGeneral = 0;
    int degreeLocal = 0;
    double betweennessGeneral = 0.0;
    double betweennessLocal = 0.0;
    double frequency = 0.0;
    QString type = "Regular";
};

/**
 * @brief Panel displaying a searchable list of network nodes
 * 
 * Shows all nodes in a table with their properties. When hovering over
 * a node in the list, it gets highlighted in the graph viewer.
 */
class NodeListPanel : public QWidget
{
    Q_OBJECT

public:
    explicit NodeListPanel(QWidget *parent = nullptr);
    ~NodeListPanel() = default;

    /**
     * @brief Set the list of nodes to display with full data
     * @param nodeData Vector of NodeData structs
     */
    void setNodeData(const std::vector<NodeData>& nodeData);

    /**
     * @brief Set the list of nodes to display (simple version)
     * @param nodeNames Vector of node names
     * @param nodeTypes Map of node name to type (seed, connector, regular)
     */
    void setNodes(const std::vector<std::string>& nodeNames,
                  const std::map<std::string, QString>& nodeTypes = {});

    /**
     * @brief Update node degrees
     * @param degrees Map of node name to degree
     */
    void updateDegrees(const std::map<std::string, int>& degrees);

    /**
     * @brief Update connector frequencies
     * @param frequencies Map of node name to frequency percentage
     */
    void updateFrequencies(const std::map<std::string, double>& frequencies);

    /**
     * @brief Clear all nodes from the list
     */
    void clearNodes();

    /**
     * @brief Update node type information
     * @param nodeTypes Map of node name to type string
     */
    void updateNodeTypes(const std::map<std::string, QString>& nodeTypes);

signals:
    /**
     * @brief Emitted when mouse hovers over a node row
     * @param nodeName Name of the hovered node
     */
    void nodeHovered(const std::string& nodeName);

    /**
     * @brief Emitted when mouse leaves a node row
     */
    void nodeUnhovered();

    /**
     * @brief Emitted when a node is clicked
     * @param nodeName Name of the clicked node
     */
    void nodeSelected(const std::string& nodeName);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onCellEntered(int row, int column);
    void onItemClicked(QTableWidgetItem* item);

private:
    void setupUI();
    void filterNodes(const QString& filter);

    QLineEdit* searchBox_;
    QTableWidget* nodeTable_;
    QLabel* countLabel_;
    
    std::vector<NodeData> allNodeData_;
    std::map<std::string, int> nodeDegrees_;
    std::map<std::string, double> nodeFrequencies_;
};

#endif
