#include "AppController.h"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>
#include <QFileDialog>
#include <QDateTime>
#include <algorithm>
#include <numeric>
#include <random>
#include "../core/GraphAlgorithms.h"
#include "panels/GraphFilterPanel.h"
#include <set>
#include <limits>
#include <QRegularExpression>
#include <iostream>
#include <chrono>

AppController::AppController(QObject *parent) 
    : QObject(parent), workerThread_(nullptr), hasValidGraph_(false)
{
}

AppController::~AppController()
{
    
    if (workerThread_) {
        workerThread_->quit();
        if (!workerThread_->wait(10000)) {
            workerThread_->terminate();
            workerThread_->wait(5000);
        }
        delete workerThread_;
        workerThread_ = nullptr;
    }
    
}

const BasicNetworkFinder& AppController::getFinder() const
{
    return finder_;
}

const Graph& AppController::getCurrentGraph() const
{
    return finder_.getGraph();
}

void AppController::onLoadNetworkRequested()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr,
        "Load Network File", "", "Text Files (*.txt);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    try {
        ColumnSelectionDialog dialog(fileName);
        if (dialog.exec() == QDialog::Accepted) {
            ColumnSelectionResult config = dialog.getResult();
            
            bool success = finder_.loadGraphWithColumns(
                fileName.toStdString(),
                config.sourceColumn,
                config.targetColumn,
                config.separator.toStdString(),
                config.hasHeader,
                config.skipLines,
                config.nodeNameColumn,
                config.applyNamesToTarget,
                config.caseSensitive
            );
            
            if (success) {
                hasValidGraph_ = true;
                currentGraphFile_ = fileName;
                lastResult_ = AnalysisResult();
                
                const Graph& graph = finder_.getGraph();
                int rejectedLoops = graph.getRejectedSelfLoopsCount();
                int isolatedNodes = graph.getIsolatedNodesCount();
                int initialConns = graph.getInitialConnectionsCount();
                int nonUniqueInteracts = graph.getNonUniqueInteractionsCount();
                
                QFileInfo fileInfo(fileName);
                emit networkLoaded(fileInfo.fileName(), 
                                 fileName,
                                 graph.getNodeCount(), 
                                 graph.getEdgeCount(),
                                 rejectedLoops,
                                 isolatedNodes,
                                 initialConns,
                                 nonUniqueInteracts);
                emit graphReadyForDisplay(graph);
                
                auto nodes = graph.getNodes();
                std::vector<std::string> nodeNames;
                nodeNames.reserve(nodes.size());
                for (int nodeId : nodes) {
                    nodeNames.push_back(graph.getNodeName(nodeId));
                }
                emit nodeListReady(nodeNames);
                
                QString message = QString("Network loaded successfully: %1 nodes, %2 edges")
                                   .arg(graph.getNodeCount())
                                   .arg(graph.getEdgeCount());
                if (rejectedLoops > 0) {
                    message += QString("\nRejected %1 self-loops").arg(rejectedLoops);
                    if (isolatedNodes > 0) {
                        message += QString(" (%1 isolated nodes)").arg(isolatedNodes);
                    }
                }
                emit showInfoMessage(message);
            } else {
                emit showErrorMessage("Failed to load network file");
            }
        }
    } catch (const std::exception& e) {
        emit showErrorMessage(QString("Error loading network: %1").arg(e.what()));
    }
}

void AppController::onLoadNetworkFileDirectly(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }
    
    try {
        ColumnSelectionDialog dialog(filePath);
        if (dialog.exec() == QDialog::Accepted) {
            ColumnSelectionResult config = dialog.getResult();
            
            bool success = finder_.loadGraphWithColumns(
                filePath.toStdString(),
                config.sourceColumn,
                config.targetColumn,
                config.separator.toStdString(),
                config.hasHeader,
                config.skipLines,
                config.nodeNameColumn,
                config.applyNamesToTarget,
                config.caseSensitive
            );
            
            if (success) {
                hasValidGraph_ = true;
                currentGraphFile_ = filePath;
                lastResult_ = AnalysisResult();
                
                const Graph& graph = finder_.getGraph();
                int rejectedLoops = graph.getRejectedSelfLoopsCount();
                int isolatedNodes = graph.getIsolatedNodesCount();
                int initialConns = graph.getInitialConnectionsCount();
                int nonUniqueInteracts = graph.getNonUniqueInteractionsCount();
                
                QFileInfo fileInfo(filePath);
                emit networkLoaded(fileInfo.fileName(), 
                                 filePath,
                                 graph.getNodeCount(), 
                                 graph.getEdgeCount(),
                                 rejectedLoops,
                                 isolatedNodes,
                                 initialConns,
                                 nonUniqueInteracts);
                emit graphReadyForDisplay(graph);
                
                auto nodes = graph.getNodes();
                std::vector<std::string> nodeNames;
                nodeNames.reserve(nodes.size());
                for (int nodeId : nodes) {
                    nodeNames.push_back(graph.getNodeName(nodeId));
                }
                emit nodeListReady(nodeNames);
                
                QString message = QString("Network loaded successfully: %1 nodes, %2 edges")
                                   .arg(graph.getNodeCount())
                                   .arg(graph.getEdgeCount());
                if (rejectedLoops > 0) {
                    message += QString("\nRejected %1 self-loops").arg(rejectedLoops);
                    if (isolatedNodes > 0) {
                        message += QString(" (%1 isolated nodes)").arg(isolatedNodes);
                    }
                }
                emit showInfoMessage(message);
            } else {
                emit showErrorMessage("Failed to load network file");
            }
        }
    } catch (const std::exception& e) {
        emit showErrorMessage(QString("Error loading network: %1").arg(e.what()));
    }
}

void AppController::onLoadSeedsFileDirectly(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }
    
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        emit seedsLoaded(content);
        emit seedsLoadedFromFile(content, filePath);
        emit showInfoMessage("Seeds loaded successfully");
    } else {
        emit showErrorMessage("Failed to open seeds file");
    }
}

void AppController::onLoadSeedsRequested()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr,
        "Load Seeds File", "", "Text Files (*.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            emit seedsLoaded(content);
            emit seedsLoadedFromFile(content, fileName);
            emit showInfoMessage("Seeds loaded successfully");
        } else {
            emit showErrorMessage("Failed to open seeds file");
        }
    }
}

void AppController::onRunAnalysisRequested(const AnalysisConfig& config, const std::vector<std::string>& seedNodes)
{
    if (!hasValidGraph_) {
        emit showErrorMessage("No network loaded");
        return;
    }
    
    if (seedNodes.size() < 2) {
        emit showErrorMessage("At least 2 seed nodes are required");
        return;
    }
    
    if (workerThread_) {
        workerThread_->quit();
        if (!workerThread_->wait(10000)) {
            workerThread_->terminate();
            workerThread_->wait(5000);
        }
        delete workerThread_;
        workerThread_ = nullptr;
    }
    
    workerThread_ = new WorkerThread(this);
    
    connect(workerThread_, &WorkerThread::analysisFinished, 
            this, &AppController::onAnalysisFinished);
    connect(workerThread_, &WorkerThread::batchAnalysisFinished, 
            this, &AppController::onBatchAnalysisFinished);
    connect(workerThread_, &WorkerThread::progressUpdate, 
            this, &AppController::onAnalysisProgress);
    connect(workerThread_, &WorkerThread::detailedProgressUpdate, 
            this, &AppController::onDetailedAnalysisProgress);
    connect(workerThread_, &WorkerThread::permutationCompleted, 
            this, &AppController::onPermutationCompleted);
    connect(workerThread_, &WorkerThread::permutationCompletedWithNames, 
            this, &AppController::onPermutationCompletedWithNames);
    connect(workerThread_, &WorkerThread::permutationCompletedWithSeeds, 
            this, &AppController::onPermutationCompletedWithSeeds);
    connect(workerThread_, &WorkerThread::batchChanged, this, [this](int currentBatch, int totalBatches){
        emit batchChanged(currentBatch, totalBatches);
    });
    connect(workerThread_, &WorkerThread::errorOccurred, 
            this, &AppController::onAnalysisError);
    
    std::vector<std::vector<std::string>> batchSeedNodes;
    
    for (const std::string& seed : seedNodes) {
        if (seed == "---BATCH_SEPARATOR---") {
            batchSeedNodes.push_back(std::vector<std::string>());
        } else {
            if (batchSeedNodes.empty()) {
                batchSeedNodes.push_back(std::vector<std::string>());
            }
            batchSeedNodes.back().push_back(seed);
        }
    }
    
    lastConfig_ = config;
    
    if (batchSeedNodes.size() > 1) {
        workerThread_->setBatchParameters(finder_.getGraph(), batchSeedNodes, config);
        emit showInfoMessage(QString("Starting analysis with %1 batches").arg(batchSeedNodes.size()));
    } else {
        workerThread_->setParameters(finder_.getGraph(), seedNodes, config);
    }
    
    emit analysisStarted();
    
    if (config.numPermutations > 1) {
        emit analysisStartedWithPermutations(config.numPermutations);
    }
    
    workerThread_->start();
}

