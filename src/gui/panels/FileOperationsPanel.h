/**
 * @file FileOperationsPanel.h
 * @brief Panel for file operations like loading networks
 */

#ifndef FILEOPERATIONSPANEL_H
#define FILEOPERATIONSPANEL_H

#include <QWidget>
#include <QString>

class QVBoxLayout;
class QGroupBox;
class QLabel;
class QPushButton;
class GraphFilterPanel;
struct GraphFilterConfig;

/**
 * @brief Panel for file operations and network loading
 * 
 * This panel handles all file-related operations including loading network files,
 * displaying file status, and managing the current graph state.
 */
class FileOperationsPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit FileOperationsPanel(QWidget *parent = nullptr);

    /**
     * @brief Get the current file path
     * @return Path to the currently loaded file
     */
    QString getCurrentFilePath() const;

    /**
     * @brief Check if a valid graph is loaded
     * @return True if a valid graph is loaded
     */
    bool hasValidGraph() const;

public slots:
    /**
     * @brief Update the file status display
     * @param fileName Name of the loaded file
     * @param filePath Full path to the loaded file
     * @param nodeCount Number of nodes in the graph
     * @param edgeCount Number of edges in the graph
     * @param rejectedSelfLoops Number of self-loops rejected
     * @param isolatedNodes Number of nodes with only self-loops
     */
    void updateFileStatus(const QString& fileName, const QString& filePath, int nodeCount, int edgeCount, 
                         int rejectedSelfLoops = 0, int isolatedNodes = 0,
                         int initialConnections = 0, int nonUniqueInteractions = 0);

    /**
     * @brief Clear the file status
     */
    void clearFileStatus();

    /**
     * @brief Enable or disable the panel
     * @param enabled Whether the panel should be enabled
     */
    void setEnabled(bool enabled);

    /**
     * @brief Update graph filter statistics (max degree, etc.)
     * @param maxDegree Maximum degree in the graph
     * @param nodeCount Number of nodes
     * @param edgeCount Number of edges
     */
    void updateGraphFilterStats(int maxDegree, int nodeCount, int edgeCount);

signals:
    /**
     * @brief Emitted when load network is requested
     */
    void loadNetworkRequested();

    /**
     * @brief Emitted when a network has been successfully loaded
     * @param filePath Path to the loaded file
     */
    void networkLoaded(const QString& filePath);
    
    /**
     * @brief Emitted when recent networks button is clicked
     */
    void recentNetworksRequested();
    
    /**
     * @brief Emitted when recent seeds button is clicked
     */
    void recentSeedsRequested();
    
    /**
     * @brief Emitted when export statistics is requested
     */
    void exportStatisticsRequested();
    
    /**
     * @brief Emitted when user requests to apply graph filters
     */
    void applyFiltersRequested(const GraphFilterConfig& config);
    
    /**
     * @brief Emitted when user requests to reset graph to original
     */
    void resetFiltersRequested();

private slots:
    /**
     * @brief Handle load network button click
     */
    void onLoadNetworkClicked();
    
    /**
     * @brief Handle recent networks button click
     */
    void onRecentNetworksClicked();
    
    /**
     * @brief Handle recent seeds button click
     */
    void onRecentSeedsClicked();
    
    /**
     * @brief Handle export statistics button click
     */
    void onExportStatisticsClicked();

private:
    /**
     * @brief Setup the user interface
     */
    void setupUI();

    QPushButton* loadGraphButton_;
    QPushButton* recentNetworksButton_;
    QPushButton* recentSeedsButton_;
    QPushButton* exportStatsButton_;
    QLabel* fileStatusLabel_;
    QLabel* graphStatsLabel_;
    GraphFilterPanel* graphFilterPanel_;
    
    QString currentFilePath_;
    bool hasValidGraph_;
};

#endif