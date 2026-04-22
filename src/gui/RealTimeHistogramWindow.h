/**
 * @file RealTimeHistogramWindow.h
 * @brief Window for displaying real-time connector histogram
 */

#ifndef REALTIMEHISTOGRAMWINDOW_H
#define REALTIMEHISTOGRAMWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QTextEdit>
#include <QProgressBar>
#include <QTableWidget>
#include <QHeaderView>

#include <QPainter>
#include <QPaintEvent>

class VerticalHistogramWidget;

/**
 * @brief Window for displaying real-time connector histogram
 * 
 * This window shows a histogram of connectors found in permutations
 * and updates in real-time as new permutations complete.
 */
class RealTimeHistogramWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit RealTimeHistogramWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~RealTimeHistogramWindow();

public slots:
    /**
     * @brief Add a new connector count to the histogram
     * @param permutation Permutation number
     * @param totalPermutations Total permutations
     * @param connectorsFound Number of connectors found
     */
    void addConnectorData(int permutation, int totalPermutations, int connectorsFound);

    /**
     * @brief Add a new connector data with names to the histogram
     * @param permutation Permutation number
     * @param totalPermutations Total permutations
     * @param connectorsFound Number of connectors found
     * @param connectorNames Vector of connector names
     */
    void addConnectorDataWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames);

    /**
     * @brief Add a new connector data with names and seeds to the histogram
     * @param permutation Permutation number
     * @param totalPermutations Total permutations
     * @param connectorsFound Number of connectors found
     * @param connectorNames Vector of connector names
     * @param seedNames Vector of seed names
     */
    void addConnectorDataWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames, int preConnectorsFound = -1);

    /**
     * @brief Start monitoring for real-time updates
     */
    void startMonitoring();

    /**
     * @brief Stop monitoring for real-time updates
     */
    void stopMonitoring();

    /**
     * @brief Clear all data and reset the histogram
     */
    void clearData();

    /**
     * @brief Update the histogram display
     */
    void updateHistogram();
    void updateDisplayWithCounts(QString& displayText);
    void updateDisplayWithNames(QString& displayText);
    void updateHistogramWithCounts();
    void updateHistogramWithNames();
    
    /**
     * @brief Get current histogram data for export
     * @return QString containing formatted data for export
     */
    QString getCurrentDataForExport() const;
    
    void createVisualHistogram();
    void updateVisualHistogram();
    void updateVisualHistogramWithCounts();
    void updateVisualHistogramWithNames();
    void clearVisualHistogram();
    
    void scheduleUpdate();
    void performUpdate();

    /**
     * @brief Handle file changes for real-time monitoring
     * @param path Path to the changed file
     */
    void onFileChanged(const QString& path);

    void setPermutationsPerBatch(int totalPermutations) { permutationsPerBatch_ = totalPermutations; }

signals:
    /**
     * @brief Emitted when the window is closed
     */
    void windowClosed();

protected:
    /**
     * @brief Handle window close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent* event) override;

private slots:
    /**
     * @brief Handle auto-refresh timer
     */
    void onAutoRefreshTimer();

    /**
     * @brief Handle manual refresh button click
     */
    void onRefreshClicked();

    /**
     * @brief Handle auto-refresh checkbox change
     * @param checked Whether auto-refresh is enabled
     */
    void onAutoRefreshChanged(bool checked);

    /**
     * @brief Handle bin count change
     * @param value New bin count
     */
    void onBinCountChanged(int value);

    /**
     * @brief Handle show statistics checkbox change
     * @param checked Whether to show statistics
     */
    void onShowStatisticsChanged(bool checked);

    /**
     * @brief Handle show connector names checkbox change
     * @param checked Whether to show connector names instead of counts
     */
    void onShowConnectorNamesChanged(bool checked);

    /**
     * @brief Handle show-after-pruning checkbox change
     * @param checked If true, display post-pruning connector counts; otherwise pre-pruning
     */
    void onShowAfterPruningChanged(bool checked);

    /**
     * @brief Handle download button click
     */
    void onDownloadClicked();

private:
    /**
     * @brief Setup the user interface
     */
    void setupUI();

    /**
     * @brief Setup the chart and histogram
     */
    void setupChart();

    /**
     * @brief Parse permutation results from file
     */
    void parseResultsFile();

    /**
     * @brief Calculate histogram bins
     * @param data Vector of connector counts
     * @param numBins Number of bins to create
     * @return Vector of bin counts
     */
    std::vector<int> calculateHistogramBins(const std::vector<int>& data, int numBins);

    /**
     * @brief Update statistics display
     */
    void updateStatistics();

    /**
     * @brief Get the currently active connector-count vector based on the show-after-pruning flag.
     */
    const std::vector<int>& activeConnectorData() const;

    /**
     * @brief Get the currently active permutation data vector based on the show-after-pruning flag.
     */
    const std::vector<std::pair<int, int>>& activePermutationData() const;

    QWidget* centralWidget_;
    QVBoxLayout* mainLayout_;
    
    QGroupBox* controlGroup_;
    QHBoxLayout* controlLayout_;
    QPushButton* refreshButton_;
    QPushButton* downloadButton_;
    QPushButton* resetButton_;
    QCheckBox* autoRefreshCheckBox_;
    QSpinBox* binCountSpinBox_;
    QCheckBox* showStatisticsCheckBox_;
    QCheckBox* showConnectorNamesCheckBox_;
    QCheckBox* showAfterPruningCheckBox_;
    QLabel* binCountLabel_;
    
    QGroupBox* statisticsGroup_;
    QVBoxLayout* statisticsLayout_;
    QLabel* totalPermutationsLabel_;
    QLabel* meanLabel_;
    QLabel* medianLabel_;
    QLabel* stdDevLabel_;
    QLabel* minLabel_;
    QLabel* maxLabel_;
    QLabel* rangeLabel_;
    
    QGroupBox* dataGroup_;
    QVBoxLayout* dataLayout_;
    QTableWidget* dataTable_;
    
    QGroupBox* dataDisplayGroup_;
    QVBoxLayout* dataDisplayLayout_;
    QTextEdit* dataDisplay_;
    
    QGroupBox* histogramGroup_;
    QVBoxLayout* histogramLayout_;
    VerticalHistogramWidget* histogramWidget_;
    
    QTimer* autoRefreshTimer_;
    QFileSystemWatcher* fileWatcher_;
    
    QTimer* updateThrottleTimer_;
    bool updatePending_;
    
    std::vector<int> connectorData_;
    std::vector<int> preConnectorData_;
    std::vector<std::pair<int, int>> permutationData_;
    std::vector<std::pair<int, int>> prePermutationData_;
    std::vector<std::pair<int, std::vector<std::string>>> connectorNamesData_;
    std::vector<std::pair<int, std::vector<std::string>>> seedNamesData_;
    
    int permutationsPerBatch_ = 0;
    std::vector<int> batchIndexData_;
    std::map<int, std::vector<std::string>> batchSeeds_;

    bool autoRefreshEnabled_;
    int binCount_;
    bool showStatistics_;
    bool showConnectorNames_;
    bool showAfterPruning_;
    int refreshInterval_;
    
    qint64 lastProcessedSize_;
    bool skipExistingOnNextParse_;
};

#endif