void AppController::onSaveResultsRequested(const QString& resultsText)
{
    if (resultsText.isEmpty()) {
        emit showErrorMessage("No results to save");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(nullptr,
        "Save Results", "", "Text Files (*.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << resultsText;
            emit showInfoMessage("Results saved successfully");
        } else {
            emit showErrorMessage("Failed to save results");
        }
    }
}

void AppController::onExportNetworkRequested()
{
    if (!hasValidGraph_) {
        emit showErrorMessage("No network loaded");
        return;
    }
    
    bool hasSingleResult = lastResult_.success || !lastResult_.allPermutationResults.empty();
    bool hasBatchResult = lastBatchResult_ && lastBatchResult_->bestResult.success;
    
    if (!hasSingleResult && !hasBatchResult) {
        emit showErrorMessage("No analysis results available. Please run analysis first.");
        return;
    }
    
    QDialog dialog;
    dialog.setWindowTitle("Export Network");
    dialog.setModal(true);
    dialog.setMinimumWidth(380);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QLabel* resultLabel = new QLabel("Select result to export:");
    layout->addWidget(resultLabel);
    
    QComboBox* resultCombo = new QComboBox;
    if (hasBatchResult) {
        resultCombo->addItem("Best result (general)", -1);
        for (size_t i = 0; i < lastBatchResult_->allBatchResults.size(); ++i) {
            if (lastBatchResult_->allBatchResults[i].success) {
                int cc = static_cast<int>(lastBatchResult_->allBatchResults[i].basicNetworkResult.connectorNodes.size());
                resultCombo->addItem(QString("Batch %1 (%2 connectors)").arg(i + 1).arg(cc), static_cast<int>(i));
            }
        }
    } else {
        resultCombo->addItem("Analysis result", -1);
    }
    layout->addWidget(resultCombo);
    
    QFrame* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);
    
    QCheckBox* extraCheckBox = new QCheckBox("Include extra connectors (>0%)");
    extraCheckBox->setToolTip(
        "Includes connectors that appeared in other permutations\n"
        "with their edges from the original graph");
    layout->addWidget(extraCheckBox);
    
    QCheckBox* attrCheckBox = new QCheckBox("Export node attributes (for Gephi)");
    attrCheckBox->setToolTip(
        "Exports an additional nodes CSV file with columns:\n"
        "  Type (Seed, Connector, Extra connector)\n"
        "  Deg(G), Deg(L), Betw(G), Betw(L)\n"
        "Import in Gephi: File > Import spreadsheet > Nodes table");
    layout->addWidget(attrCheckBox);
    
    QFrame* line2 = new QFrame;
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line2);
    
    QLabel* formatLabel = new QLabel("Export format:");
    layout->addWidget(formatLabel);
    
    QRadioButton* csvButton = new QRadioButton("CSV format (*.csv)");
    csvButton->setChecked(true);
    layout->addWidget(csvButton);
    
    QRadioButton* txtButton = new QRadioButton("Text format (*.txt)");
    layout->addWidget(txtButton);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    QPushButton* okButton = new QPushButton("Export");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    int selectedBatch = resultCombo->currentData().toInt();
    bool includeExtra = extraCheckBox->isChecked();
    bool exportAttributes = attrCheckBox->isChecked();
    bool useCsv = csvButton->isChecked();
    
    const AnalysisResult* selectedResult = nullptr;
    if (hasBatchResult && lastBatchResult_) {
        if (selectedBatch >= 0 && selectedBatch < static_cast<int>(lastBatchResult_->allBatchResults.size())) {
            selectedResult = &lastBatchResult_->allBatchResults[selectedBatch];
        } else {
            selectedResult = &lastBatchResult_->bestResult;
        }
    } else {
        selectedResult = &lastResult_;
    }
    
    if (!selectedResult || !selectedResult->success) {
        emit showErrorMessage("Selected result is not valid");
        return;
    }
    
    QString fileExtension = useCsv ? ".csv" : ".txt";
    QString fileFilter = useCsv ? "CSV Files (*.csv);;All Files (*)" : "Text Files (*.txt);;All Files (*)";
    
    QString fileName = QFileDialog::getSaveFileName(nullptr, "Export Network", "", fileFilter);
    if (fileName.isEmpty()) return;
    
    if (!fileName.endsWith(fileExtension)) {
        fileName += fileExtension;
    }
    
    std::set<int> extraConnectors;
    if (includeExtra && selectedResult->hasDetailedResults) {
        for (const auto& permRes : selectedResult->allPermutationResults) {
            if (permRes.isValid) {
                for (int connId : permRes.connectorNodes) {
                    if (selectedResult->basicNetworkResult.connectorNodes.find(connId) 
                        == selectedResult->basicNetworkResult.connectorNodes.end()
                        && selectedResult->basicNetworkResult.seedNodes.find(connId)
                        == selectedResult->basicNetworkResult.seedNodes.end()) {
                        extraConnectors.insert(connId);
                    }
                }
            }
        }
    }
    
    std::set<int> exportedNodes;
    exportedNodes.insert(selectedResult->basicNetworkResult.seedNodes.begin(),
                         selectedResult->basicNetworkResult.seedNodes.end());
    exportedNodes.insert(selectedResult->basicNetworkResult.connectorNodes.begin(),
                         selectedResult->basicNetworkResult.connectorNodes.end());
    if (includeExtra) {
        exportedNodes.insert(extraConnectors.begin(), extraConnectors.end());
    }
    
    const Graph& originalGraph = finder_.getGraph();
    bool success = false;
    
    QFile edgesFile(fileName);
    if (edgesFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&edgesFile);
        
        if (useCsv) {
            out << "Source,Target\n";
        }
        
        auto allEdges = originalGraph.getEdges();
        for (const auto& edge : allEdges) {
            if (exportedNodes.count(edge.first) && exportedNodes.count(edge.second)) {
                QString src = QString::fromStdString(originalGraph.getNodeName(edge.first));
                QString tgt = QString::fromStdString(originalGraph.getNodeName(edge.second));
                
                if (useCsv) {
                    if (src.contains(',') || src.contains('"') || src.contains('\n'))
                        src = "\"" + src.replace("\"", "\"\"") + "\"";
                    if (tgt.contains(',') || tgt.contains('"') || tgt.contains('\n'))
                        tgt = "\"" + tgt.replace("\"", "\"\"") + "\"";
                    out << src << "," << tgt << "\n";
                } else {
                    out << src << " " << tgt << "\n";
                }
            }
        }
        
        edgesFile.close();
        success = true;
    }
    
    if (!success) {
        emit showErrorMessage("Failed to export network");
        return;
    }
    
    if (exportAttributes) {
        QString nodesFileName = fileName;
        int dotPos = nodesFileName.lastIndexOf('.');
        if (dotPos >= 0) {
            nodesFileName.insert(dotPos, "_nodes");
        } else {
            nodesFileName += "_nodes";
        }
        
        auto generalBetweenness = GraphAlgorithms::betweennessCentrality(originalGraph);
        
        std::unordered_map<int, double> basicNetBetweenness;
        const Graph* basicNetGraph = selectedResult->basicNetworkResult.basicNetwork_ptr.get();
        if (basicNetGraph && basicNetGraph->getNodeCount() > 0) {
            basicNetBetweenness = GraphAlgorithms::betweennessCentrality(*basicNetGraph);
        }
        
        QFile nodesFile(nodesFileName);
        if (nodesFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&nodesFile);
            out << "Id,Label,Type,Deg(G),Deg(L),Betw(G),Betw(L)\n";
            
            for (int nodeId : exportedNodes) {
                QString name = QString::fromStdString(originalGraph.getNodeName(nodeId));
                QString type;
                
                if (selectedResult->basicNetworkResult.seedNodes.count(nodeId)) {
                    type = "Seed";
                } else if (selectedResult->basicNetworkResult.connectorNodes.count(nodeId)) {
                    type = "Connector";
                } else if (extraConnectors.count(nodeId)) {
                    type = "Extra connector (>0%)";
                }
                
                int degG = static_cast<int>(originalGraph.getNeighbors(nodeId).size());
                
                int degL = 0;
                double betwL = 0.0;
                if (basicNetGraph) {
                    std::string nodeName = originalGraph.getNodeName(nodeId);
                    int bnId = basicNetGraph->getNodeId(nodeName);
                    if (bnId >= 0) {
                        degL = static_cast<int>(basicNetGraph->getNeighbors(bnId).size());
                        auto bnIt = basicNetBetweenness.find(bnId);
                        if (bnIt != basicNetBetweenness.end()) betwL = bnIt->second;
                    }
                }
                
                double betwG = 0.0;
                auto bgIt = generalBetweenness.find(nodeId);
                if (bgIt != generalBetweenness.end()) betwG = bgIt->second;
                
                QString escapedName = name;
                if (escapedName.contains(',') || escapedName.contains('"') || escapedName.contains('\n')) {
                    escapedName = "\"" + escapedName.replace("\"", "\"\"") + "\"";
                }
                
                out << escapedName << "," << escapedName << "," << type
                    << "," << degG << "," << degL
                    << "," << QString::number(betwG, 'f', 6)
                    << "," << QString::number(betwL, 'f', 6) << "\n";
            }
            nodesFile.close();
            
            emit showInfoMessage(QString("Network exported to:\n%1\n\nNode attributes exported to:\n%2")
                .arg(fileName, nodesFileName));
        } else {
            emit showInfoMessage(QString("Network exported to %1\nFailed to export node attributes file").arg(fileName));
        }
    } else {
        emit showInfoMessage(QString("Network exported successfully to %1").arg(fileName));
    }
}

