#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <QThread>
#include <QString>
#include <mutex>
#include <atomic>
#include <memory>
#include "BasicNetworkFinder.h"
#include "PathManager.h"

class WorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit WorkerThread(QObject *parent = nullptr);
    ~WorkerThread();
    
    void setParameters(const Graph& graph, 
                      const std::vector<std::string>& seedNodes, 
                      const AnalysisConfig& config);
    
    void setBatchParameters(const Graph& graph,
                           const std::vector<std::vector<std::string>>& batchSeedNodes,
                           const AnalysisConfig& config);

    /**
     * @brief Request the thread to stop execution
     */
    void requestStop();

    /**
     * @brief Check if stop has been requested
     * @return True if stop has been requested
     */
    bool isStopRequested() const;

signals:
    void progressUpdate(int current, int total, const QString& status);
    void detailedProgressUpdate(int permutation, int totalPermutations, 
                               const QString& stage, int stageProgress, int stageTotal);
    void permutationCompleted(int permutation, int totalPermutations, int connectorsFound);
    void permutationCompletedWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames);
    void permutationCompletedWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames, int preConnectorsFound = -1);
    void analysisFinished(const AnalysisResult& result);
    void batchAnalysisFinished(std::shared_ptr<BatchAnalysisResult> result);
    void errorOccurred(const QString& error);
    
    void batchChanged(int currentBatch, int totalBatches);

protected:
    void run() override;

private slots:
    void onProgress(int current, int total, const std::string& status);
    void onDetailedProgress(int permutation, int totalPermutations, 
                           const std::string& stage, int stageProgress, int stageTotal);
    void onPermutationCompleted(int permutation, int totalPermutations, int connectorsFound);
    void onPermutationCompletedWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames);
    void onPermutationCompletedWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames, int preConnectorsFound = -1);

private:
    BasicNetworkFinder finder_;
    Graph graph_;
    std::vector<std::string> seedNodes_;
    std::vector<std::vector<std::string>> batchSeedNodes_;
    AnalysisConfig config_;
    bool isBatchMode_;
    
    std::atomic<bool> stopRequested_;
    std::atomic<bool> callbackActive_;
    
    void setCurrentBatch(int batchIndex);
    void resetBatchTracking();
    
    std::map<int, std::vector<std::pair<int, int>>> batchResults_;
    std::map<int, std::vector<std::pair<int, std::vector<std::string>>>> batchConnectorNames_;
    std::map<int, std::vector<std::pair<int, std::vector<std::string>>>> batchSeedNames_;
    int currentBatch_;
    
    std::mutex threadMutex_;
    std::atomic<bool> isRunning_;
};

#endif