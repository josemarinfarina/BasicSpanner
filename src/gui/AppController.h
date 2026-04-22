#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <vector>
#include <string>
#include <memory>
#include "BasicNetworkFinder.h"
#include "WorkerThread.h"
#include "ColumnSelectionDialog.h"

struct GraphFilterConfig;

/**
 * @brief Application controller that owns all business logic
 * 
 * The AppController class serves as the application's "brain" following the MVC pattern.
 * It owns the core BasicNetworkFinder model and WorkerThread, handles all application
 * logic, and communicates with UI panels through signals and slots.
 * 
 */
class AppController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent Parent QObject
     */
    explicit AppController(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~AppController();

    /**
     * @brief Get access to the BasicNetworkFinder for node operations
     * 
     * @return Reference to the BasicNetworkFinder instance
     */
    const BasicNetworkFinder& getFinder() const;
    
    /**
     * @brief Get the current loaded graph
     * 
     * @return Reference to the current Graph
     */
    const Graph& getCurrentGraph() const;

public slots:
    void onLoadNetworkRequested();
    void onLoadSeedsRequested();
    
    void onLoadNetworkFileDirectly(const QString& filePath);
    void onLoadSeedsFileDirectly(const QString& filePath);
    /**
     * @brief Handle run analysis request
     * @param config Configuration for the analysis
     * @param seedNodes Seed nodes for the analysis
     */
    void onRunAnalysisRequested(const AnalysisConfig& config, const std::vector<std::string>& seedNodes);
    void onSaveResultsRequested(const QString& resultsText);
    void onExportNetworkRequested();
    void onExportOriginalGraphRequested();
    void onExportStatisticsRequested();
    void onSelectAllNodesRequested();
    void onClearSeedsRequested();
    void onNodeClicked(const std::string& nodeName);
    void onGenerateRandomSeedsRequested(int amount);
    void onBatchesRequested(bool enabled, int amount, int seedsPerBatch);
    void onStopAnalysisRequested();
    void onApplyGraphFilter(const GraphFilterConfig& config);
    void onResetGraphFilter();

private slots:
    void onAnalysisFinished(const AnalysisResult& result);
    void onBatchAnalysisFinished(std::shared_ptr<BatchAnalysisResult> result);
    void onAnalysisProgress(int current, int total, const QString& status);
    void onDetailedAnalysisProgress(int permutation, int totalPermutations, 
                                   const QString& stage, int stageProgress, int stageTotal);
    void onPermutationCompleted(int permutation, int totalPermutations, int connectorsFound);
    void onPermutationCompletedWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames);
    void onPermutationCompletedWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames, int preConnectorsFound = -1);
    void onAnalysisError(const QString& error);

signals:
    void networkLoaded(const QString& fileName, const QString& filePath, int nodeCount, int edgeCount,
                      int rejectedSelfLoops = 0, int isolatedNodes = 0,
                      int initialConnections = 0, int nonUniqueInteractions = 0);
    void graphReadyForDisplay(const Graph& graph);
    void seedsLoaded(const QString& seedContent);
    void seedsLoadedFromFile(const QString& seedContent, const QString& filePath);
    void analysisStarted();
    void analysisStartedWithPermutations(int totalPermutations);
    void analysisFinished(bool success);
    void resultsReady(const QString& formattedResults);
    void basicNetworkReady(const BasicNetworkResult& result);
    void connectorFrequencyReady(const std::map<int, int>& globalFrequency, int totalBatches);
    void perBatchConnectorsReady(const std::vector<std::set<int>>& perBatchConnectors,
                                 const std::vector<std::map<int, int>>& perBatchFrequency,
                                 int numBatches);
    void allNodeNamesReady(const std::vector<std::string>& nodeNames);
    void nodeListReady(const std::vector<std::string>& nodeNames);
    void showInfoMessage(const QString& message);
    void showErrorMessage(const QString& message);
    void graphFiltered(const QString& statusMsg, int nodeCount, int edgeCount);

    void progressUpdate(int current, int total, const QString& status);
    void detailedProgressUpdate(int permutation, int totalPermutations, 
                               const QString& stage, int stageProgress, int stageTotal);
    void permutationCompleted(int permutation, int totalPermutations, int connectorsFound);
    void permutationCompletedWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames);
    void permutationCompletedWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames, int preConnectorsFound = -1);
    
    void batchChanged(int currentBatch, int totalBatches);

private:
    /**
     * @brief Format analysis results as human-readable string
     * @param result Analysis result to format
     * @return Formatted string representation
     */
    QString formatResults(const AnalysisResult& result);
    
    /**
     * @brief Format batch analysis results as human-readable string
     * @param result Batch analysis result to format
     * @return Formatted string representation
     */
    QString formatBatchResults(const BatchAnalysisResult& result);
    
    /**
     * @brief Export network as CSV format
     * 
     * @param result Analysis result containing the network to export
     * @param filename Output file path
     * @return true if export was successful, false otherwise
     */
    bool exportNetworkAsCSV(const AnalysisResult& result, const QString& filename);

    BasicNetworkFinder finder_;
    WorkerThread* workerThread_;
    AnalysisResult lastResult_;
    std::shared_ptr<BatchAnalysisResult> lastBatchResult_;
    AnalysisConfig lastConfig_;
    bool hasValidGraph_;
    QString currentGraphFile_;
    
    Graph originalGraph_;
    bool hasOriginalGraph_ = false;
    
public:
    const BasicNetworkResult* getBatchResult(int batchIndex) const;
    int getBatchCount() const;
    const BasicNetworkResult* getPermutationResult(int batchIndex, int permutationIndex) const;
    int getPermutationCount(int batchIndex) const;
    const AnalysisResult& getLastResult() const { return lastResult_; }
};

#endif