bool AppController::exportNetworkAsCSV(const AnalysisResult& result, const QString& filename)
{
    if (!result.basicNetworkResult.basicNetwork_ptr) {
        return false;
    }
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    
    out << "Source,Target\n";
    
    const Graph& basicNetwork = *result.basicNetworkResult.basicNetwork_ptr;
    auto edges = basicNetwork.getEdges();
    
    for (const auto& edge : edges) {
        QString sourceName = QString::fromStdString(basicNetwork.getNodeName(edge.first));
        QString targetName = QString::fromStdString(basicNetwork.getNodeName(edge.second));
        
        if (sourceName.contains(',') || sourceName.contains('"') || sourceName.contains('\n')) {
            sourceName = "\"" + sourceName.replace("\"", "\"\"") + "\"";
        }
        if (targetName.contains(',') || targetName.contains('"') || targetName.contains('\n')) {
            targetName = "\"" + targetName.replace("\"", "\"\"") + "\"";
        }
        
        out << sourceName << "," << targetName << "\n";
    }
    
    file.close();
    return true;
}

void AppController::onExportOriginalGraphRequested()
{
    if (!hasValidGraph_) {
        emit showErrorMessage("No network loaded");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(nullptr,
        "Export Original Network", "", "Text Files (*.txt);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    if (!fileName.endsWith(".txt")) {
        fileName += ".txt";
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit showErrorMessage("Failed to open file for writing");
        return;
    }
    
    QTextStream out(&file);
    
    const Graph& graph = finder_.getGraph();
    auto edges = graph.getEdges();
    
    for (const auto& edge : edges) {
        QString sourceName = QString::fromStdString(graph.getNodeName(edge.first));
        QString targetName = QString::fromStdString(graph.getNodeName(edge.second));
        out << sourceName << " " << targetName << "\n";
    }
    
    file.close();
    
    emit showInfoMessage(QString("Original network exported to %1\nNodes: %2, Edges: %3")
        .arg(fileName)
        .arg(graph.getNodeCount())
        .arg(edges.size()));
}

void AppController::onExportStatisticsRequested()
{
    if (!hasValidGraph_) {
        emit showErrorMessage("No network loaded");
        return;
    }
    
    const Graph& graph = finder_.getGraph();
    
    int rejectedCount = graph.getRejectedSelfLoopsCount();
    int isolatedCount = graph.getIsolatedNodesCount();
    
    if (rejectedCount == 0 && isolatedCount == 0) {
        emit showInfoMessage("No statistics to export (no rejected self-loops or isolated nodes)");
        return;
    }
    
    QString dirPath = QFileDialog::getExistingDirectory(nullptr,
        "Select Directory to Export Statistics", "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (dirPath.isEmpty()) {
        return;
    }
    
    QString message;
    int filesExported = 0;
    
    if (rejectedCount > 0) {
        QString selfLoopsFile = dirPath + "/rejected_self_loops.txt";
        QFile file(selfLoopsFile);
        
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "# Rejected Self-Loops\n";
            out << "# Total: " << rejectedCount << "\n";
            out << "# Format: NodeName NodeName\n\n";
            
            auto rejectedLoops = graph.getRejectedSelfLoops();
            for (const auto& loop : rejectedLoops) {
                out << QString::fromStdString(loop.first) << " " 
                    << QString::fromStdString(loop.second) << "\n";
            }
            
            file.close();
            message += QString("Rejected self-loops: %1 entries -> rejected_self_loops.txt\n")
                      .arg(rejectedCount);
            filesExported++;
        } else {
            message += "Failed to export rejected self-loops\n";
        }
    }
    
    if (isolatedCount > 0) {
        QString isolatedNodesFile = dirPath + "/isolated_nodes.txt";
        QFile file(isolatedNodesFile);
        
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "# Isolated Nodes (nodes with no connections after self-loop filtering)\n";
            out << "# Total: " << isolatedCount << "\n\n";
            
            auto isolatedNodes = graph.getIsolatedNodeNames();
            for (const auto& nodeName : isolatedNodes) {
                out << QString::fromStdString(nodeName) << "\n";
            }
            
            file.close();
            message += QString("Isolated nodes: %1 entries -> isolated_nodes.txt\n")
                      .arg(isolatedCount);
            filesExported++;
        } else {
            message += "Failed to export isolated nodes\n";
        }
    }
    
    if (filesExported > 0) {
        message += QString("\nFiles saved to: %1").arg(dirPath);
        emit showInfoMessage(message);
    } else {
        emit showErrorMessage("Failed to export statistics files");
    }
}

void AppController::onSelectAllNodesRequested()
{
    if (!hasValidGraph_) {
        emit showErrorMessage("No network loaded");
        return;
    }
    
    try {
        auto nodes = finder_.getGraph().getNodes();
        std::vector<std::string> nodeNames;
        for (int nodeId : nodes) {
            nodeNames.push_back(finder_.getGraph().getNodeName(nodeId));
        }
        emit allNodeNamesReady(nodeNames);
        emit showInfoMessage(QString("Selected all %1 nodes as seeds").arg(nodeNames.size()));
    } catch (const std::exception& e) {
        emit showErrorMessage(QString("Error selecting all nodes: %1").arg(e.what()));
    }
}

void AppController::onClearSeedsRequested()
{
    emit seedsLoaded("");
    emit showInfoMessage("Seeds cleared");
}

void AppController::onNodeClicked(const std::string& nodeName)
{
    QString currentSeeds;
    
    if (!currentSeeds.isEmpty() && !currentSeeds.endsWith('\n')) {
        currentSeeds += '\n';
    }
    currentSeeds += QString::fromStdString(nodeName);
    
    emit seedsLoaded(currentSeeds);
    emit showInfoMessage(QString("Added node '%1' to seed list").arg(QString::fromStdString(nodeName)));
}

void AppController::onAnalysisFinished(const AnalysisResult& result)
{
    
    lastResult_ = result;

    QString formattedResults = formatResults(result);
    emit resultsReady(formattedResults);

    if (result.success) {
        emit basicNetworkReady(result.basicNetworkResult);

        std::map<int, int> connectorFrequency;
        if (result.hasDetailedResults && !result.allPermutationResults.empty()) {
            for (const auto& permResult : result.allPermutationResults) {
                if (permResult.isValid) {
                    for (int connId : permResult.connectorNodes) {
                        connectorFrequency[connId]++;
                    }
                }
            }
            emit connectorFrequencyReady(connectorFrequency, 
                                         static_cast<int>(result.allPermutationResults.size()));
        } else {
            for (int connId : result.basicNetworkResult.connectorNodes) {
                connectorFrequency[connId] = 1;
            }
            emit connectorFrequencyReady(connectorFrequency, 1);
        }
    }

    emit analysisFinished(result.success);
}

void AppController::onBatchAnalysisFinished(std::shared_ptr<BatchAnalysisResult> resultPtr)
{
    if (!resultPtr) {
        emit showErrorMessage("Error: received null result");
        return;
    }
    
    lastBatchResult_ = resultPtr;
    const BatchAnalysisResult& result = *resultPtr;
    
    try {
        QString formattedResults = formatBatchResults(result);
        emit resultsReady(formattedResults);
        
        if (result.bestResult.success) {
            emit basicNetworkReady(result.bestResult.basicNetworkResult);
            
            std::map<int, int> globalConnectorFrequency;
            int validBatches = 0;
            for (const auto& batchRes : result.allBatchResults) {
                if (batchRes.success) {
                    validBatches++;
                    for (int connId : batchRes.basicNetworkResult.connectorNodes) {
                        globalConnectorFrequency[connId]++;
                    }
                }
            }
            
            emit connectorFrequencyReady(globalConnectorFrequency, validBatches);
        }
        
        if (result.allBatchResults.size() > 1) {
            std::vector<std::set<int>> perBatchConnectors;
            std::vector<std::map<int, int>> perBatchConnectorFrequency;
            
            perBatchConnectors.reserve(result.allBatchResults.size());
            perBatchConnectorFrequency.reserve(result.allBatchResults.size());
            
            for (const auto& batchRes : result.allBatchResults) {
                if (batchRes.success) {
                    perBatchConnectors.push_back(batchRes.basicNetworkResult.connectorNodes);
                    
                    std::map<int, int> batchFreq;
                    for (int connId : batchRes.basicNetworkResult.connectorNodes) {
                        batchFreq[connId] = 1;
                    }
                    perBatchConnectorFrequency.push_back(std::move(batchFreq));
                }
            }
            emit perBatchConnectorsReady(perBatchConnectors, perBatchConnectorFrequency,
                                         static_cast<int>(result.allBatchResults.size()));
        }
        
        emit analysisFinished(result.bestResult.success);
        
    } catch (const std::exception& e) {
        emit showErrorMessage(QString("Critical error processing batch results: %1").arg(e.what()));
    } catch (...) {
        emit showErrorMessage("Critical unknown error processing batch results");
    }
}

