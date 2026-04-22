#include "WorkerThread.h"
#include <QDebug>
#include <QDateTime>
#include <fstream>
#include <future>
#include <iostream>
#include <chrono>
#include "ThreadPool.h"

WorkerThread::WorkerThread(QObject* parent) : QThread(parent), isRunning_(false), stopRequested_(false), callbackActive_(false), currentBatch_(0) {
    finder_.setProgressCallback([this](int current, int total, const std::string& status) {
        onProgress(current, total, status);
    });
    
    finder_.setDetailedProgressCallback([this](int permutation, int totalPermutations, 
                                              const std::string& stage, int stageProgress, int stageTotal) {
        onDetailedProgress(permutation, totalPermutations, stage, stageProgress, stageTotal);
    });
    
    finder_.setPermutationCompletedCallback([this](int permutation, int totalPermutations, int connectorsFound, const std::vector<int>& connectorIds, int preConnectorsFound) {
        if (!callbackActive_) {
            return;
        }
        
        std::vector<std::string> connectorNames;
        for (int connectorId : connectorIds) {
            connectorNames.push_back(graph_.getNodeName(connectorId));
        }
        
        std::vector<std::string> currentSeedNames;
        if (isBatchMode_ && currentBatch_ < batchSeedNodes_.size()) {
            currentSeedNames = batchSeedNodes_[currentBatch_];
        } else {
            currentSeedNames = seedNodes_;
        }
        
        onPermutationCompletedWithSeeds(permutation, totalPermutations, connectorsFound, connectorNames, currentSeedNames, preConnectorsFound);
    });
    
    finder_.setStopRequestCallback([this]() {
        return isStopRequested();
    });
}

WorkerThread::~WorkerThread() {
    if (isRunning()) {
        quit();
        wait(5000);
    }
    
    batchResults_.clear();
    batchConnectorNames_.clear();
}

void WorkerThread::setParameters(const Graph& graph, 
                                const std::vector<std::string>& seedNodes, 
                                const AnalysisConfig& config) {
    graph_ = graph;
    seedNodes_ = seedNodes;
    config_ = config;
    isBatchMode_ = false;
}

void WorkerThread::setBatchParameters(const Graph& graph,
                                     const std::vector<std::vector<std::string>>& batchSeedNodes,
                                     const AnalysisConfig& config) {
    graph_ = graph;
    batchSeedNodes_ = batchSeedNodes;
    config_ = config;
    isBatchMode_ = true;
}

