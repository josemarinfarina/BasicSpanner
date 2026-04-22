/**
 * @file MainWindow.h
 * @brief Main application window for BasicSpanner
 * 
 * This header defines the MainWindow class which provides the main user interface
 * for the BasicSpanner application. The MainWindow now acts as a pure container
 * and coordinator, delegating UI management to specialized manager classes.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <unordered_map>
#include <vector>
#include <map>

#include "GraphVisualization.h"
#include "SettingsDialog.h"
#include "AppController.h"
#include "UIManager.h"
#include "RecentFilesManager.h"
#include "RealTimeHistogramWindow.h"

/**
 * @brief Main application window
 * 
 * The MainWindow class provides the primary user interface container for the
 * BasicSpanner application. It acts as a pure coordinator, delegating UI 
 * management to specialized manager classes and business logic to the AppController.
 * 
 * Architecture:
 * - UIManager: Handles all dockable panels
 * - LargeNetworksManager: Handles large networks tab
 * - AppController: Handles all business logic
 * - MainWindow: Pure container and signal/slot coordinator
 * 
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent Parent widget
     */
    MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MainWindow();

private slots:
    void about();
    void showUserGuide();
    void showSettings();
    
    void showInfoMessage(const QString& message);
    void showErrorMessage(const QString& message);
    void visualizeBasicNetwork(const BasicNetworkResult& result);
    void onEnableVisualizationToggled(bool checked);
    void onViewModeChanged(int mode);
    void onFrequencyHighlightToggled(bool enabled);
    void onBatchSelectionChanged(int batchIndex);
    void onPermutationSelectionChanged(int permutationIndex);
    void applyConnectorHighlighting();
    void updateColorLegend(const QString& attributeName, double minVal, double maxVal);
    
    void onRealTimeControlRequested();
    void onRealTimeHistogramWindowClosed();
    
    void updateWindowMenuStates();

private:
    void setupUI();
    void setupMenus();
    void setupConnections();
    void setupUserGuideTab();
    
    void visualizeGraph();
    void clearVisualization();
    void updateNodeListForCurrentView(const BasicNetworkResult& result);
    void buildAllBatchesCombinedResult();
    
    AppController* appController_;
    UIManager* uiManager_;
    RecentFilesManager* recentFilesManager_;
    
    RealTimeHistogramWindow* realTimeHistogramWindow_;
    
    QGraphicsScene* graphScene_;
    GraphVisualization* graphVisualization_;
    QWidget* viewerContainer_;
    QWidget* colorLegend_;
    std::unordered_map<int, QGraphicsEllipseItem*> nodeItems_;
    std::vector<QGraphicsLineItem*> edgeItems_;
    
    BasicNetworkResult lastAnalysisResult_;
    bool hasAnalysisResult_ = false;
    
    std::map<int, int> connectorFrequency_;
    int totalPermutations_ = 0;
    
    std::unordered_map<int, double> generalBetweenness_;
    bool generalBetweennessComputed_ = false;
    
    BasicNetworkResult allBatchesCombinedResult_;
    bool allBatchesCombinedValid_ = false;
    
    std::vector<std::set<int>> perBatchConnectors_;
    std::vector<std::map<int, int>> perBatchConnectorFrequency_;
    int numBatches_ = 0;
    
    QTabWidget* tabWidget_;
    QWidget* userGuideTab_;
    
    QAction* actionLoadNetwork_;
    QAction* actionLoadSeeds_;
    QAction* actionSaveResults_;
    QAction* actionExportNetwork_;
    QAction* actionExportOriginalGraph_;
    QAction* actionExit_;
    QAction* actionClearSeeds_;
    QAction* actionSelectAllNodes_;
    QAction* actionRunAnalysis_;
    QAction* actionSettings_;
    QAction* actionAbout_;
    QAction* actionUserGuide_;
    
    QAction* actionToggleFileOperationsPanel_;
    QAction* actionToggleVisualizationPanel_;
    QAction* actionToggleSeedSelectionPanel_;
    QAction* actionToggleAlgorithmConfigPanel_;
    QAction* actionToggleExecutionPanel_;
    QAction* actionToggleResultsPanel_;
    QAction* actionToggleNodeListPanel_;

    QMenu* recentNetworksSubmenu_;
    QMenu* recentSeedsSubmenu_;
};

#endif