void AppController::onAnalysisProgress(int current, int total, const QString& status)
{
    emit progressUpdate(current, total, status);
}

void AppController::onDetailedAnalysisProgress(int permutation, int totalPermutations, 
                                             const QString& stage, int stageProgress, int stageTotal)
{
    emit detailedProgressUpdate(permutation, totalPermutations, stage, stageProgress, stageTotal);
}

void AppController::onPermutationCompleted(int permutation, int totalPermutations, int connectorsFound)
{
    int displayedPermutation = ((permutation - 1) % std::max(1, totalPermutations)) + 1;
    emit permutationCompleted(displayedPermutation, totalPermutations, connectorsFound);
}

void AppController::onPermutationCompletedWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames)
{
    int displayedPermutation = ((permutation - 1) % std::max(1, totalPermutations)) + 1;
    emit permutationCompletedWithNames(displayedPermutation, totalPermutations, connectorsFound, connectorNames);
}

void AppController::onPermutationCompletedWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames, int preConnectorsFound)
{
    int displayedPermutation = ((permutation - 1) % std::max(1, totalPermutations)) + 1;
    emit permutationCompletedWithSeeds(displayedPermutation, totalPermutations, connectorsFound, connectorNames, seedNames, preConnectorsFound);
    
    emit permutationCompleted(displayedPermutation, totalPermutations, connectorsFound);
    emit permutationCompletedWithNames(displayedPermutation, totalPermutations, connectorsFound, connectorNames);
}

void AppController::onAnalysisError(const QString& error)
{
    emit showErrorMessage(error);
    
    emit analysisFinished(false);
}

void AppController::onGenerateRandomSeedsRequested(int amount)
{
    if (!hasValidGraph_) {
        emit showErrorMessage("No network loaded");
        return;
    }

    try {
        const Graph& graph = finder_.getGraph();
        auto nodeIds = graph.getNodes();
        
        if (nodeIds.empty()) {
            emit showErrorMessage("No nodes found in the loaded network");
            return;
        }

        std::vector<std::string> allNodeNames;
        for (int nodeId : nodeIds) {
            allNodeNames.push_back(graph.getNodeName(nodeId));
        }

        if (amount <= 0) {
            emit showErrorMessage("Amount must be greater than 0");
            return;
        }

        if (amount > static_cast<int>(allNodeNames.size())) {
            emit showErrorMessage(QString("Requested amount (%1) exceeds available nodes (%2)")
                                .arg(amount).arg(allNodeNames.size()));
            return;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, allNodeNames.size() - 1);

        std::vector<std::string> randomSeeds;
        std::set<int> selectedIndices;

        while (static_cast<int>(selectedIndices.size()) < amount) {
            int index = dis(gen);
            selectedIndices.insert(index);
        }

        for (int index : selectedIndices) {
            randomSeeds.push_back(allNodeNames[index]);
        }

        QStringList qRandomSeeds;
        for (const auto& seed : randomSeeds) {
            qRandomSeeds << QString::fromStdString(seed);
        }

        QString seedsContent = qRandomSeeds.join('\n');
        emit seedsLoaded(seedsContent);
        emit showInfoMessage(QString("Generated %1 random seeds from the dataset").arg(amount));

    } catch (const std::exception& e) {
        emit showErrorMessage(QString("Error generating random seeds: %1").arg(e.what()));
    }
}

void AppController::onBatchesRequested(bool enabled, int amount, int seedsPerBatch)
{
    if (!hasValidGraph_) {
        emit showErrorMessage("No network loaded");
        return;
    }

    if (!enabled) {
        emit showErrorMessage("Batches must be enabled");
        return;
    }

    try {
        const Graph& graph = finder_.getGraph();
        auto nodeIds = graph.getNodes();
        
        if (nodeIds.empty()) {
            emit showErrorMessage("No nodes found in the loaded network");
            return;
        }

        std::vector<std::string> allNodeNames;
        for (int nodeId : nodeIds) {
            allNodeNames.push_back(graph.getNodeName(nodeId));
        }

        if (amount <= 0) {
            emit showErrorMessage("Batches amount must be greater than 0");
            return;
        }

        if (seedsPerBatch <= 0) {
            emit showErrorMessage("Seeds per batch must be greater than 0");
            return;
        }

        if (seedsPerBatch > static_cast<int>(allNodeNames.size())) {
            emit showErrorMessage(QString("Requested seed amount (%1) exceeds available nodes (%2)")
                                .arg(seedsPerBatch).arg(allNodeNames.size()));
            return;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, allNodeNames.size() - 1);

        QStringList allBatchesContent;
        
        for (int batch = 0; batch < amount; ++batch) {
            std::vector<std::string> batchSeeds;
            std::set<int> selectedIndices;

            while (static_cast<int>(selectedIndices.size()) < seedsPerBatch) {
                int index = dis(gen);
                selectedIndices.insert(index);
            }

            for (int index : selectedIndices) {
                batchSeeds.push_back(allNodeNames[index]);
            }

            QStringList qBatchSeeds;
            for (const auto& seed : batchSeeds) {
                qBatchSeeds << QString::fromStdString(seed);
            }

            QString batchContent = qBatchSeeds.join('\n');
            allBatchesContent << batchContent;
        }

        QString finalContent = allBatchesContent.join("\n---BATCH_SEPARATOR---\n");
        
        emit seedsLoaded(finalContent);
        emit showInfoMessage(QString("Generated %1 batches of %2 random seeds each").arg(amount).arg(seedsPerBatch));

    } catch (const std::exception& e) {
        emit showErrorMessage(QString("Error generating batches: %1").arg(e.what()));
    }
}

