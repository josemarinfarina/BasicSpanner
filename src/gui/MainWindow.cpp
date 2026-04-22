#include "MainWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QTextEdit>
#include <QKeySequence>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <chrono>
#include <iostream>

#include "panels/SeedSelectionPanel.h"
#include "panels/FileOperationsPanel.h"
#include "panels/AlgorithmConfigPanel.h"
#include "panels/ExecutionPanel.h"
#include "panels/ResultsPanel.h"
#include "panels/VisualizationControlPanel.h"
#include "panels/NodeListPanel.h"
#include "../core/GraphAlgorithms.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , appController_(nullptr)
    , uiManager_(nullptr)
    , realTimeHistogramWindow_(nullptr)
    , graphScene_(nullptr)
    , graphVisualization_(nullptr)
    , tabWidget_(nullptr)
    , userGuideTab_(nullptr)
{
    setWindowTitle("BasicSpanner");
    setMinimumSize(1200, 800);
    
    appController_ = new AppController(this);
    uiManager_ = new UIManager(this);
    recentFilesManager_ = new RecentFilesManager(this);
    
    setupUI();
    setupMenus();
    setupConnections();
}

MainWindow::~MainWindow()
{
    
    if (realTimeHistogramWindow_) {
        try {
            
            realTimeHistogramWindow_->close();
            realTimeHistogramWindow_->deleteLater();
            realTimeHistogramWindow_ = nullptr;
        } catch (...) {
        }
    }
    
}

void MainWindow::setupUI()
{
    tabWidget_ = new QTabWidget;
    
    viewerContainer_ = new QWidget(this);
    QVBoxLayout* viewerLayout = new QVBoxLayout(viewerContainer_);
    viewerLayout->setContentsMargins(0, 0, 0, 0);
    viewerLayout->setSpacing(0);
    
    graphVisualization_ = new GraphVisualization(this);
    viewerLayout->addWidget(graphVisualization_, 1);
    
    colorLegend_ = new QWidget(this);
    colorLegend_->setFixedHeight(50);
    colorLegend_->setStyleSheet(
        "QWidget { background-color: #1a1f2e; border-top: 1px solid #3d4a5c; }"
    );
    QHBoxLayout* legendLayout = new QHBoxLayout(colorLegend_);
    legendLayout->setContentsMargins(20, 8, 20, 8);
    
    QLabel* minLabel = new QLabel("0.00");
    minLabel->setObjectName("legendMinLabel");
    minLabel->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: bold;");
    legendLayout->addWidget(minLabel);
    
    QWidget* gradientBar = new QWidget;
    gradientBar->setObjectName("legendGradientBar");
    gradientBar->setFixedHeight(20);
    gradientBar->setMinimumWidth(300);
    gradientBar->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 #3b82f6, stop:0.5 #facc15, stop:1 #ef4444);"
        "border-radius: 4px;"
    );
    legendLayout->addWidget(gradientBar, 1);
    
    QLabel* maxLabel = new QLabel("1.00");
    maxLabel->setObjectName("legendMaxLabel");
    maxLabel->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: bold;");
    legendLayout->addWidget(maxLabel);
    
    legendLayout->addSpacing(20);
    QLabel* attrLabel = new QLabel("Attribute");
    attrLabel->setObjectName("legendAttrLabel");
    attrLabel->setStyleSheet("color: #e2e8f0; font-size: 12px; font-weight: bold;");
    legendLayout->addWidget(attrLabel);
    
    colorLegend_->setVisible(false);
    viewerLayout->addWidget(colorLegend_);
    
    tabWidget_->addTab(viewerContainer_, "Viewer");
    
    userGuideTab_ = new QWidget;
    setupUserGuideTab();
    tabWidget_->addTab(userGuideTab_, "User Guide");
    
    setCentralWidget(tabWidget_);
}