void WorkerThread::run() {
    isRunning_ = true;
    stopRequested_ = false;
    
    try {
        finder_.getGraph() = graph_;

        if (isBatchMode_) {
            auto batchStartTime = std::chrono::high_resolution_clock::now();
            
            const int totalBatches = batchSeedNodes_.size();

            emit progressUpdate(0, totalBatches, QString("Phase 1/2: Calculating all distance maps (%1 batches)...").arg(totalBatches));
            
            callbackActive_ = false;
            
            std::vector<std::vector<int>> batchSeedIds(totalBatches);
            for(int i = 0; i < totalBatches; ++i) {
                for (const std::string& seedName : batchSeedNodes_[i]) {
                    int id = graph_.getNodeId(seedName);
                    if (id != -1) {
                        batchSeedIds[i].push_back(id);
                    } else {
                        emit errorOccurred(QString("Fatal Error: Seed '%1' not found.").arg(QString::fromStdString(seedName)));
                        return;
                    }
                }
            }

            ThreadPool path_calculation_pool(config_.maxThreads);
            std::vector<std::future<PathManager>> pathManagerFutures;
            

            for (const auto& seedIdsForBatch : batchSeedIds) {
                if (seedIdsForBatch.size() < 2) {
                    std::promise<PathManager> p;
                    p.set_value(PathManager(graph_, {}));
                    pathManagerFutures.push_back(p.get_future());
                    continue;
                }
                pathManagerFutures.emplace_back(
                    path_calculation_pool.enqueue([this, seedIdsForBatch] {
                        return PathManager(this->graph_, seedIdsForBatch);
                    })
                );
            }

            std::vector<PathManager> calculatedPathManagers;
            calculatedPathManagers.reserve(totalBatches);
            for (size_t i = 0; i < pathManagerFutures.size(); ++i) {
                if (stopRequested_) {
                    emit errorOccurred("Analysis stopped by user");
                    return;
                }
                
                calculatedPathManagers.push_back(pathManagerFutures[i].get());
                emit progressUpdate(i + 1, totalBatches, QString("Phase 1/2: Calculated distance map for batch %1...").arg(i + 1));
            }

            emit progressUpdate(0, totalBatches, QString("Phase 2/2: Running permutation analysis..."));
            
            std::vector<AnalysisResult> allBatchResults;
            std::vector<AnalysisResult> allBatchResultsWithoutPruning;
            std::vector<AnalysisResult> allBatchResultsWithPruning;
            
            AnalysisConfig configWithoutPruning = config_;
            configWithoutPruning.enablePruning = false;
            
            AnalysisConfig configWithPruning = config_;
            configWithPruning.enablePruning = true;
            
            bool useInterBatchParallel = (config_.enableParallelExecution &&
                                          config_.maxThreads > 1 &&
                                          totalBatches > 1);
            
            if (useInterBatchParallel) {
                int numThreads = std::min(config_.maxThreads, totalBatches);
                
                callbackActive_ = false;
                
                allBatchResults.resize(totalBatches);
                allBatchResultsWithoutPruning.resize(totalBatches);
                allBatchResultsWithPruning.resize(totalBatches);
                
                std::atomic<int> completedBatches{0};
                
                emit progressUpdate(0, totalBatches, QString("Phase 2a: Running %1 permutations (parallel)...").arg(totalBatches));
                {
                    ThreadPool batch_pool(numThreads);
                    std::vector<std::future<void>> batchFutures;
                    
                    for (int i = 0; i < totalBatches; ++i) {
                        if (batchSeedIds[i].size() < 2) {
                            AnalysisResult invalidResult;
                            invalidResult.success = false;
                            invalidResult.errorMessage = "Batch has less than 2 valid seed nodes";
                            allBatchResults[i] = invalidResult;
                            allBatchResultsWithoutPruning[i] = invalidResult;
                            allBatchResultsWithPruning[i] = invalidResult;
                            completedBatches.fetch_add(1);
                            continue;
                        }
                        
                        batchFutures.emplace_back(
                            batch_pool.enqueue([this, i, totalBatches,
                                                &allBatchResultsWithoutPruning,
                                                &batchSeedIds,
                                                &calculatedPathManagers,
                                                &configWithoutPruning,
                                                &completedBatches]()
                            {
                                if (stopRequested_) {
                                    return;
                                }
                                
                                BasicNetworkFinder localFinder;
                                localFinder.setStopRequestCallback([this]() {
                                    return isStopRequested();
                                });
                                
                                AnalysisConfig localNoPrune = configWithoutPruning;
                                localNoPrune.enableParallelExecution = false;
                                
                                allBatchResultsWithoutPruning[i] =
                                    localFinder.runAnalysisWithPrecalculatedPaths(
                                        graph_, batchSeedIds[i], localNoPrune,
                                        calculatedPathManagers[i]);
                                
                                int completed = completedBatches.fetch_add(1) + 1;
                                emit progressUpdate(completed, totalBatches,
                                    QString("Phase 2a: %1/%2 permutations done")
                                        .arg(completed).arg(totalBatches));
                            })
                        );
                    }
                    
                    for (auto& f : batchFutures) {
                        f.get();
                    }
                }
                
                if (stopRequested_) {
                    auto partialBatchResult = std::make_shared<BatchAnalysisResult>();
                    partialBatchResult->allBatchResults = allBatchResultsWithoutPruning;
                    partialBatchResult->allBatchResultsWithoutPruning = allBatchResultsWithoutPruning;
                    try { partialBatchResult->calculateStatistics(); } catch (...) {}
                    emit batchAnalysisFinished(partialBatchResult);
                    emit errorOccurred("Analysis stopped by user");
                    return;
                }
                
                if (config_.enablePruning) {
                    emit progressUpdate(0, totalBatches, QString("Phase 2b: Running %1 prunings (parallel)...").arg(totalBatches));
                    
                    std::atomic<int> completedPrunings{0};
                    {
                        ThreadPool prune_pool(numThreads);
                        std::vector<std::future<void>> pruneFutures;
                        
                        for (int i = 0; i < totalBatches; ++i) {
                            if (!allBatchResultsWithoutPruning[i].success) {
                                allBatchResultsWithPruning[i] = allBatchResultsWithoutPruning[i];
                                allBatchResults[i] = allBatchResultsWithoutPruning[i];
                                completedPrunings.fetch_add(1);
                                continue;
                            }
                            
                            pruneFutures.emplace_back(
                                prune_pool.enqueue([this, i, totalBatches,
                                                    &allBatchResults,
                                                    &allBatchResultsWithoutPruning,
                                                    &allBatchResultsWithPruning,
                                                    &batchSeedIds,
                                                    &calculatedPathManagers,
                                                    &configWithPruning,
                                                    &completedPrunings]()
                                {
                                    if (stopRequested_) {
                                        return;
                                    }
                                    
                                    BasicNetworkFinder localFinder;
                                    localFinder.setStopRequestCallback([this]() {
                                        return isStopRequested();
                                    });
                                    
                                    AnalysisConfig localPrune = configWithPruning;
                                    localPrune.enableParallelExecution = false;
                                    
                                    allBatchResultsWithPruning[i] =
                                        localFinder.runAnalysisWithPrecalculatedPaths(
                                            graph_, batchSeedIds[i], localPrune,
                                            calculatedPathManagers[i]);
                                    allBatchResults[i] = allBatchResultsWithPruning[i];
                                    
                                    const auto& result = allBatchResults[i];
                                    if (result.success && result.basicNetworkResult.isValid) {
                                        int connectorCount = result.basicNetworkResult.connectorNodes.size();
                                        std::vector<std::string> connectorNames;
                                        for (int connId : result.basicNetworkResult.connectorNodes) {
                                            connectorNames.push_back(graph_.getNodeName(connId));
                                        }
                                        emit permutationCompletedWithSeeds(
                                            i + 1, totalBatches, connectorCount,
                                            connectorNames, batchSeedNodes_[i]);
                                    }
                                    
                                    int completed = completedPrunings.fetch_add(1) + 1;
                                    emit progressUpdate(completed, totalBatches,
                                        QString("Phase 2b: %1/%2 prunings done")
                                            .arg(completed).arg(totalBatches));
                                })
                            );
                        }
                        
                        for (auto& f : pruneFutures) {
                            f.get();
                        }
                    }
                } else {
                    for (int i = 0; i < totalBatches; ++i) {
                        allBatchResults[i] = allBatchResultsWithoutPruning[i];
                    }
                }
                
                if (stopRequested_) {
                    auto partialBatchResult = std::make_shared<BatchAnalysisResult>();
                    partialBatchResult->allBatchResults = allBatchResults;
                    partialBatchResult->allBatchResultsWithoutPruning = allBatchResultsWithoutPruning;
                    partialBatchResult->allBatchResultsWithPruning = allBatchResultsWithPruning;
                    try { partialBatchResult->calculateStatistics(); } catch (...) {}
                    emit batchAnalysisFinished(partialBatchResult);
                    emit errorOccurred("Analysis stopped by user");
                    return;
                }
                
            } else {
                callbackActive_ = true;
                
                for (int i = 0; i < totalBatches; ++i) {
                    if (stopRequested_) {
                        
                        if (!allBatchResults.empty()) {
                            try {
                                auto partialBatchResult = std::make_shared<BatchAnalysisResult>();
                                partialBatchResult->allBatchResults = allBatchResults;
                                partialBatchResult->allBatchResultsWithoutPruning = allBatchResultsWithoutPruning;
                                partialBatchResult->allBatchResultsWithPruning = allBatchResultsWithPruning;
                                try { partialBatchResult->calculateStatistics(); } catch (...) {}
                                emit batchAnalysisFinished(partialBatchResult);
                            } catch (...) {}
                        }
                        
                        callbackActive_ = false;
                        emit errorOccurred("Analysis stopped by user");
                        return;
                    }
                    
                    setCurrentBatch(i);
                    emit batchChanged(i + 1, totalBatches);
                    emit progressUpdate(i, totalBatches, QString("Phase 2/2: Analyzing permutations for Batch %1...").arg(i + 1));
                    
                    if (batchSeedIds[i].size() < 2) {
                        AnalysisResult invalidResult;
                        invalidResult.success = false;
                        invalidResult.errorMessage = "Batch has less than 2 valid seed nodes";
                        allBatchResults.push_back(invalidResult);
                        allBatchResultsWithoutPruning.push_back(invalidResult);
                        allBatchResultsWithPruning.push_back(invalidResult);
                        continue;
                    }
                    
                    callbackActive_ = false;
                    AnalysisResult batchResultWithoutPruning = finder_.runAnalysisWithPrecalculatedPaths(
                        graph_, batchSeedIds[i], configWithoutPruning, calculatedPathManagers[i]);
                    allBatchResultsWithoutPruning.push_back(batchResultWithoutPruning);

                    if (config_.enablePruning) {
                        callbackActive_ = true;
                        AnalysisResult batchResultWithPruning = finder_.runAnalysisWithPrecalculatedPaths(
                            graph_, batchSeedIds[i], configWithPruning, calculatedPathManagers[i]);
                        allBatchResultsWithPruning.push_back(batchResultWithPruning);
                        allBatchResults.push_back(batchResultWithPruning);
                    } else {
                        allBatchResults.push_back(batchResultWithoutPruning);
                    }
                }
            }
            
            try {
                emit progressUpdate(totalBatches, totalBatches, QString("Finalizing results..."));
                
                auto finalBatchResult = std::make_shared<BatchAnalysisResult>();
                finalBatchResult->allBatchResults = std::move(allBatchResults);
                finalBatchResult->allBatchResultsWithoutPruning = std::move(allBatchResultsWithoutPruning);
                finalBatchResult->allBatchResultsWithPruning = std::move(allBatchResultsWithPruning);
                
                finalBatchResult->calculateBasicStatistics();
                
                auto batchEndTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(batchEndTime - batchStartTime);
                finalBatchResult->wallClockTimeSeconds = duration.count() / 1000.0;
                
                callbackActive_ = false;
                
                emit batchAnalysisFinished(finalBatchResult);
                
            } catch (const std::exception& e) {
                emit errorOccurred(QString("Critical error in batch analysis: %1").arg(e.what()));
            } catch (...) {
                emit errorOccurred("Critical unknown error in batch analysis");
            }

        } else {
            std::vector<int> seedIds;
            for (const std::string& seedName : seedNodes_) {
                int id = graph_.getNodeId(seedName);
                if (id != -1) {
                    seedIds.push_back(id);
                } else {
                    emit errorOccurred(QString("Seed node '%1' not found in graph").arg(QString::fromStdString(seedName)));
                    return;
                }
            }
            
            if (seedIds.empty()) {
                emit errorOccurred("No valid seed nodes found");
                return;
            }
            
            callbackActive_ = true;
            
            if (stopRequested_) {
                callbackActive_ = false;
                emit errorOccurred("Analysis stopped by user");
                return;
            }
            
            AnalysisResult result = finder_.findBasicNetwork(graph_, seedIds, config_);
            
            if (stopRequested_) {
                if (result.success || !result.allPermutationResults.empty()) {
                    emit analysisFinished(result);
                }
                emit errorOccurred("Analysis stopped by user");
                return;
            }

            emit analysisFinished(result);
            
            callbackActive_ = false;
        }
        
    } catch (const std::exception& e) {
        emit errorOccurred(QString("An exception occurred during analysis: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("An unknown exception occurred during analysis");
    }
    
    isRunning_ = false;
    
    try {
        batchResults_.clear();
        batchConnectorNames_.clear();
    } catch (...) {
    }
}

void WorkerThread::onProgress(int current, int total, const std::string& status) {
    emit progressUpdate(current, total, QString::fromStdString(status));
}

void WorkerThread::onDetailedProgress(int permutation, int totalPermutations, 
                                     const std::string& stage, int stageProgress, int stageTotal) {
    emit detailedProgressUpdate(permutation, totalPermutations, 
                               QString::fromStdString(stage), stageProgress, stageTotal);
}

void WorkerThread::onPermutationCompleted(int permutation, int totalPermutations, int connectorsFound) {
    std::lock_guard<std::mutex> lock(threadMutex_);
    batchResults_[currentBatch_].push_back({permutation, connectorsFound});
    
    std::ofstream permFile("real_time_permutations.txt", std::ios::trunc);
    if (permFile.is_open()) {
        permFile << "=== REAL-TIME PERMUTATION RESULTS ===" << std::endl;
        permFile << "Timestamp: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString() << std::endl;
        permFile << std::endl;
        
        for (const auto& batch : batchResults_) {
            permFile << "--- BATCH " << (batch.first + 1) << " ---" << std::endl;
            
            std::vector<std::pair<int, int>> sortedPerms = batch.second;
            std::sort(sortedPerms.begin(), sortedPerms.end());
            
            for (const auto& perm : sortedPerms) {
                permFile << "  Permutation " << perm.first << "/" << totalPermutations << ": " << perm.second << " connectors" << std::endl;
            }
            permFile << std::endl;
        }
        permFile.close();
    }
    
    emit permutationCompleted(permutation, totalPermutations, connectorsFound);
}

void WorkerThread::onPermutationCompletedWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames) {
    std::vector<std::string> currentSeedNames;
    if (isBatchMode_ && currentBatch_ < batchSeedNodes_.size()) {
        currentSeedNames = batchSeedNodes_[currentBatch_];
    } else {
        currentSeedNames = seedNodes_;
    }
    
    onPermutationCompletedWithSeeds(permutation, totalPermutations, connectorsFound, connectorNames, currentSeedNames);
}

void WorkerThread::onPermutationCompletedWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames, int preConnectorsFound) {
    std::lock_guard<std::mutex> lock(threadMutex_);
    batchResults_[currentBatch_].push_back({permutation, connectorsFound});
    batchConnectorNames_[currentBatch_].push_back({permutation, connectorNames});
    batchSeedNames_[currentBatch_].push_back({permutation, seedNames});
    
    std::ofstream permFile("real_time_permutations.txt", std::ios::trunc);
    if (permFile.is_open()) {
        permFile << "=== REAL-TIME PERMUTATION RESULTS ===" << std::endl;
        permFile << "Timestamp: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString() << std::endl;
        permFile << std::endl;
        
        for (const auto& batch : batchResults_) {
            permFile << "--- BATCH " << (batch.first + 1) << " ---" << std::endl;
            
            std::vector<std::pair<int, int>> sortedPerms = batch.second;
            std::sort(sortedPerms.begin(), sortedPerms.end());
            
            for (const auto& perm : sortedPerms) {
                permFile << "  Permutation " << perm.first << "/" << totalPermutations << ": " << perm.second << " connectors";
                
                auto namesIt = batchConnectorNames_.find(batch.first);
                if (namesIt != batchConnectorNames_.end()) {
                    for (const auto& nameData : namesIt->second) {
                        if (nameData.first == perm.first) {
                            permFile << " [Connectors: ";
                            for (size_t i = 0; i < nameData.second.size(); ++i) {
                                if (i > 0) permFile << ", ";
                                permFile << nameData.second[i];
                            }
                            permFile << "]";
                            break;
                        }
                    }
                }
                
                auto seedsIt = batchSeedNames_.find(batch.first);
                if (seedsIt != batchSeedNames_.end()) {
                    for (const auto& seedData : seedsIt->second) {
                        if (seedData.first == perm.first) {
                            permFile << " [Seeds: ";
                            for (size_t i = 0; i < seedData.second.size(); ++i) {
                                if (i > 0) permFile << ", ";
                                permFile << seedData.second[i];
                            }
                            permFile << "]";
                            break;
                        }
                    }
                }
                permFile << std::endl;
            }
            permFile << std::endl;
        }
        permFile.close();
    }
    
    emit permutationCompletedWithSeeds(permutation, totalPermutations, connectorsFound, connectorNames, seedNames, preConnectorsFound);
}

void WorkerThread::setCurrentBatch(int batchIndex) {
    std::lock_guard<std::mutex> lock(threadMutex_);
    currentBatch_ = batchIndex;
}

void WorkerThread::resetBatchTracking() {
    std::lock_guard<std::mutex> lock(threadMutex_);
    currentBatch_ = 0;
    batchResults_.clear();
    batchConnectorNames_.clear();
    batchSeedNames_.clear();
}

void WorkerThread::requestStop() {
    stopRequested_ = true;
}

bool WorkerThread::isStopRequested() const {
    return stopRequested_;
} 