QString AppController::formatResults(const AnalysisResult& result)
{
    if (!result.success) {
        return "Analysis failed.";
    }
    
    QString formattedResult;
    QTextStream stream(&formattedResult);
    
    stream << "=== BASIC NETWORK ANALYSIS RESULTS ===\n\n";
    
    if (result.basicNetworkResult.isValid) {
        stream << "BASIC NETWORK FOUND (final, after pruning if enabled):\n";
        stream << "  - Seed nodes: " << result.basicNetworkResult.seedNodes.size() << "\n";
        stream << "  - Connector nodes: " << result.basicNetworkResult.connectorNodes.size() << "\n";
        stream << "  - Total basic units: " << result.basicNetworkResult.allBasicUnits.size() << "\n";
        if (result.basicNetworkResult.basicNetwork_ptr) {
            stream << "  - Links: " << result.basicNetworkResult.basicNetwork_ptr->getEdgeCount() << "\n";
        }
        if (result.basicNetworkResult.preTotalConnectors > 0 &&
            result.basicNetworkResult.preTotalConnectors != result.basicNetworkResult.totalConnectors) {
            int preCount = result.basicNetworkResult.preTotalConnectors;
            int finalConnectors = static_cast<int>(result.basicNetworkResult.connectorNodes.size());
            stream << "  - Pruning applied (best permutation): " << preCount << " -> " << finalConnectors
                   << " connectors (" << (preCount - finalConnectors) << " removed)\n";
        }
        
        if (!result.basicNetworkResult.connectorNodes.empty()) {
            stream << "\nCONNECTOR NODES:\n";
            for (int connectorId : result.basicNetworkResult.connectorNodes) {
                try {
                    QString nodeName = QString::fromStdString(finder_.getGraph().getNodeName(connectorId));
                    stream << "  - " << nodeName << " (ID: " << connectorId << ")\n";
                } catch (...) {
                stream << "  - Node ID: " << connectorId << "\n";
                }
            }
        }
        
        if (!result.basicNetworkResult.seedNodes.empty()) {
            stream << "\nSEED NODES:\n";
            for (int seedId : result.basicNetworkResult.seedNodes) {
                try {
                    QString nodeName = QString::fromStdString(finder_.getGraph().getNodeName(seedId));
                    stream << "  - " << nodeName << " (ID: " << seedId << ")\n";
                } catch (...) {
                    stream << "  - Node ID: " << seedId << "\n";
                }
            }
        }
    } else {
        stream << "No basic network found with the given parameters.\n";
    }
    
    stream << "\nANALYSIS PARAMETERS:\n";
    stream << "  - Execution method: " << (result.usedParallelExecution ? "Parallel" : "Serial") << "\n";
    if (result.usedParallelExecution) {
        stream << "  - Threads used: " << result.threadsUsed << "\n";
        stream << "  - Parallel speedup: " << QString::number(result.parallelSpeedup, 'f', 2) << "x\n";
    }
    stream << "  - Execution time: " << QString::number(result.executionTimeSeconds, 'f', 2) << " seconds\n";
    
    if (result.hasDetailedResults && !result.allPermutationResults.empty()) {
        bool anyPerPermPruning = false;
        for (const auto& permResult : result.allPermutationResults) {
            if (permResult.isValid && permResult.preTotalConnectors > 0 &&
                permResult.preTotalConnectors != permResult.totalConnectors) {
                anyPerPermPruning = true;
                break;
            }
        }

        stream << "\n=== PERMUTATION RESULTS SUMMARY ===\n";
        if (anyPerPermPruning) {
            stream << "Note: Pruning is applied independently to EACH permutation.\n";
            stream << "      The connector counts below are post-pruning values.\n";
            stream << "      Pre-pruning heuristic counts are shown as reference statistics.\n";
        } else {
            stream << "Note: Pruning is disabled. The connector counts below correspond to the\n";
            stream << "      heuristic output of each permutation.\n";
        }
        stream << "Total permutations analyzed: " << result.allPermutationResults.size() << "\n\n";
        
        int totalPermutations = result.allPermutationResults.size();
        int successfulPermutations = std::count_if(result.allPermutationResults.begin(), result.allPermutationResults.end(),
            [](const auto& perm) { return perm.isValid; });
        int failedPermutations = totalPermutations - successfulPermutations;
        int tiedNetworks = std::count_if(result.allPermutationResults.begin(), result.allPermutationResults.end(),
            [](const auto& perm) { return perm.isTiedNetwork; });
        
        stream << "SUCCESS RATE ANALYSIS:\n";
        stream << "  - Successful permutations: " << successfulPermutations << " (" 
               << QString::number((successfulPermutations * 100.0) / totalPermutations, 'f', 1) << "%)\n";
        stream << "  - Failed permutations: " << failedPermutations << " (" 
               << QString::number((failedPermutations * 100.0) / totalPermutations, 'f', 1) << "%)\n";
        stream << "  - Tied networks found: " << tiedNetworks << " (" 
               << QString::number((tiedNetworks * 100.0) / totalPermutations, 'f', 1) << "%)\n";
        
        if (result.executionTimeSeconds > 0) {
            double permutationsPerSecond = totalPermutations / result.executionTimeSeconds;
            stream << "\nPERFORMANCE METRICS:\n";
            stream << "  - Permutations per second: " << QString::number(permutationsPerSecond, 'f', 2) << "\n";
            stream << "  - Average time per permutation: " << QString::number(result.executionTimeSeconds / totalPermutations * 1000, 'f', 2) << " ms\n";
            if (result.usedParallelExecution) {
                stream << "  - Theoretical serial time: " << QString::number(result.executionTimeSeconds * result.threadsUsed, 'f', 2) << " seconds\n";
                stream << "  - Time saved with parallelization: " << QString::number(result.executionTimeSeconds * (result.threadsUsed - 1), 'f', 2) << " seconds\n";
            }
        }
        
        stream << "\n";
        
        std::vector<int> connectorCounts;
        for (const auto& permResult : result.allPermutationResults) {
            if (permResult.isValid) {
                connectorCounts.push_back(permResult.totalConnectors);
            }
        }
        
        if (!connectorCounts.empty()) {
            std::sort(connectorCounts.begin(), connectorCounts.end());
            
            int minConnectors = connectorCounts.front();
            int maxConnectors = connectorCounts.back();
            double avgConnectors = std::accumulate(connectorCounts.begin(), connectorCounts.end(), 0.0) / connectorCounts.size();
            
            double variance = 0.0;
            for (int count : connectorCounts) {
                variance += std::pow(count - avgConnectors, 2);
            }
            variance /= connectorCounts.size();
            double stdDev = std::sqrt(variance);
            
            int p25 = connectorCounts[connectorCounts.size() * 0.25];
            int p50 = connectorCounts[connectorCounts.size() * 0.50];
            int p75 = connectorCounts[connectorCounts.size() * 0.75];
            
            std::map<int, int> frequency;
            for (int count : connectorCounts) {
                frequency[count]++;
            }
            auto modeIt = std::max_element(frequency.begin(), frequency.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            int mode = modeIt->first;
            int modeFreq = modeIt->second;
            
            int bestCount = std::count(connectorCounts.begin(), connectorCounts.end(), minConnectors);
            
            stream << "CONNECTOR STATISTICS (post-pruning when pruning is enabled):\n";
            stream << "  - Best (minimum): " << minConnectors << " connectors\n";
            stream << "  - Worst (maximum): " << maxConnectors << " connectors\n";
            stream << "  - Range: " << (maxConnectors - minConnectors) << " connectors\n";
            stream << "  - Average (mean): " << QString::number(avgConnectors, 'f', 2) << " connectors\n";
            stream << "  - Median (50th percentile): " << p50 << " connectors\n";
            stream << "  - Mode (most frequent): " << mode << " connectors (appears " << modeFreq << " times)\n";
            stream << "  - 25th percentile: " << p25 << " connectors\n";
            stream << "  - 75th percentile: " << p75 << " connectors\n";
            stream << "  - Interquartile range: " << (p75 - p25) << " connectors\n";
            stream << "  - Variance: " << QString::number(variance, 'f', 2) << "\n";
            stream << "  - Standard deviation: " << QString::number(stdDev, 'f', 2) << " connectors\n";
            stream << "  - Coefficient of variation: " << QString::number((stdDev / avgConnectors) * 100, 'f', 1) << "%\n";
            stream << "  - Permutations achieving best result: " << bestCount << " ("
                   << QString::number((bestCount * 100.0) / connectorCounts.size(), 'f', 1) << "%)\n";

            std::vector<int> preCounts;
            for (const auto& permResult : result.allPermutationResults) {
                if (permResult.isValid && permResult.preTotalConnectors > 0 &&
                    permResult.preTotalConnectors != permResult.totalConnectors) {
                    preCounts.push_back(permResult.preTotalConnectors);
                }
            }
            if (!preCounts.empty()) {
                std::sort(preCounts.begin(), preCounts.end());
                double preAvg = std::accumulate(preCounts.begin(), preCounts.end(), 0.0) / preCounts.size();
                int preMedian = preCounts[preCounts.size() / 2];
                stream << "\nPRE-PRUNING HEURISTIC STATISTICS (reference):\n";
                stream << "  - Best (minimum): " << preCounts.front() << " connectors\n";
                stream << "  - Worst (maximum): " << preCounts.back() << " connectors\n";
                stream << "  - Average (mean): " << QString::number(preAvg, 'f', 2) << " connectors\n";
                stream << "  - Median: " << preMedian << " connectors\n";
                int totalReduction = 0;
                for (const auto& permResult : result.allPermutationResults) {
                    if (permResult.isValid && permResult.preTotalConnectors > 0) {
                        totalReduction += (permResult.preTotalConnectors - permResult.totalConnectors);
                    }
                }
                if (!preCounts.empty()) {
                    double avgReduction = static_cast<double>(totalReduction) / preCounts.size();
                    stream << "  - Average connectors removed by pruning per permutation: "
                           << QString::number(avgReduction, 'f', 2) << "\n";
                }
            }
            
            stream << "\nDISTRIBUTION ANALYSIS:\n";
            int totalValid = connectorCounts.size();
            int excellent = std::count_if(connectorCounts.begin(), connectorCounts.end(), 
                [minConnectors](int x) { return x <= minConnectors + 2; });
            int good = std::count_if(connectorCounts.begin(), connectorCounts.end(), 
                [minConnectors](int x) { return x > minConnectors + 2 && x <= minConnectors + 5; });
            int fair = std::count_if(connectorCounts.begin(), connectorCounts.end(), 
                [minConnectors](int x) { return x > minConnectors + 5 && x <= minConnectors + 10; });
            int poor = std::count_if(connectorCounts.begin(), connectorCounts.end(), 
                [minConnectors](int x) { return x > minConnectors + 10; });
            
            stream << "  - Excellent (≤" << (minConnectors + 2) << "): " << excellent << " (" 
                   << QString::number((excellent * 100.0) / totalValid, 'f', 1) << "%)\n";
            stream << "  - Good (" << (minConnectors + 3) << "-" << (minConnectors + 5) << "): " << good << " (" 
                   << QString::number((good * 100.0) / totalValid, 'f', 1) << "%)\n";
            stream << "  - Fair (" << (minConnectors + 6) << "-" << (minConnectors + 10) << "): " << fair << " (" 
                   << QString::number((fair * 100.0) / totalValid, 'f', 1) << "%)\n";
            stream << "  - Poor (>" << (minConnectors + 10) << "): " << poor << " (" 
                   << QString::number((poor * 100.0) / totalValid, 'f', 1) << "%)\n";
        }
        
        bool showPre = false;
        for (const auto& permResult : result.allPermutationResults) {
            if (permResult.isValid && permResult.preTotalConnectors > 0 &&
                permResult.preTotalConnectors != permResult.totalConnectors) {
                showPre = true;
                break;
            }
        }

        stream << "\nDETAILED PERMUTATION RESULTS:\n";
        if (showPre) {
            stream << "Perm#\tPre-prune\tPost-prune\tValid\tTied\n";
            stream << "-----\t---------\t----------\t-----\t----\n";
        } else {
            stream << "Perm#\tConnectors\tValid\tTied\n";
            stream << "-----\t----------\t-----\t----\n";
        }

        int maxShow = std::min(20, static_cast<int>(result.allPermutationResults.size()));
        for (int i = 0; i < maxShow; ++i) {
            const auto& permResult = result.allPermutationResults[i];
            stream << QString("%1").arg(i + 1, 3) << "\t";
            if (showPre) {
                stream << QString("%1").arg(permResult.isValid ? permResult.preTotalConnectors : -1, 8) << "\t";
                stream << QString("%1").arg(permResult.isValid ? permResult.totalConnectors : -1, 8) << "\t";
            } else {
                stream << QString("%1").arg(permResult.isValid ? permResult.totalConnectors : -1, 8) << "\t";
            }
            stream << (permResult.isValid ? "YES" : "NO") << "\t";
            stream << (permResult.isTiedNetwork ? "YES" : "NO") << "\n";
        }
        
        if (result.allPermutationResults.size() > 20) {
            stream << "... (" << (result.allPermutationResults.size() - 20) << " more permutations)\n";
        }
        
        if (!connectorCounts.empty()) {
            double avgConnectors = std::accumulate(connectorCounts.begin(), connectorCounts.end(), 0.0) / connectorCounts.size();
            double variance = 0.0;
            for (int count : connectorCounts) {
                variance += std::pow(count - avgConnectors, 2);
            }
            variance /= connectorCounts.size();
            double stdDev = std::sqrt(variance);
            double permutationsPerSecond = result.executionTimeSeconds > 0 ? totalPermutations / result.executionTimeSeconds : 0;
            
            stream << "\n=== TREND ANALYSIS & RECOMMENDATIONS ===\n";
            
            double cv = (stdDev / avgConnectors) * 100;
            if (cv < 10) {
                stream << "CONSISTENCY: High - Results are very consistent (CV < 10%)\n";
                stream << "  - The algorithm is finding stable solutions\n";
                stream << "  - Current seed set appears optimal for this graph\n";
            } else if (cv < 25) {
                stream << "CONSISTENCY: Moderate - Results show some variation (CV 10-25%)\n";
                stream << "  - Consider running more permutations for better confidence\n";
                stream << "  - Seed selection may benefit from optimization\n";
            } else {
                stream << "CONSISTENCY: Low - High variability in results (CV > 25%)\n";
                stream << "  - Graph may have complex topology affecting solution stability\n";
                stream << "  - Consider different seed selection strategies\n";
            }
            
            double successRate = (successfulPermutations * 100.0) / totalPermutations;
            if (successRate > 90) {
                stream << "SUCCESS RATE: Excellent - " << QString::number(successRate, 'f', 1) << "% success rate\n";
                stream << "  - Algorithm parameters are well-suited for this graph\n";
            } else if (successRate > 70) {
                stream << "SUCCESS RATE: Good - " << QString::number(successRate, 'f', 1) << "% success rate\n";
                stream << "  - Consider adjusting algorithm parameters for better results\n";
            } else {
                stream << "SUCCESS RATE: Poor - " << QString::number(successRate, 'f', 1) << "% success rate\n";
                stream << "  - Graph may be challenging for current algorithm settings\n";
                stream << "  - Consider using different seed nodes or graph preprocessing\n";
            }
            
            stream << "\nPERFORMANCE RECOMMENDATIONS:\n";
            if (result.usedParallelExecution && result.parallelSpeedup > 8) {
                stream << "  - Parallel execution is highly effective (speedup > 8x)\n";
                stream << "  - Consider increasing thread count for larger analyses\n";
            } else if (result.usedParallelExecution && result.parallelSpeedup < 4) {
                stream << "  - Parallel speedup is limited (speedup < 4x)\n";
                stream << "  - Consider reducing thread count to minimize overhead\n";
            }
            
            if (permutationsPerSecond > 10) {
                stream << "  - High processing rate achieved\n";
                stream << "  - Suitable for large-scale analyses\n";
            } else if (permutationsPerSecond < 1) {
                stream << "  - Low processing rate detected\n";
                stream << "  - Consider optimizing graph representation or algorithm parameters\n";
            }
        }
    }
    
    if (result.hasStatisticalAnalysis) {
        stream << "\n=== STATISTICAL ANALYSIS ===\n";
        stream << "Statistical analysis completed successfully.\n";
        stream << "Detailed statistics available in saved files.\n";
    }
    
    stream << "\n=== EXECUTION SUMMARY ===\n";
    stream << "Analysis completed successfully at " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
    
    return formattedResult;
}

QString AppController::formatBatchResults(const BatchAnalysisResult& result)
{
    
    try {
        QString formattedResult;
        QTextStream stream(&formattedResult);
        
        stream << "=== BATCH ANALYSIS RESULTS ===\n\n";
        
        const Graph& graph = finder_.getGraph();
        stream << "NETWORK DATA:\n";
        if (graph.getInitialConnectionsCount() > 0) {
            stream << "  - Initial connections: " << graph.getInitialConnectionsCount() << "\n";
            stream << "  - Non-unique interactions: " << graph.getNonUniqueInteractionsCount() << "\n";
        }
        stream << "  - Nodes: " << graph.getNodeCount() << "\n";
        stream << "  - Edges: " << graph.getEdgeCount() << "\n";
        if (graph.getRejectedSelfLoopsCount() > 0) {
            stream << "  - Rejected self-loops: " << graph.getRejectedSelfLoopsCount() << "\n";
        }
        if (graph.getIsolatedNodesCount() > 0) {
            stream << "  - Isolated nodes: " << graph.getIsolatedNodesCount() << "\n";
        }
        stream << "\n";
        
        stream << "ALGORITHM CONFIGURATION:\n";
        stream << "  - Permutations: " << lastConfig_.numPermutations << "\n";
        stream << "  - Pruning: " << (lastConfig_.enablePruning ? "Yes" : "No") << "\n";
        if (lastConfig_.enablePruning) {
            stream << "  - Random pruning (ties): " << (lastConfig_.randomPruning ? "Yes" : "No") << "\n";
        }
        stream << "  - Parallel execution: " << (lastConfig_.enableParallelExecution ? "Yes" : "No") << "\n";
        if (lastConfig_.enableParallelExecution) {
            stream << "  - Max threads: " << lastConfig_.maxThreads << "\n";
        }
        stream << "\n";
        
        int permutationsPerBatch = 0;
        for (const auto& batchRes : result.allBatchResults) {
            if (batchRes.hasDetailedResults && !batchRes.allPermutationResults.empty()) {
                permutationsPerBatch = static_cast<int>(batchRes.allPermutationResults.size());
                break;
            }
        }
        
        stream << "BATCH SUMMARY:\n";
        stream << "  - Total batches processed: " << result.totalBatches << "\n";
        stream << "  - Successful batches: " << result.successfulBatches << "\n";
        stream << "  - Failed batches: " << result.failedBatches << "\n";
        stream << "  - Success rate: " << QString::number(result.successRate, 'f', 1) << "%\n";
        if (permutationsPerBatch > 0) {
            stream << "  - Permutations per batch: " << permutationsPerBatch << "\n";
        }
        stream << "\n";
        
        stream << "TIMING STATISTICS:\n";
        stream << "  - Total execution time: " << QString::number(result.wallClockTimeSeconds, 'f', 2) << " seconds\n";
        stream << "  - Cumulative batch time: " << QString::number(result.cumulativeBatchTime, 'f', 2) << " seconds\n";
        stream << "  - Average time per batch: " << QString::number(result.averageExecutionTime, 'f', 2) << " seconds\n";
        stream << "  - Minimum time: " << QString::number(result.minExecutionTime, 'f', 2) << " seconds\n";
        stream << "  - Maximum time: " << QString::number(result.maxExecutionTime, 'f', 2) << " seconds\n\n";
        
        if (result.connectorStatistics.allValues.size() > 0) {
            stream << "CONNECTOR STATISTICS (across all batches):\n";
            stream << "  - Mean: " << QString::number(result.connectorStatistics.mean, 'f', 2) << "\n";
            stream << "  - Standard deviation: " << QString::number(result.connectorStatistics.standardDeviation, 'f', 2) << "\n";
            stream << "  - Minimum: " << result.connectorStatistics.minimum << "\n";
            stream << "  - Maximum: " << result.connectorStatistics.maximum << "\n";
            
            if (result.connectorStatistics.allValues.size() <= 20) {
                QString valuesStr;
                for (size_t i = 0; i < result.connectorStatistics.allValues.size(); ++i) {
                    if (i > 0) valuesStr += ", ";
                    valuesStr += QString::number(result.connectorStatistics.allValues[i]);
                }
                stream << "  - All values: " << valuesStr << "\n";
            } else {
                stream << "  - All values: " << result.connectorStatistics.allValues.size() << " values (too many to display)\n";
            }
            stream << "\n";
        }
        
        if (!result.allBatchResultsWithoutPruning.empty() && !result.allBatchResultsWithPruning.empty()) {
            stream << "=== SEPARATE PRUNING STATISTICS ===\n\n";
            
            if (result.connectorStatisticsWithoutPruning.allValues.size() > 0) {
                stream << "CONNECTOR STATISTICS WITHOUT PRUNING:\n";
                stream << "  - Mean: " << QString::number(result.connectorStatisticsWithoutPruning.mean, 'f', 2) << "\n";
                stream << "  - Standard deviation: " << QString::number(result.connectorStatisticsWithoutPruning.standardDeviation, 'f', 2) << "\n";
                stream << "  - Minimum: " << result.connectorStatisticsWithoutPruning.minimum << "\n";
                stream << "  - Maximum: " << result.connectorStatisticsWithoutPruning.maximum << "\n";
                
                if (result.connectorStatisticsWithoutPruning.allValues.size() <= 20) {
                    QString valuesStr;
                    for (size_t i = 0; i < result.connectorStatisticsWithoutPruning.allValues.size(); ++i) {
                        if (i > 0) valuesStr += ", ";
                        valuesStr += QString::number(result.connectorStatisticsWithoutPruning.allValues[i]);
                    }
                    stream << "  - All values: " << valuesStr << "\n";
                }
                stream << "\n";
            }
            
            if (result.connectorStatisticsWithPruning.allValues.size() > 0) {
                stream << "CONNECTOR STATISTICS WITH PRUNING:\n";
                stream << "  - Mean: " << QString::number(result.connectorStatisticsWithPruning.mean, 'f', 2) << "\n";
                stream << "  - Standard deviation: " << QString::number(result.connectorStatisticsWithPruning.standardDeviation, 'f', 2) << "\n";
                stream << "  - Minimum: " << result.connectorStatisticsWithPruning.minimum << "\n";
                stream << "  - Maximum: " << result.connectorStatisticsWithPruning.maximum << "\n";
                
                if (result.connectorStatisticsWithPruning.allValues.size() <= 20) {
                    QString valuesStr;
                    for (size_t i = 0; i < result.connectorStatisticsWithPruning.allValues.size(); ++i) {
                        if (i > 0) valuesStr += ", ";
                        valuesStr += QString::number(result.connectorStatisticsWithPruning.allValues[i]);
                    }
                    stream << "  - All values: " << valuesStr << "\n";
                }
                stream << "\n";
            }
            
            if (result.connectorStatisticsWithoutPruning.allValues.size() > 0 && 
                result.connectorStatisticsWithPruning.allValues.size() > 0) {
                double meanDiff = result.connectorStatisticsWithPruning.mean - result.connectorStatisticsWithoutPruning.mean;
                QString effect = "NO CHANGE";
                if (meanDiff < -0.1) effect = "IMPROVEMENT";
                else if (meanDiff > 0.1) effect = "DEGRADATION";
                
                stream << "PRUNING COMPARISON:\n";
                stream << "  - Mean difference: " << QString::number(meanDiff, 'f', 2) << " connectors\n";
                stream << "  - Pruning effect: " << effect << "\n";
                stream << "  - Best without pruning: " << result.connectorStatisticsWithoutPruning.minimum << " connectors\n";
                stream << "  - Best with pruning: " << result.connectorStatisticsWithPruning.minimum << " connectors\n\n";
            }
        }
        
        if (result.bestResult.success && result.bestResult.basicNetworkResult.isValid) {
            int bestBatchNumber = -1;
            int bestConnectorCount = std::numeric_limits<int>::max();
            for (size_t i = 0; i < result.allBatchResults.size(); ++i) {
                const auto& br = result.allBatchResults[i];
                if (br.success && br.basicNetworkResult.isValid) {
                    int cc = static_cast<int>(br.basicNetworkResult.connectorNodes.size());
                    if (cc < bestConnectorCount) {
                        bestConnectorCount = cc;
                        bestBatchNumber = static_cast<int>(i) + 1;
                    }
                }
            }
            
            stream << "BEST RESULT (from batch with FEWEST connectors):\n";
            if (bestBatchNumber > 0) {
                stream << "  - Batch number: " << bestBatchNumber << "\n";
            }
            stream << "  - Seed nodes: " << result.bestResult.seedCount << "\n";
            stream << "  - Connector nodes: " << result.bestResult.connectorCount << "\n";
            stream << "  - Total basic units: " << (result.bestResult.seedCount + result.bestResult.connectorCount) << "\n";
            if (result.bestResult.basicNetworkResult.basicNetwork_ptr) {
                stream << "  - Links: " << result.bestResult.basicNetworkResult.basicNetwork_ptr->getEdgeCount() << "\n";
            }
            stream << "  - Original network: " << result.bestResult.originalNodeCount << " nodes, " 
                   << result.bestResult.originalEdgeCount << " edges\n";
            stream << "  - Reduction: " << QString::number(result.bestResult.reductionPercentage, 'f', 1) << "%\n\n";
            
            if (!result.bestResult.basicNetworkResult.connectorNodes.empty()) {
                stream << "CONNECTOR NODES (from best result):\n";
                for (int connectorId : result.bestResult.basicNetworkResult.connectorNodes) {
                    try {
                        QString nodeName = QString::fromStdString(finder_.getGraph().getNodeName(connectorId));
                        stream << "  - " << nodeName << " (ID: " << connectorId << ")\n";
                    } catch (...) {
                        stream << "  - Node ID: " << connectorId << "\n";
                    }
                }
                stream << "\n";
            }
            
            if (!result.bestResult.basicNetworkResult.seedNodes.empty()) {
                stream << "SEED NODES (from best result):\n";
                for (int seedId : result.bestResult.basicNetworkResult.seedNodes) {
                    try {
                        QString nodeName = QString::fromStdString(finder_.getGraph().getNodeName(seedId));
                        stream << "  - " << nodeName << " (ID: " << seedId << ")\n";
                    } catch (...) {
                        stream << "  - Node ID: " << seedId << "\n";
                    }
                }
                stream << "\n";
            }
        }
        
        if (!result.allBatchResults.empty()) {
            stream << "INDIVIDUAL BATCH RESULTS:\n";
            int maxShow = std::min(10, static_cast<int>(result.allBatchResults.size()));
            for (int i = 0; i < maxShow; ++i) {
                const auto& batchResult = result.allBatchResults[i];
                
                if (batchResult.success && batchResult.basicNetworkResult.isValid) {
                    int actualConnectors = batchResult.basicNetworkResult.connectorNodes.size();
                    stream << "  Batch " << (i + 1) << ": SUCCESS - " 
                           << actualConnectors << " connectors - "
                           << QString::number(batchResult.executionTimeSeconds, 'f', 2) << "s";
                    
                    if (batchResult.basicNetworkResult.isTiedNetwork) {
                        stream << " (TIED)";
                    }
                    stream << "\n";
                } else {
                    stream << "  Batch " << (i + 1) << ": FAILED - " 
                           << QString::fromStdString(batchResult.errorMessage) << "\n";
                }
            }
            
            if (result.allBatchResults.size() > 10) {
                stream << "  ... (" << (result.allBatchResults.size() - 10) << " more batches)\n";
            }
            stream << "\n";
        }
        
        if (result.connectorStatistics.allValues.size() > 0) {
            stream << "=== PERFORMANCE ANALYSIS ===\n";
            
            if (result.successRate > 90) {
                stream << "SUCCESS RATE: Excellent - " << QString::number(result.successRate, 'f', 1) << "% success rate\n";
                stream << "  - Algorithm parameters are well-suited for this graph\n";
            } else if (result.successRate > 70) {
                stream << "SUCCESS RATE: Good - " << QString::number(result.successRate, 'f', 1) << "% success rate\n";
                stream << "  - Consider adjusting algorithm parameters for better results\n";
            } else {
                stream << "SUCCESS RATE: Poor - " << QString::number(result.successRate, 'f', 1) << "% success rate\n";
                stream << "  - Graph may be challenging for current algorithm settings\n";
                stream << "  - Consider using different seed nodes or graph preprocessing\n";
            }
            
        }
        
        stream << "\n=== EXECUTION SUMMARY ===\n";
        stream << "Analysis completed successfully at " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
        
        return formattedResult;
        
    } catch (const std::exception& e) {
        return "ERROR: Failed to format batch results due to critical error: " + QString::fromStdString(e.what());
    } catch (...) {
        return "ERROR: Failed to format batch results due to unknown critical error";
    }
}

void AppController::onStopAnalysisRequested()
{
    
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->requestStop();
        
        if (!workerThread_->wait(3000)) {
            workerThread_->quit();
            if (!workerThread_->wait(2000)) {
            }
        }
        
        emit showInfoMessage("Analysis stop requested");
    } else {
        emit showInfoMessage("No analysis is currently running");
    }
}