void MainWindow::setupUserGuideTab()
{
    QVBoxLayout* guideLayout = new QVBoxLayout(userGuideTab_);
    
    QScrollArea* userGuideScrollArea = new QScrollArea;
    userGuideScrollArea->setWidgetResizable(true);
    userGuideScrollArea->setFrameShape(QFrame::NoFrame);
    
    QTextEdit* userGuideText = new QTextEdit;
    userGuideText->setReadOnly(true);
    
    QString guideContent = R"(
<!DOCTYPE html>
<html>
<head>
<style>
body { 
    background-color: transparent; 
    color: #e2e8f0; 
    font-family: 'Segoe UI', Arial, sans-serif; 
    line-height: 1.7; 
    margin: 30px; 
}
h1 { color: #00d4aa; margin-bottom: 10px; font-size: 32px; }
.subtitle { color: #94a3b8; font-size: 16px; margin-bottom: 30px; }
h2 { color: #00d4aa; margin-top: 35px; margin-bottom: 15px; border-bottom: 2px solid #00d4aa; padding-bottom: 8px; font-size: 22px; }
h3 { color: #60a5fa; margin-top: 25px; margin-bottom: 12px; font-size: 18px; }
.section {
    background-color: rgba(30, 41, 59, 0.8);
    border-left: 4px solid #00d4aa;
    padding: 20px 25px;
    margin: 20px 0;
    border-radius: 8px;
}
.step {
    background-color: rgba(30, 58, 138, 0.3);
    padding: 18px;
    border-radius: 10px;
    margin: 12px 0;
    border-left: 4px solid #3b82f6;
}
.tip {
    background-color: rgba(0, 212, 170, 0.15);
    border: 1px solid #00d4aa;
    color: #e2e8f0;
    padding: 15px 18px;
    border-radius: 10px;
    margin: 15px 0;
}
.tip strong { color: #00d4aa; }
.warning {
    background-color: rgba(239, 68, 68, 0.15);
    border: 1px solid #ef4444;
    color: #e2e8f0;
    padding: 15px 18px;
    border-radius: 10px;
    margin: 15px 0;
}
.warning strong { color: #ef4444; }
pre {
    background-color: #1e293b;
    color: #e2e8f0;
    padding: 18px;
    border-radius: 8px;
    overflow-x: auto;
    font-family: 'Consolas', 'Monaco', monospace;
    font-size: 13px;
    border: 1px solid #334155;
}
ul, ol { margin-left: 25px; }
li { margin-bottom: 10px; }
strong { color: #f1f5f9; }
.highlight { color: #00d4aa; font-weight: bold; }
.param { color: #facc15; }
table { border-collapse: collapse; width: 100%; margin: 15px 0; }
th { background-color: #1e293b; color: #00d4aa; padding: 12px; text-align: left; border: 1px solid #334155; }
td { padding: 10px 12px; border: 1px solid #334155; }
tr:nth-child(even) { background-color: rgba(30, 41, 59, 0.5); }
</style>
</head>
<body>

<h1>BasicSpanner</h1>
<p class="subtitle">Find the minimal subnetwork connecting your seed nodes</p>

<div class="section">
<h2>What is BasicSpanner?</h2>
<p>BasicSpanner is a tool for finding the <strong>basic network</strong> (or minimal spanning subnetwork) that connects a set of user-defined <span class="highlight">seed nodes</span>. The algorithm identifies the smallest set of <span class="highlight">connector nodes</span> needed to maintain connectivity between all seeds.</p>

<div class="tip">
<strong>Key Concept:</strong> Given a network and a set of seed nodes, BasicSpanner finds the minimal set of intermediate nodes (connectors) required to keep all seeds connected.
</div>
</div>

<div class="section">
<h2>Quick Start Guide</h2>

<h3>Step 1: Load Your Network</h3>
<div class="step">
<ol>
<li>Go to <strong>File Operations</strong> panel (left side)</li>
<li>Click <strong>"Load Network"</strong></li>
<li>Select your network file (edge list format)</li>
<li>Configure the column mapping if needed</li>
<li>The network will appear in the Viewer tab</li>
</ol>
</div>

<h3>Step 2: Define Seed Nodes</h3>
<div class="step">
<ol>
<li>Go to <strong>Seed Selection</strong> panel</li>
<li>Enter seed node names/IDs, one per line</li>
<li>Or click <strong>"Load Seeds"</strong> to load from a file</li>
<li>Seeds will be highlighted in <span style="color: #ff6b6b;">red</span> in the viewer</li>
</ol>
</div>

<h3>Step 3: Configure and Run Analysis</h3>
<div class="step">
<ol>
<li>Go to <strong>Algorithm Configuration</strong> panel (right side)</li>
<li>Set the number of <strong>permutations</strong> (more = more accurate)</li>
<li>Enable <strong>pruning</strong> for cleaner results</li>
<li>Click <strong>"Run Analysis"</strong> in Execution Control</li>
<li>View results in the <strong>Results and Export</strong> panel</li>
</ol>
</div>
</div>

<div class="section">
<h2>Understanding the Interface</h2>

<h3>Left Panel Tabs</h3>
<table>
<tr><th>Panel</th><th>Description</th></tr>
<tr><td><strong>File Operations</strong></td><td>Load networks, load seeds, export results</td></tr>
<tr><td><strong>Visualization Controls</strong></td><td>Layout algorithms, visual settings, node coloring</td></tr>
<tr><td><strong>Seed Selection</strong></td><td>Define and manage seed nodes</td></tr>
<tr><td><strong>Results and Export</strong></td><td>View analysis results and export data</td></tr>
</table>

<h3>Right Panel Tabs</h3>
<table>
<tr><th>Panel</th><th>Description</th></tr>
<tr><td><strong>Algorithm Configuration</strong></td><td>Set permutations, pruning, batch mode</td></tr>
<tr><td><strong>Execution Control</strong></td><td>Run/stop analysis, view progress, real-time monitoring</td></tr>
</table>

<h3>Viewer Tab</h3>
<p>The main visualization area where your network is displayed:</p>
<ul>
<li><span style="color: #ff6b6b;"><strong>Red nodes</strong></span> = Seed nodes</li>
<li><span style="color: #1e40af;"><strong>Blue nodes</strong></span> = Connector nodes (in best result)</li>
<li><span style="color: #fbbf24;"><strong>Yellow nodes</strong></span> = Extra connectors (appeared in other permutations)</li>
<li><span style="color: #00d4aa;"><strong>Cyan nodes</strong></span> = Regular nodes</li>
</ul>
</div>

<div class="section">
<h2>File Formats</h2>

<h3>Network File (Edge List)</h3>
<p>Your network should be a text file with edges listed as pairs of node names:</p>
<pre>
NodeA    NodeB
NodeA    NodeC
NodeB    NodeD
NodeC    NodeD
</pre>

<p>Or with a header line specifying node and edge counts:</p>
<pre>
4 4
NodeA NodeB
NodeA NodeC
NodeB NodeD
NodeC NodeD
</pre>

<h3>Seed File</h3>
<p>One seed node name per line:</p>
<pre>
NodeA
NodeD
</pre>
</div>

<div class="section">
<h2>Algorithm Parameters</h2>

<h3>Permutations</h3>
<p>The algorithm runs multiple permutations to find the optimal solution:</p>
<table>
<tr><th>Value</th><th>Speed</th><th>Accuracy</th><th>Recommended For</th></tr>
<tr><td>1-100</td><td>Very Fast</td><td>Low</td><td>Quick exploration</td></tr>
<tr><td>100-1,000</td><td>Fast</td><td>Medium</td><td>Initial analysis</td></tr>
<tr><td>1,000-10,000</td><td>Moderate</td><td>High</td><td>Publication-quality results</td></tr>
<tr><td>10,000+</td><td>Slow</td><td>Very High</td><td>Critical analyses</td></tr>
</table>

<h3>Pruning</h3>
<p>Removes redundant connector nodes that don't contribute to connectivity. <strong>Recommended: Always ON</strong> for cleaner results.</p>

<h3>Batch Mode</h3>
<p>Run analysis on multiple random seed sets to assess network robustness. Useful for statistical validation.</p>
</div>

<div class="section">
<h2>Visualization Features</h2>

<h3>Layout Algorithms</h3>
<ul>
<li><strong>Seed-Centric:</strong> Places seeds at center, connectors radially outward</li>
<li><strong>FWTL (Frequency-Weighted):</strong> Novel layout using connector frequency from permutations</li>
<li><strong>Yifan Hu:</strong> Fast force-directed layout for large networks</li>
<li><strong>Force Atlas:</strong> Classic force-directed with customizable parameters</li>
<li><strong>Circular / Grid / Random:</strong> Simple geometric layouts</li>
</ul>

<h3>Node Color Attributes</h3>
<p>Color nodes by network metrics (View Mode section):</p>
<ul>
<li><strong>Seeds/Connectors:</strong> Default analysis coloring</li>
<li><strong>Degree Centrality:</strong> Number of connections</li>
<li><strong>Betweenness Centrality:</strong> How often node lies on shortest paths</li>
<li><strong>Closeness Centrality:</strong> Average distance to all other nodes</li>
<li><strong>Clustering Coefficient:</strong> How connected neighbors are</li>
<li><strong>PageRank:</strong> Node importance measure</li>
</ul>

<div class="tip">
<strong>Tip:</strong> Use the color legend at the bottom of the viewer to interpret attribute values.
</div>
</div>

<div class="section">
<h2>Interpreting Results</h2>

<h3>Analysis Output</h3>
<ul>
<li><strong>Seed Nodes:</strong> Your input seeds</li>
<li><strong>Connector Nodes:</strong> Minimal set connecting all seeds</li>
<li><strong>Basic Network Size:</strong> Total nodes and edges in the result</li>
<li><strong>Compression Ratio:</strong> How much smaller the basic network is vs. original</li>
</ul>

<h3>Connector Frequency</h3>
<p>When running multiple permutations, BasicSpanner tracks how often each connector appears:</p>
<ul>
<li><strong>High frequency (near 100%):</strong> Essential connector - appears in most/all solutions</li>
<li><strong>Low frequency:</strong> Alternative connector - may be replaceable</li>
</ul>

<div class="tip">
<strong>Tip:</strong> Enable "Connector frequency brightness" to visualize connector reliability.
</div>
</div>

<div class="section">
<h2>Keyboard Shortcuts</h2>
<table>
<tr><th>Shortcut</th><th>Action</th></tr>
<tr><td><strong>Ctrl+O</strong></td><td>Load Network</td></tr>
<tr><td><strong>Ctrl+S</strong></td><td>Save Results</td></tr>
<tr><td><strong>F5</strong></td><td>Run Analysis</td></tr>
<tr><td><strong>Ctrl+Q</strong></td><td>Exit Application</td></tr>
<tr><td><strong>Mouse Wheel</strong></td><td>Zoom in/out</td></tr>
<tr><td><strong>Click + Drag</strong></td><td>Pan view / Move nodes</td></tr>
</table>
</div>

<div class="section">
<h2>Troubleshooting</h2>

<div class="warning">
<strong>"Seeds not found in network"</strong><br>
Ensure seed names exactly match node names in your network file (case-sensitive).
</div>

<div class="warning">
<strong>"No connectors found"</strong><br>
Your seeds may not be connected in the network. Try different seeds or check network connectivity.
</div>

<div class="warning">
<strong>"Analysis is slow"</strong><br>
Reduce permutation count, or use fewer seeds. Large networks (>10,000 nodes) may require patience.
</div>

<div class="tip">
<strong>Performance Tip:</strong> For very large networks, disable "Enable Graph Visualization" during analysis, then enable it to view results.
</div>
</div>

<div class="section">
<h2>Use Cases</h2>

<h3>Biological Networks</h3>
<p>Find the minimal protein/gene pathway connecting disease-related genes. Identify essential mediators in signaling networks.</p>

<h3>Social Networks</h3>
<p>Discover key individuals connecting different communities. Identify information flow bottlenecks.</p>

<h3>Infrastructure Networks</h3>
<p>Find critical nodes in transportation or communication networks. Assess network vulnerability.</p>
</div>

<p style="margin-top: 40px; text-align: center; color: #64748b; font-size: 14px;">
BasicSpanner v1.0 | Developed for network analysis research
</p>

</body>
</html>
    )";
    
    userGuideText->setHtml(guideContent);
    userGuideScrollArea->setWidget(userGuideText);
    
    guideLayout->addWidget(userGuideScrollArea);
}

void MainWindow::setupMenus()
{
    QMenuBar* menuBar = this->menuBar();
    
    QMenu* fileMenu = menuBar->addMenu("File");
    actionLoadNetwork_ = fileMenu->addAction("Load Network...");
    actionLoadSeeds_ = fileMenu->addAction("Load Seeds...");
    fileMenu->addSeparator();
    
    recentNetworksSubmenu_ = recentFilesManager_->createRecentNetworksSubmenu(fileMenu);
    recentSeedsSubmenu_ = recentFilesManager_->createRecentSeedsSubmenu(fileMenu);
    fileMenu->addMenu(recentNetworksSubmenu_);
    fileMenu->addMenu(recentSeedsSubmenu_);
    
    fileMenu->addSeparator();
    actionSaveResults_ = fileMenu->addAction("Save Results...");
    actionExportNetwork_ = fileMenu->addAction("Export Network...");
    actionExportOriginalGraph_ = fileMenu->addAction("Export Original Graph...");
    fileMenu->addSeparator();
    actionExit_ = fileMenu->addAction("Exit");
    
    QMenu* editMenu = menuBar->addMenu("Edit");
    actionClearSeeds_ = editMenu->addAction("Clear Seeds");
    actionSelectAllNodes_ = editMenu->addAction("Select All Nodes as Seeds");
    
    QMenu* toolsMenu = menuBar->addMenu("Tools");
    actionRunAnalysis_ = toolsMenu->addAction("Run Analysis");
    toolsMenu->addSeparator();
    actionSettings_ = toolsMenu->addAction("Settings...");
    
    QMenu* helpMenu = menuBar->addMenu("Help");
    actionAbout_ = helpMenu->addAction("About");
    actionUserGuide_ = helpMenu->addAction("User Guide");
    
    QMenu* windowMenu = menuBar->addMenu("Window");
    
    actionToggleFileOperationsPanel_ = uiManager_->getFileOperationsDock()->toggleViewAction();
    actionToggleVisualizationPanel_ = uiManager_->getVisualizationControlDock()->toggleViewAction();
    actionToggleSeedSelectionPanel_ = uiManager_->getSeedSelectionDock()->toggleViewAction();
    actionToggleAlgorithmConfigPanel_ = uiManager_->getAlgorithmConfigDock()->toggleViewAction();
    actionToggleExecutionPanel_ = uiManager_->getExecutionDock()->toggleViewAction();
    actionToggleResultsPanel_ = uiManager_->getResultsDock()->toggleViewAction();
    actionToggleNodeListPanel_ = uiManager_->getNodeListDock()->toggleViewAction();

    windowMenu->addAction(actionToggleFileOperationsPanel_);
    windowMenu->addAction(actionToggleVisualizationPanel_);
    windowMenu->addAction(actionToggleSeedSelectionPanel_);
    windowMenu->addAction(actionToggleResultsPanel_);
    windowMenu->addSeparator();
    windowMenu->addAction(actionToggleAlgorithmConfigPanel_);
    windowMenu->addAction(actionToggleExecutionPanel_);
    windowMenu->addAction(actionToggleNodeListPanel_);
    
    windowMenu->addSeparator();
    QAction* actionResetLayout = windowMenu->addAction("Reset Layout");
    connect(actionResetLayout, &QAction::triggered, uiManager_, &UIManager::resetLayout);
    
    actionLoadNetwork_->setShortcut(QKeySequence::Open);
    actionSaveResults_->setShortcut(QKeySequence::Save);
    actionExit_->setShortcut(QKeySequence::Quit);
    actionRunAnalysis_->setShortcut(QKeySequence("F5"));
}

void MainWindow::setupConnections()
{
    
    connect(actionLoadNetwork_, &QAction::triggered, appController_, &AppController::onLoadNetworkRequested);
    connect(actionLoadSeeds_, &QAction::triggered, appController_, &AppController::onLoadSeedsRequested);
    connect(actionExit_, &QAction::triggered, this, &QWidget::close);
    connect(actionClearSeeds_, &QAction::triggered, appController_, &AppController::onClearSeedsRequested);
    connect(actionSelectAllNodes_, &QAction::triggered, appController_, &AppController::onSelectAllNodesRequested);
    connect(actionSettings_, &QAction::triggered, this, &MainWindow::showSettings);
    connect(actionAbout_, &QAction::triggered, this, &MainWindow::about);
    connect(actionUserGuide_, &QAction::triggered, this, &MainWindow::showUserGuide);
    connect(actionExportNetwork_, &QAction::triggered, appController_, &AppController::onExportNetworkRequested);
    connect(actionExportOriginalGraph_, &QAction::triggered, appController_, &AppController::onExportOriginalGraphRequested);
    
    connect(recentFilesManager_, &RecentFilesManager::recentNetworkSelected,
            appController_, &AppController::onLoadNetworkFileDirectly);
    connect(recentFilesManager_, &RecentFilesManager::recentSeedsSelected,
            appController_, &AppController::onLoadSeedsFileDirectly);
    
    connect(actionRunAnalysis_, &QAction::triggered, this, [this](){
        auto config = uiManager_->getAlgorithmConfigPanel()->getConfig();
        auto seeds = uiManager_->getSeedSelectionPanel()->getSeedNodes();
        appController_->onRunAnalysisRequested(config, seeds);
    });
    connect(actionSaveResults_, &QAction::triggered, this, [this](){
        QString resultsText = uiManager_->getResultsPanel()->getResultsText();
        appController_->onSaveResultsRequested(resultsText);
    });
    
    auto filePanel = uiManager_->getFileOperationsPanel();
    auto seedPanel = uiManager_->getSeedSelectionPanel();
    auto algorithmPanel = uiManager_->getAlgorithmConfigPanel();
    auto executionPanel = uiManager_->getExecutionPanel();
    auto resultsPanel = uiManager_->getResultsPanel();
    
    connect(filePanel, &FileOperationsPanel::loadNetworkRequested, 
            appController_, &AppController::onLoadNetworkRequested);
    
    connect(filePanel, &FileOperationsPanel::recentNetworksRequested, 
            this, [this]() {
        QMenu* recentMenu = recentFilesManager_->createRecentNetworksSubmenu(nullptr);
        recentMenu->exec(QCursor::pos());
        delete recentMenu;
    });
    
    connect(filePanel, &FileOperationsPanel::recentSeedsRequested, 
            this, [this]() {
        QMenu* recentMenu = recentFilesManager_->createRecentSeedsSubmenu(nullptr);
        recentMenu->exec(QCursor::pos());
        delete recentMenu;
    });
    
    connect(filePanel, &FileOperationsPanel::exportStatisticsRequested,
            appController_, &AppController::onExportStatisticsRequested);
    
    connect(filePanel, &FileOperationsPanel::applyFiltersRequested,
            appController_, &AppController::onApplyGraphFilter);
    connect(filePanel, &FileOperationsPanel::resetFiltersRequested,
            appController_, &AppController::onResetGraphFilter);
    
    connect(seedPanel, &SeedSelectionPanel::loadSeedsRequested,
            appController_, &AppController::onLoadSeedsRequested);
    
    connect(seedPanel, &SeedSelectionPanel::generateRandomSeedsRequested,
            appController_, &AppController::onGenerateRandomSeedsRequested);
    
    connect(seedPanel, &SeedSelectionPanel::batchesRequested,
            appController_, &AppController::onBatchesRequested);
    
    connect(seedPanel, &SeedSelectionPanel::seedsChanged, 
            graphVisualization_, &GraphVisualization::highlightSeedsByName);
    
    connect(executionPanel, &ExecutionPanel::runAnalysisRequested, this, [this](){
        auto config = uiManager_->getAlgorithmConfigPanel()->getConfig();
        auto seeds = uiManager_->getSeedSelectionPanel()->getSeedNodes();
        appController_->onRunAnalysisRequested(config, seeds);
    });
    
    connect(resultsPanel, &ResultsPanel::saveResultsRequested, this, [this](){
        QString resultsText = uiManager_->getResultsPanel()->getResultsText();
        appController_->onSaveResultsRequested(resultsText);
    });
    connect(resultsPanel, &ResultsPanel::exportNetworkRequested, 
            appController_, &AppController::onExportNetworkRequested);
    
    connect(appController_, &AppController::networkLoaded, 
            filePanel, &FileOperationsPanel::updateFileStatus);
    
    connect(appController_, &AppController::networkLoaded, 
            this, [this](const QString& fileName, const QString& filePath, int nodeCount, int edgeCount,
                        int rejectedSelfLoops, int isolatedNodes, int initialConnections, int nonUniqueInteractions) {
        Q_UNUSED(fileName);
        Q_UNUSED(nodeCount);
        Q_UNUSED(edgeCount);
        Q_UNUSED(rejectedSelfLoops);
        Q_UNUSED(isolatedNodes);
        Q_UNUSED(initialConnections);
        Q_UNUSED(nonUniqueInteractions);
        recentFilesManager_->addRecentNetwork(filePath);
    });
    
    connect(appController_, &AppController::seedsLoadedFromFile, 
            this, [this](const QString& seedContent, const QString& filePath) {
        recentFilesManager_->addRecentSeeds(filePath);
    });
    connect(appController_, &AppController::seedsLoaded,
            seedPanel, &SeedSelectionPanel::setSeedContent);
    connect(appController_, &AppController::allNodeNamesReady, this, [this, seedPanel](const std::vector<std::string>& nodeNames) {
        QStringList qNodeNames;
        for (const auto& name : nodeNames) {
            qNodeNames << QString::fromStdString(name);
        }
        seedPanel->setSeedContent(qNodeNames.join('\n'));
    });
    
    connect(appController_, &AppController::analysisStarted, 
            executionPanel, &ExecutionPanel::onAnalysisStarted);
    connect(appController_, &AppController::analysisStarted,
            this, [this]() {
        auto vizPanel = uiManager_->getVisualizationControlPanel();
        if (vizPanel) {
            vizPanel->setBatchCount(0);
            vizPanel->setPermutationCount(0);
        }
        perBatchConnectors_.clear();
        perBatchConnectorFrequency_.clear();
        numBatches_ = 0;
        allBatchesCombinedValid_ = false;
    });
    connect(appController_, &AppController::analysisStartedWithPermutations, 
            executionPanel, &ExecutionPanel::onAnalysisStartedWithPermutations);
    connect(appController_, &AppController::analysisFinished, 
            executionPanel, &ExecutionPanel::onAnalysisFinished);
    connect(appController_, &AppController::progressUpdate, 
            executionPanel, &ExecutionPanel::updateProgress);
    connect(appController_, &AppController::detailedProgressUpdate, 
            executionPanel, &ExecutionPanel::updateDetailedProgress);
    connect(appController_, &AppController::batchChanged, 
            executionPanel, &ExecutionPanel::onBatchChanged);
    connect(appController_, &AppController::permutationCompletedWithSeeds, 
            executionPanel, &ExecutionPanel::onPermutationCompletedWithSeeds);
    connect(appController_, &AppController::permutationCompleted, 
            executionPanel, &ExecutionPanel::onPermutationCompleted);
    
    connect(executionPanel, &ExecutionPanel::stopAnalysisRequested,
            appController_, &AppController::onStopAnalysisRequested);
    
    connect(executionPanel, &ExecutionPanel::realTimeControlRequested,
            this, &MainWindow::onRealTimeControlRequested);
    
    connect(appController_, &AppController::resultsReady, 
            resultsPanel, &ResultsPanel::displayResults);
    
    connect(appController_, &AppController::graphReadyForDisplay,
            graphVisualization_, &GraphVisualization::displayGraph);
    
    connect(appController_, &AppController::graphReadyForDisplay,
            this, [filePanel](const Graph& graph) {
        int maxDegree = 0;
        for (int nodeId : graph.getNodes()) {
            int degree = static_cast<int>(graph.getNeighbors(nodeId).size());
            if (degree > maxDegree) {
                maxDegree = degree;
            }
        }
        filePanel->updateGraphFilterStats(maxDegree, graph.getNodeCount(), graph.getEdgeCount());
    });
    
    connect(appController_, &AppController::graphReadyForDisplay,
            this, [this, seedPanel](const Graph&) {
        QTimer::singleShot(100, [this, seedPanel]() {
            auto seeds = seedPanel->getSeedNodes();
            if (!seeds.empty()) {
                graphVisualization_->highlightSeedsByName(seeds);
            }
        });
    });
    connect(appController_, &AppController::basicNetworkReady,
            this, [this](const BasicNetworkResult& result) {
        hasAnalysisResult_ = true;
        lastAnalysisResult_ = result;
        auto visualizationPanel = uiManager_->getVisualizationControlPanel();
        if (visualizationPanel) {
            visualizationPanel->setResultsAvailable(true);
            int permCount = appController_->getPermutationCount(-1);
            visualizationPanel->setPermutationCount(permCount);
        }
        QTimer::singleShot(10, this, [this]() {
            const BasicNetworkResult* batchResult = appController_->getBatchResult(0);
            if (batchResult) {
                visualizeBasicNetwork(*batchResult);
            } else if (lastAnalysisResult_.isValid) {
                visualizeBasicNetwork(lastAnalysisResult_);
            }
        });
    });
    
    connect(appController_, &AppController::connectorFrequencyReady,
            this, [this](const std::map<int, int>& globalFrequency, int totalBatches) {
        connectorFrequency_ = globalFrequency;
        totalPermutations_ = totalBatches;
        
        perBatchConnectors_.clear();
        perBatchConnectorFrequency_.clear();
        numBatches_ = 0;
        
        generalBetweenness_.clear();
        generalBetweennessComputed_ = false;
        allBatchesCombinedValid_ = false;
    });
    connect(appController_, &AppController::showInfoMessage, 
            this, &MainWindow::showInfoMessage);
    connect(appController_, &AppController::showErrorMessage, 
            this, &MainWindow::showErrorMessage);
    
    connect(appController_, &AppController::networkLoaded, this, [this, seedPanel, algorithmPanel, executionPanel](){
        seedPanel->setEnabled(true);
        algorithmPanel->setEnabled(true);
        executionPanel->setEnabled(true);
        visualizeGraph();
    });
    
    
    auto visualizationPanel = uiManager_->getVisualizationControlPanel();
    connect(visualizationPanel, &VisualizationControlPanel::layoutAlgorithmChanged,
            this, [this](int layoutType) {
                graphVisualization_->setLayoutType(static_cast<LayoutType>(layoutType));
            });
    connect(visualizationPanel, &VisualizationControlPanel::visualizationEnabledChanged,
            this, &MainWindow::onEnableVisualizationToggled);
    connect(visualizationPanel, &VisualizationControlPanel::applyLayoutRequested,
            this, [this, visualizationPanel]() {
                if (graphVisualization_) {
                    LayoutParameters params = visualizationPanel->getLayoutParameters();
                    
                    if (!connectorFrequency_.empty() && totalPermutations_ > 0) {
                        graphVisualization_->setConnectorFrequencyData(connectorFrequency_, totalPermutations_);
                    }

                    graphVisualization_->setLayoutParameters(params);
                    graphVisualization_->visualizeGraph();

                    if (hasAnalysisResult_) {
                        applyConnectorHighlighting();
                    }
                }
            });
    
    connect(visualizationPanel, &VisualizationControlPanel::viewModeChanged,
            this, [this](ViewMode mode) {
                onViewModeChanged(static_cast<int>(mode));
            });
    
    connect(visualizationPanel, &VisualizationControlPanel::frequencyHighlightChanged,
            this, &MainWindow::onFrequencyHighlightToggled);
    
    connect(visualizationPanel, &VisualizationControlPanel::batchSelectionChanged,
            this, &MainWindow::onBatchSelectionChanged);
    
    connect(visualizationPanel, &VisualizationControlPanel::permutationSelectionChanged,
            this, &MainWindow::onPermutationSelectionChanged);
    
    connect(appController_, &AppController::perBatchConnectorsReady,
            this, [this, visualizationPanel](const std::vector<std::set<int>>& perBatchConnectors,
                                             const std::vector<std::map<int, int>>& perBatchFrequency,
                                             int numBatches) {
        perBatchConnectors_ = perBatchConnectors;
        perBatchConnectorFrequency_ = perBatchFrequency;
        numBatches_ = numBatches;
        if (visualizationPanel) {
            visualizationPanel->setBatchCount(numBatches);
            int permCount = appController_->getPermutationCount(0);
            visualizationPanel->setPermutationCount(permCount);
        }
    });
    
    connect(visualizationPanel, &VisualizationControlPanel::nodeColorAttributeChanged,
            this, [this](int attributeIndex) {
                if (graphVisualization_) {
                    if (attributeIndex == 0) {
                        colorLegend_->setVisible(false);
                        graphVisualization_->colorByAttribute(attributeIndex);
                        
                        if (hasAnalysisResult_) {
                            applyConnectorHighlighting();
                        }
                    } else {
                        graphVisualization_->colorByAttribute(attributeIndex);
                    }
                }
            });
    
    connect(graphVisualization_, &GraphVisualization::attributeColoringApplied,
            this, &MainWindow::updateColorLegend);
    
    connect(graphVisualization_, &GraphVisualization::nodeClicked,
            appController_, &AppController::onNodeClicked);
    
    auto nodeListPanel = uiManager_->getNodeListPanel();
    
    connect(nodeListPanel, &NodeListPanel::nodeHovered,
            graphVisualization_, &GraphVisualization::highlightNodeTemporary);
    
    connect(nodeListPanel, &NodeListPanel::nodeUnhovered,
            graphVisualization_, &GraphVisualization::clearTemporaryHighlight);
    
    connect(appController_, &AppController::nodeListReady,
            this, [this, nodeListPanel](const std::vector<std::string>& nodeNames) {
                const Graph& graph = appController_->getCurrentGraph();
                
                std::map<std::string, int> degrees;
                for (const auto& name : nodeNames) {
                    int nodeId = graph.getNodeId(name);
                    if (nodeId >= 0) {
                        degrees[name] = static_cast<int>(graph.getNeighbors(nodeId).size());
                    }
                }
                
                nodeListPanel->setNodes(nodeNames);
                nodeListPanel->updateDegrees(degrees);
            });
    
    connect(appController_, &AppController::connectorFrequencyReady,
            this, [this, nodeListPanel, seedPanel](const std::map<int, int>& freq, int totalPerms) {
                Q_UNUSED(freq);
                
                std::map<std::string, QString> nodeTypes;
                const Graph& graph = appController_->getCurrentGraph();
                
                auto seeds = seedPanel->getSeedNodes();
                for (const auto& seed : seeds) {
                    nodeTypes[seed] = "Seed";
                }
                
                const BasicNetworkResult* bestResult = appController_->getBatchResult(0);
                if (bestResult) {
                    for (int connId : bestResult->connectorNodes) {
                        std::string name = graph.getNodeName(connId);
                        if (!name.empty() && nodeTypes.find(name) == nodeTypes.end()) {
                            nodeTypes[name] = "Connector";
                        }
                    }
                }
                
                for (const auto& [connId, count] : connectorFrequency_) {
                    std::string name = graph.getNodeName(connId);
                    if (!name.empty() && nodeTypes.find(name) == nodeTypes.end()) {
                        nodeTypes[name] = "Extra Connector";
                    }
                }
                
                nodeListPanel->updateNodeTypes(nodeTypes);
                
                if (totalPerms > 0) {
                    std::map<std::string, double> frequencies;
                    for (const auto& [connId, count] : connectorFrequency_) {
                        std::string name = graph.getNodeName(connId);
                        if (!name.empty()) {
                            frequencies[name] = (static_cast<double>(count) / totalPerms) * 100.0;
                        }
                    }
                    nodeListPanel->updateFrequencies(frequencies);
                }
            });
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About BasicSpanner"),
        tr("BasicSpanner v1.0\n\n"
           "A desktop application for the rapid analysis of networks\n"
           "through extreme graph simplification.\n\n"
           "Given a set of seed nodes, BasicSpanner computes the basic\n"
           "network: the minimal set of additional nodes (connectors)\n"
           "required to preserve every pairwise distance among the seeds\n"
           "of the original graph.\n\n"
           "Reference:\n"
           "Marin J, Marin I. Extreme graph simplification applied to\n"
           "functional discovery in biological networks (2026)."));
}

void MainWindow::showUserGuide()
{
    tabWidget_->setCurrentIndex(1);
}

void MainWindow::showSettings()
{
    SettingsDialog dialog(graphVisualization_, this);
    dialog.exec();
}

void MainWindow::showInfoMessage(const QString& message)
{
    QMessageBox::information(this, "Information", message);
}

void MainWindow::showErrorMessage(const QString& message)
{
    QMessageBox::critical(this, "Error", message);
}

void MainWindow::visualizeGraph()
{
    qDebug() << "[MainWindow DEBUG] visualizeGraph() called - UI refresh requested";
    
    if (graphVisualization_ && appController_) {
        const Graph& currentGraph = appController_->getCurrentGraph();
        if (currentGraph.getNodeCount() > 0) {
            graphVisualization_->displayGraph(currentGraph);
            qDebug() << "[MainWindow DEBUG] Graph visualization refresh completed with" 
                     << currentGraph.getNodeCount() << "nodes";
        } else {
            qDebug() << "[MainWindow DEBUG] No graph loaded yet";
        }
    }
}

void MainWindow::visualizeBasicNetwork(const BasicNetworkResult& result)
{
    if (!graphVisualization_) {
        return;
    }
    
    if (!result.isValid || !result.basicNetwork_ptr) {
        return;
    }
    
    hasAnalysisResult_ = true;
    
    auto visualizationPanel = uiManager_->getVisualizationControlPanel();
    if (visualizationPanel) {
        visualizationPanel->setResultsAvailable(true);
    }
    
    QApplication::processEvents();
    
    clearVisualization();
    
    try {
        ViewMode viewMode = ViewMode::BasicNetwork;
        if (visualizationPanel) {
            viewMode = visualizationPanel->getViewMode();
        }
        
        const Graph& originalGraph = appController_->getFinder().getGraph();
        
        int selectedBatch = visualizationPanel ? visualizationPanel->getSelectedBatch() : -1;
        bool usingPerBatch = (selectedBatch >= 0 && selectedBatch < static_cast<int>(perBatchConnectorFrequency_.size()));
        const std::map<int, int>& activeFrequency = usingPerBatch
            ? perBatchConnectorFrequency_[selectedBatch]
            : connectorFrequency_;
        int activeTotalPermutations = usingPerBatch ? 1 : totalPermutations_;
        
        bool useFrequencyHighlight = visualizationPanel && 
                                     visualizationPanel->isFrequencyHighlightEnabled() && 
                                     !activeFrequency.empty();
        
        if (viewMode == ViewMode::FullGraph) {
            graphVisualization_->setGraph(originalGraph);
            
            std::set<int> seedIds(result.seedNodes.begin(), result.seedNodes.end());
            
            NodeAppearance appearance = graphVisualization_->getNodeAppearance();
            if (!seedIds.empty()) {
                graphVisualization_->highlightNodes(seedIds, appearance.seedColor);
            }
            
            if (useFrequencyHighlight) {
                std::set<int> bestConnectorIds(result.connectorNodes.begin(), result.connectorNodes.end());
                
                int extraCount = 0;
                for (const auto& [id, count] : activeFrequency) {
                    if (bestConnectorIds.find(id) == bestConnectorIds.end()) {
                        extraCount++;
                    }
                }
                
                graphVisualization_->highlightConnectorsWithFrequency(
                    activeFrequency, activeTotalPermutations, bestConnectorIds,
                    appearance.connectorColor, appearance.extraConnectorColor);
            } else {
                std::set<int> connectorIds(result.connectorNodes.begin(), result.connectorNodes.end());
                if (!connectorIds.empty()) {
                    graphVisualization_->highlightNodes(connectorIds, appearance.connectorColor);
                }
            }
        } else {
            graphVisualization_->setGraph(*result.basicNetwork_ptr);
            
            const Graph& displayedGraph = *result.basicNetwork_ptr;
            
            std::set<int> seedIdsInNewGraph;
            for (int originalSeedId : result.seedNodes) {
                if (originalGraph.hasNode(originalSeedId)) {
                    std::string nodeName = originalGraph.getNodeName(originalSeedId);
                    int newId = displayedGraph.getNodeId(nodeName);
                    if (newId != -1) {
                        seedIdsInNewGraph.insert(newId);
                    }
                }
            }
            
            NodeAppearance appearance = graphVisualization_->getNodeAppearance();
            if (!seedIdsInNewGraph.empty()) {
                graphVisualization_->highlightNodes(seedIdsInNewGraph, appearance.seedColor);
            }
            
            std::set<int> bestConnectorIdsInNewGraph;
            for (int originalConnectorId : result.connectorNodes) {
                if (originalGraph.hasNode(originalConnectorId)) {
                    std::string nodeName = originalGraph.getNodeName(originalConnectorId);
                    int newId = displayedGraph.getNodeId(nodeName);
                    if (newId != -1) {
                        bestConnectorIdsInNewGraph.insert(newId);
                    }
                }
            }
            
            if (useFrequencyHighlight) {
                std::map<int, int> connectorFrequencyInNewGraph;
                for (const auto& [originalId, count] : activeFrequency) {
                    if (originalGraph.hasNode(originalId)) {
                        std::string nodeName = originalGraph.getNodeName(originalId);
                        int newId = displayedGraph.getNodeId(nodeName);
                        if (newId != -1) {
                            connectorFrequencyInNewGraph[newId] = count;
                        }
                    }
                }
                if (!connectorFrequencyInNewGraph.empty()) {
                    graphVisualization_->highlightConnectorsWithFrequency(
                        connectorFrequencyInNewGraph, activeTotalPermutations, bestConnectorIdsInNewGraph,
                        appearance.connectorColor, appearance.extraConnectorColor);
                }
            } else {
                if (!bestConnectorIdsInNewGraph.empty()) {
                    graphVisualization_->highlightNodes(bestConnectorIdsInNewGraph, appearance.connectorColor);
                }
            }
        }
        
        updateNodeListForCurrentView(result);
        
    } catch (...) {
    }
}

void MainWindow::onViewModeChanged(int mode)
{
    if (!hasAnalysisResult_ || !graphVisualization_) {
        return;
    }

    auto visualizationPanel = uiManager_->getVisualizationControlPanel();
    int selectedBatch = visualizationPanel ? visualizationPanel->getSelectedBatch() : -1;
    
    const BasicNetworkResult* batchResult = appController_->getBatchResult(selectedBatch);
    if (batchResult) {
        visualizeBasicNetwork(*batchResult);
    } else if (numBatches_ > 1) {
        buildAllBatchesCombinedResult();
        if (allBatchesCombinedValid_) {
            visualizeBasicNetwork(allBatchesCombinedResult_);
        } else {
            visualizeBasicNetwork(lastAnalysisResult_);
        }
    } else {
        visualizeBasicNetwork(lastAnalysisResult_);
    }
}

void MainWindow::onFrequencyHighlightToggled(bool enabled)
{
    Q_UNUSED(enabled);
    if (!hasAnalysisResult_ || !graphVisualization_) {
        return;
    }
    
    auto visualizationPanel = uiManager_->getVisualizationControlPanel();
    int selectedBatch = visualizationPanel ? visualizationPanel->getSelectedBatch() : -1;
    
    const BasicNetworkResult* batchResult = appController_->getBatchResult(selectedBatch);
    if (batchResult) {
        visualizeBasicNetwork(*batchResult);
    } else if (numBatches_ > 1) {
        buildAllBatchesCombinedResult();
        if (allBatchesCombinedValid_) {
            visualizeBasicNetwork(allBatchesCombinedResult_);
        } else {
            visualizeBasicNetwork(lastAnalysisResult_);
        }
    } else {
        visualizeBasicNetwork(lastAnalysisResult_);
    }
}

void MainWindow::onBatchSelectionChanged(int batchIndex)
{
    if (!hasAnalysisResult_ || !graphVisualization_) {
        return;
    }
    
    auto visualizationPanel = uiManager_->getVisualizationControlPanel();
    if (visualizationPanel) {
        int permCount = appController_->getPermutationCount(batchIndex);
        visualizationPanel->setPermutationCount(permCount);
    }
    
    const BasicNetworkResult* batchResult = appController_->getBatchResult(batchIndex);
    if (batchResult) {
        visualizeBasicNetwork(*batchResult);
    } else if (numBatches_ > 1) {
        buildAllBatchesCombinedResult();
        if (allBatchesCombinedValid_) {
            visualizeBasicNetwork(allBatchesCombinedResult_);
        } else {
            visualizeBasicNetwork(lastAnalysisResult_);
        }
    } else {
        visualizeBasicNetwork(lastAnalysisResult_);
    }
}

void MainWindow::onPermutationSelectionChanged(int permutationIndex)
{
    if (!hasAnalysisResult_ || !graphVisualization_) {
        return;
    }
    
    auto visualizationPanel = uiManager_->getVisualizationControlPanel();
    int selectedBatch = visualizationPanel ? visualizationPanel->getSelectedBatch() : -1;
    
    if (permutationIndex >= 0) {
        const BasicNetworkResult* permResult = appController_->getPermutationResult(selectedBatch, permutationIndex);
        if (permResult) {
            visualizeBasicNetwork(*permResult);
            return;
        }
    }
    
    const BasicNetworkResult* batchResult = appController_->getBatchResult(selectedBatch);
    if (batchResult) {
        visualizeBasicNetwork(*batchResult);
    } else if (numBatches_ > 1) {
        buildAllBatchesCombinedResult();
        if (allBatchesCombinedValid_) {
            visualizeBasicNetwork(allBatchesCombinedResult_);
        } else {
            visualizeBasicNetwork(lastAnalysisResult_);
        }
    } else {
        visualizeBasicNetwork(lastAnalysisResult_);
    }
}

void MainWindow::applyConnectorHighlighting()
{
    if (!hasAnalysisResult_ || !graphVisualization_) {
        return;
    }
    
    auto visualizationPanel = uiManager_->getVisualizationControlPanel();
    if (!visualizationPanel) {
        return;
    }
    
    const Graph& originalGraph = appController_->getFinder().getGraph();
    
    int selectedBatch = visualizationPanel->getSelectedBatch();
    const BasicNetworkResult* batchResult = appController_->getBatchResult(selectedBatch);
    const BasicNetworkResult& result = batchResult ? *batchResult : lastAnalysisResult_;
    
    const std::map<int, int>& activeFrequency = 
        (selectedBatch >= 0 && selectedBatch < static_cast<int>(perBatchConnectorFrequency_.size()))
        ? perBatchConnectorFrequency_[selectedBatch]
        : connectorFrequency_;
    int activeTotalPermutations = batchResult ? 1 : totalPermutations_;
    
    ViewMode viewMode = visualizationPanel->getViewMode();
    bool useFrequencyHighlight = visualizationPanel->isFrequencyHighlightEnabled() && 
                                  !activeFrequency.empty();
    
    NodeAppearance appearance = graphVisualization_->getNodeAppearance();
    
    std::vector<std::string> seedNames;
    for (int seedId : result.seedNodes) {
        if (originalGraph.hasNode(seedId)) {
            seedNames.push_back(originalGraph.getNodeName(seedId));
        }
    }
    if (!seedNames.empty()) {
        graphVisualization_->highlightSeedsByName(seedNames);
    }
    
    if (viewMode == ViewMode::FullGraph) {
        std::set<int> bestConnectorIds(result.connectorNodes.begin(), result.connectorNodes.end());
        
        if (useFrequencyHighlight) {
            graphVisualization_->highlightConnectorsWithFrequency(
                activeFrequency, activeTotalPermutations, bestConnectorIds,
                appearance.connectorColor, appearance.extraConnectorColor);
        } else {
            if (!bestConnectorIds.empty()) {
                graphVisualization_->highlightNodes(bestConnectorIds, appearance.connectorColor);
            }
        }
    } else {
        const Graph& displayedGraph = *result.basicNetwork_ptr;
        
        std::set<int> bestConnectorIdsInNewGraph;
        for (int originalConnectorId : result.connectorNodes) {
            if (originalGraph.hasNode(originalConnectorId)) {
                std::string nodeName = originalGraph.getNodeName(originalConnectorId);
                int newId = displayedGraph.getNodeId(nodeName);
                if (newId != -1) {
                    bestConnectorIdsInNewGraph.insert(newId);
                }
            }
        }
        
        if (useFrequencyHighlight) {
            std::map<int, int> connectorFrequencyInNewGraph;
            for (const auto& [originalId, count] : activeFrequency) {
                if (originalGraph.hasNode(originalId)) {
                    std::string nodeName = originalGraph.getNodeName(originalId);
                    int newId = displayedGraph.getNodeId(nodeName);
                    if (newId != -1) {
                        connectorFrequencyInNewGraph[newId] = count;
                    }
                }
            }
            if (!connectorFrequencyInNewGraph.empty()) {
                graphVisualization_->highlightConnectorsWithFrequency(
                    connectorFrequencyInNewGraph, activeTotalPermutations, bestConnectorIdsInNewGraph,
                    appearance.connectorColor, appearance.extraConnectorColor);
            }
        } else {
            if (!bestConnectorIdsInNewGraph.empty()) {
                graphVisualization_->highlightNodes(bestConnectorIdsInNewGraph, appearance.connectorColor);
            }
        }
    }
}

void MainWindow::clearVisualization()
{
    
    if (graphVisualization_) {
        try {
            graphVisualization_->setCurrentSeeds({});
            graphVisualization_->setGraph(Graph());
            graphVisualization_->clearHighlights();
            graphVisualization_->update();
        } catch (...) {
        }
    } else {
    }
}

void MainWindow::onEnableVisualizationToggled(bool checked)
{
    if (graphVisualization_) {
        graphVisualization_->setVisualizationEnabled(checked);
        if (checked) {
            visualizeGraph();
        }
    }
}

void MainWindow::updateWindowMenuStates()
{
}

void MainWindow::onRealTimeControlRequested()
{
    if (!realTimeHistogramWindow_) {
        try {
            realTimeHistogramWindow_ = new RealTimeHistogramWindow(this);
            
            if (realTimeHistogramWindow_) {
                connect(realTimeHistogramWindow_, &RealTimeHistogramWindow::windowClosed,
                        this, &MainWindow::onRealTimeHistogramWindowClosed);
                
                connect(appController_, &AppController::permutationCompletedWithSeeds,
                        realTimeHistogramWindow_, &RealTimeHistogramWindow::addConnectorDataWithSeeds);
                connect(appController_, &AppController::analysisStartedWithPermutations,
                        realTimeHistogramWindow_, &RealTimeHistogramWindow::setPermutationsPerBatch);
                
                connect(appController_, &AppController::analysisStarted,
                        realTimeHistogramWindow_, &RealTimeHistogramWindow::clearData);

            }
        } catch (...) {
            QMessageBox::warning(this, "Error", "Failed to create Real-Time Control window.");
            return;
        }
    }
    
    if (realTimeHistogramWindow_) {
        try {
            realTimeHistogramWindow_->show();
            realTimeHistogramWindow_->raise();
            realTimeHistogramWindow_->activateWindow();
        } catch (...) {
            QMessageBox::warning(this, "Error", "Failed to show Real-Time Control window.");
        }
    }
}

void MainWindow::onRealTimeHistogramWindowClosed()
{
    
    if (realTimeHistogramWindow_) {
        try {
            disconnect(appController_, &AppController::permutationCompletedWithSeeds,
                      realTimeHistogramWindow_, &RealTimeHistogramWindow::addConnectorDataWithSeeds);
            disconnect(appController_, &AppController::analysisStartedWithPermutations,
                      realTimeHistogramWindow_, &RealTimeHistogramWindow::setPermutationsPerBatch);
            disconnect(appController_, &AppController::analysisStarted,
                      realTimeHistogramWindow_, &RealTimeHistogramWindow::clearData);
        } catch (...) {
        }
        
        realTimeHistogramWindow_->deleteLater();
        realTimeHistogramWindow_ = nullptr;
        
    }
}

void MainWindow::updateColorLegend(const QString& attributeName, double minVal, double maxVal)
{
    if (!colorLegend_) return;
    
    QLabel* minLabel = colorLegend_->findChild<QLabel*>("legendMinLabel");
    if (minLabel) {
        minLabel->setText(QString::number(minVal, 'f', 4));
    }
    
    QLabel* maxLabel = colorLegend_->findChild<QLabel*>("legendMaxLabel");
    if (maxLabel) {
        maxLabel->setText(QString::number(maxVal, 'f', 4));
    }
    
    QLabel* attrLabel = colorLegend_->findChild<QLabel*>("legendAttrLabel");
    if (attrLabel) {
        attrLabel->setText(attributeName);
    }
    
    colorLegend_->setVisible(true);
}

void MainWindow::updateNodeListForCurrentView(const BasicNetworkResult& result)
{
    auto nodeListPanel = uiManager_->getNodeListPanel();
    if (!nodeListPanel) return;

    auto visualizationPanel = uiManager_->getVisualizationControlPanel();
    const Graph& originalGraph = appController_->getFinder().getGraph();

    int selectedBatch = visualizationPanel ? visualizationPanel->getSelectedBatch() : -1;
    bool usingPerBatch = (selectedBatch >= 0 && selectedBatch < static_cast<int>(perBatchConnectorFrequency_.size()));
    const std::map<int, int>& activeFrequency = usingPerBatch
        ? perBatchConnectorFrequency_[selectedBatch]
        : connectorFrequency_;
    int activeTotalPermutations = usingPerBatch ? 1 : totalPermutations_;

    ViewMode viewMode = visualizationPanel ? visualizationPanel->getViewMode() : ViewMode::BasicNetwork;

    const Graph* basicNetGraph = result.basicNetwork_ptr.get();
    
    std::vector<NodeData> nodeDataList;
    std::set<int> seedSet(result.seedNodes.begin(), result.seedNodes.end());
    std::set<int> connectorSet(result.connectorNodes.begin(), result.connectorNodes.end());
    
    if (viewMode == ViewMode::FullGraph) {
        nodeDataList.reserve(originalGraph.getNodeCount());
        for (int nodeId : originalGraph.getNodes()) {
            NodeData nd;
            nd.name = originalGraph.getNodeName(nodeId);
            nd.degreeGeneral = originalGraph.getNeighbors(nodeId).size();
            
            if (basicNetGraph) {
                std::string nodeName = originalGraph.getNodeName(nodeId);
                int bnId = basicNetGraph->getNodeId(nodeName);
                if (bnId >= 0) {
                    nd.degreeLocal = basicNetGraph->getNeighbors(bnId).size();
                }
            }
            
            if (seedSet.count(nodeId)) {
                nd.type = "Seed";
            } else if (connectorSet.count(nodeId)) {
                nd.type = "Connector";
            } else if (activeFrequency.count(nodeId)) {
                nd.type = "Extra Connector";
            } else {
                nd.type = "Regular";
            }
            
            auto freqIt = activeFrequency.find(nodeId);
            if (freqIt != activeFrequency.end() && activeTotalPermutations > 0) {
                nd.frequency = (static_cast<double>(freqIt->second) / activeTotalPermutations) * 100.0;
            }
            
            nodeDataList.push_back(std::move(nd));
        }
    } else {
        if (!basicNetGraph) return;
        const Graph& displayedGraph = *basicNetGraph;
        
        nodeDataList.reserve(displayedGraph.getNodeCount());
        for (int newId : displayedGraph.getNodes()) {
            std::string nodeName = displayedGraph.getNodeName(newId);
            int originalId = originalGraph.getNodeId(nodeName);
            
            NodeData nd;
            nd.name = nodeName;
            nd.degreeLocal = displayedGraph.getNeighbors(newId).size();
            
            if (originalId >= 0) {
                nd.degreeGeneral = originalGraph.getNeighbors(originalId).size();
            }
            
            if (originalId >= 0 && seedSet.count(originalId)) {
                nd.type = "Seed";
            } else if (originalId >= 0 && connectorSet.count(originalId)) {
                nd.type = "Connector";
            } else if (originalId >= 0 && activeFrequency.count(originalId)) {
                nd.type = "Extra Connector";
            } else {
                nd.type = "Regular";
            }
            
            if (originalId >= 0) {
                auto freqIt = activeFrequency.find(originalId);
                if (freqIt != activeFrequency.end() && activeTotalPermutations > 0) {
                    nd.frequency = (static_cast<double>(freqIt->second) / activeTotalPermutations) * 100.0;
                }
            }
            
            nodeDataList.push_back(std::move(nd));
        }
    }
    
    nodeListPanel->setNodeData(nodeDataList);
}

void MainWindow::buildAllBatchesCombinedResult()
{
    int batchCount = appController_->getBatchCount();
    if (allBatchesCombinedValid_ || batchCount <= 1) return;
    
    const Graph& originalGraph = appController_->getFinder().getGraph();
    
    std::set<int> allSeeds;
    std::set<int> allConnectors;
    
    for (int i = 0; i < batchCount; ++i) {
        const BasicNetworkResult* batchResult = appController_->getBatchResult(i);
        if (!batchResult || !batchResult->isValid) continue;
        allSeeds.insert(batchResult->seedNodes.begin(), batchResult->seedNodes.end());
        allConnectors.insert(batchResult->connectorNodes.begin(), batchResult->connectorNodes.end());
    }
    
    std::set<int> allUnits;
    allUnits.insert(allSeeds.begin(), allSeeds.end());
    allUnits.insert(allConnectors.begin(), allConnectors.end());
    
    allBatchesCombinedResult_ = BasicNetworkResult();
    allBatchesCombinedResult_.isValid = true;
    allBatchesCombinedResult_.seedNodes = allSeeds;
    allBatchesCombinedResult_.connectorNodes = allConnectors;
    allBatchesCombinedResult_.allBasicUnits = allUnits;
    allBatchesCombinedResult_.totalConnectors = static_cast<int>(allConnectors.size());
    allBatchesCombinedResult_.basicNetwork_ptr = std::make_unique<Graph>(originalGraph.getSubgraph(allUnits));
    
    allBatchesCombinedValid_ = true;
}