void AppController::onApplyGraphFilter(const GraphFilterConfig& config)
{
    if (!hasValidGraph_) {
        emit showErrorMessage("No graph loaded to filter");
        return;
    }
    
    if (!hasOriginalGraph_) {
        originalGraph_ = finder_.getGraph();
        hasOriginalGraph_ = true;
    }
    
    const Graph& sourceGraph = originalGraph_;
    std::vector<int> allNodes = sourceGraph.getNodes();
    std::set<int> keepNodes(allNodes.begin(), allNodes.end());
    
    QStringList details;
    std::unordered_map<int, double> betweennessCache;
    bool betweennessComputed = false;
    
    for (const auto& layer : config.orderedFilters) {
        std::set<int> toRemove;
        int removedCount = 0;
        
        switch (layer.type) {
            case FilterType::Degree: {
                int degMin = layer.parameters.value("min", 0).toInt();
                int degMax = layer.parameters.value("max", 999999).toInt();
                for (int nodeId : keepNodes) {
                    int deg = 0;
                    for (int neighbor : sourceGraph.getNeighbors(nodeId)) {
                        if (keepNodes.find(neighbor) != keepNodes.end() && neighbor != nodeId) {
                            deg++;
                        }
                    }
                    if (deg < degMin || deg > degMax) {
                        toRemove.insert(nodeId);
                    }
                }
                removedCount = static_cast<int>(toRemove.size());
                for (int id : toRemove) keepNodes.erase(id);
                if (removedCount > 0) {
                    details << QString("Degree [%1-%2]: %3 removed").arg(degMin).arg(degMax).arg(removedCount);
                }
                break;
            }
            
            case FilterType::Betweenness: {
                if (!betweennessComputed) {
                    betweennessCache = GraphAlgorithms::betweennessCentrality(sourceGraph);
                    betweennessComputed = true;
                }
                double bwMin = layer.parameters.value("min", 0.0).toDouble();
                double bwMax = layer.parameters.value("max", 1.0).toDouble();
                for (int nodeId : keepNodes) {
                    double bw = 0.0;
                    auto it = betweennessCache.find(nodeId);
                    if (it != betweennessCache.end()) bw = it->second;
                    if (bw < bwMin || bw > bwMax) {
                        toRemove.insert(nodeId);
                    }
                }
                removedCount = static_cast<int>(toRemove.size());
                for (int id : toRemove) keepNodes.erase(id);
                if (removedCount > 0) {
                    details << QString("Betweenness [%1-%2]: %3 removed").arg(bwMin).arg(bwMax).arg(removedCount);
                }
                break;
            }
            
            case FilterType::RemoveIsolated: {
                for (int nodeId : keepNodes) {
                    bool hasNeighborInKeepNodes = false;
                    for (int neighbor : sourceGraph.getNeighbors(nodeId)) {
                        if (keepNodes.find(neighbor) != keepNodes.end() && neighbor != nodeId) {
                            hasNeighborInKeepNodes = true;
                            break;
                        }
                    }
                    if (!hasNeighborInKeepNodes) {
                        toRemove.insert(nodeId);
                    }
                }
                removedCount = static_cast<int>(toRemove.size());
                for (int id : toRemove) keepNodes.erase(id);
                if (removedCount > 0) {
                    details << QString("Isolated: %1 removed").arg(removedCount);
                }
                break;
            }
            
            case FilterType::NamePattern: {
                QString pattern = layer.parameters.value("pattern", "").toString();
                bool exclude = layer.parameters.value("exclude", false).toBool();
                if (!pattern.isEmpty()) {
                    QString regexPattern = QRegularExpression::escape(pattern);
                    regexPattern.replace("\\*", ".*");
                    regexPattern.replace("\\?", ".");
                    QRegularExpression regex("^" + regexPattern + "$", QRegularExpression::CaseInsensitiveOption);
                    
                    for (int nodeId : keepNodes) {
                        QString nodeName = QString::fromStdString(sourceGraph.getNodeName(nodeId));
                        bool matches = regex.match(nodeName).hasMatch();
                        if (exclude && matches) {
                            toRemove.insert(nodeId);
                        } else if (!exclude && !matches) {
                            toRemove.insert(nodeId);
                        }
                    }
                    removedCount = static_cast<int>(toRemove.size());
                    for (int id : toRemove) keepNodes.erase(id);
                    if (removedCount > 0) {
                        details << QString("Name '%1': %2 removed").arg(pattern).arg(removedCount);
                    }
                }
                break;
            }
            
            case FilterType::LargestComponent: {
                if (keepNodes.empty()) break;
                Graph tempGraph = sourceGraph.getSubgraph(keepNodes);
                auto components = tempGraph.getConnectedComponents();
                if (components.size() > 1) {
                    size_t largestIdx = 0;
                    for (size_t i = 1; i < components.size(); ++i) {
                        if (components[i].size() > components[largestIdx].size()) {
                            largestIdx = i;
                        }
                    }
                    std::set<int> largestSet(components[largestIdx].begin(), components[largestIdx].end());
                    int beforeCount = static_cast<int>(keepNodes.size());
                    keepNodes = largestSet;
                    removedCount = beforeCount - static_cast<int>(keepNodes.size());
                    if (removedCount > 0) {
                        details << QString("Largest component: %1 removed").arg(removedCount);
                    }
                }
                break;
            }
        }
        
        if (keepNodes.empty()) {
            emit showErrorMessage("Filter would remove all nodes. No changes applied.");
            return;
        }
    }
    
    Graph filteredGraph = sourceGraph.getSubgraph(keepNodes);
    
    if (filteredGraph.getNodeCount() == 0) {
        emit showErrorMessage("Filter would remove all nodes. No changes applied.");
        return;
    }
    
    finder_.getGraph() = filteredGraph;
    
    int filteredMaxDegree = 0;
    int filteredMinDegree = std::numeric_limits<int>::max();
    for (int nodeId : filteredGraph.getNodes()) {
        int deg = static_cast<int>(filteredGraph.getNeighbors(nodeId).size());
        if (deg > filteredMaxDegree) filteredMaxDegree = deg;
        if (deg < filteredMinDegree) filteredMinDegree = deg;
    }
    if (filteredMinDegree == std::numeric_limits<int>::max()) filteredMinDegree = 0;
    
    QString statusMsg = QString("Filtered: %1 nodes, %2 edges (from %3 nodes)\nDegree range in result: %4-%5")
        .arg(filteredGraph.getNodeCount())
        .arg(filteredGraph.getEdgeCount())
        .arg(originalGraph_.getNodeCount())
        .arg(filteredMinDegree)
        .arg(filteredMaxDegree);
    if (!details.isEmpty()) {
        statusMsg += "\n" + details.join(", ");
    }
    
    emit graphFiltered(statusMsg, filteredGraph.getNodeCount(), filteredGraph.getEdgeCount());
    emit graphReadyForDisplay(filteredGraph);
    
    auto nodes = filteredGraph.getNodes();
    std::vector<std::string> nodeNames;
    nodeNames.reserve(nodes.size());
    for (int nodeId : nodes) {
        nodeNames.push_back(filteredGraph.getNodeName(nodeId));
    }
    emit nodeListReady(nodeNames);
    
    emit showInfoMessage(statusMsg);
}

void AppController::onResetGraphFilter()
{
    if (!hasOriginalGraph_) {
        emit showInfoMessage("No filter has been applied");
        return;
    }
    
    finder_.getGraph() = originalGraph_;
    hasOriginalGraph_ = false;
    
    const Graph& graph = finder_.getGraph();
    
    emit graphFiltered(QString("Original graph restored: %1 nodes, %2 edges")
        .arg(graph.getNodeCount()).arg(graph.getEdgeCount()),
        graph.getNodeCount(), graph.getEdgeCount());
    emit graphReadyForDisplay(graph);
    
    auto nodes = graph.getNodes();
    std::vector<std::string> nodeNames;
    nodeNames.reserve(nodes.size());
    for (int nodeId : nodes) {
        nodeNames.push_back(graph.getNodeName(nodeId));
    }
    emit nodeListReady(nodeNames);
    
    emit showInfoMessage(QString("Original graph restored: %1 nodes, %2 edges")
        .arg(graph.getNodeCount()).arg(graph.getEdgeCount()));
}

const BasicNetworkResult* AppController::getBatchResult(int batchIndex) const
{
    if (!lastBatchResult_ || batchIndex < 0 || 
        batchIndex >= static_cast<int>(lastBatchResult_->allBatchResults.size())) {
        return nullptr;
    }
    return &lastBatchResult_->allBatchResults[batchIndex].basicNetworkResult;
}

int AppController::getBatchCount() const
{
    if (!lastBatchResult_) {
        return 0;
    }
    return static_cast<int>(lastBatchResult_->allBatchResults.size());
}

const BasicNetworkResult* AppController::getPermutationResult(int batchIndex, int permutationIndex) const
{
    if (permutationIndex < 0) {
        return nullptr;
    }
    
    if (lastBatchResult_ && batchIndex >= 0 && 
        batchIndex < static_cast<int>(lastBatchResult_->allBatchResults.size())) {
        const auto& batchResult = lastBatchResult_->allBatchResults[batchIndex];
        if (batchResult.hasDetailedResults && 
            permutationIndex < static_cast<int>(batchResult.allPermutationResults.size())) {
            return &batchResult.allPermutationResults[permutationIndex];
        }
    } else if (lastResult_.success && lastResult_.hasDetailedResults) {
        if (permutationIndex < static_cast<int>(lastResult_.allPermutationResults.size())) {
            return &lastResult_.allPermutationResults[permutationIndex];
        }
    }
    
    return nullptr;
}

int AppController::getPermutationCount(int batchIndex) const
{
    if (lastBatchResult_ && batchIndex >= 0 && 
        batchIndex < static_cast<int>(lastBatchResult_->allBatchResults.size())) {
        const auto& batchResult = lastBatchResult_->allBatchResults[batchIndex];
        if (batchResult.hasDetailedResults) {
            return static_cast<int>(batchResult.allPermutationResults.size());
        }
    } else if (lastResult_.success && lastResult_.hasDetailedResults) {
        return static_cast<int>(lastResult_.allPermutationResults.size());
    }
    return 0;
}
