#include "UIManager.h"
#include <QMainWindow>
#include <QDockWidget>

#include "panels/SeedSelectionPanel.h"
#include "panels/FileOperationsPanel.h"
#include "panels/AlgorithmConfigPanel.h"
#include "panels/ExecutionPanel.h"
#include "panels/ResultsPanel.h"
#include "panels/VisualizationControlPanel.h"
#include "panels/NodeListPanel.h"

UIManager::UIManager(QMainWindow* parent)
    : QObject(parent)
    , mainWindow_(parent)
    , seedSelectionPanel_(nullptr)
    , fileOperationsPanel_(nullptr)
    , algorithmConfigPanel_(nullptr)
    , executionPanel_(nullptr)
    , resultsPanel_(nullptr)
    , visualizationControlPanel_(nullptr)
    , nodeListPanel_(nullptr)
    , seedSelectionDock_(nullptr)
    , fileOperationsDock_(nullptr)
    , algorithmConfigDock_(nullptr)
    , executionDock_(nullptr)
    , resultsDock_(nullptr)
    , visualizationControlDock_(nullptr)
    , nodeListDock_(nullptr)
{
    createPanelsAndDockWidgets();
}

UIManager::~UIManager()
{
}

void UIManager::createPanelsAndDockWidgets()
{
    seedSelectionPanel_ = new SeedSelectionPanel(mainWindow_);
    fileOperationsPanel_ = new FileOperationsPanel(mainWindow_);
    algorithmConfigPanel_ = new AlgorithmConfigPanel(mainWindow_);
    executionPanel_ = new ExecutionPanel(mainWindow_);
    resultsPanel_ = new ResultsPanel(mainWindow_);
    visualizationControlPanel_ = new VisualizationControlPanel(mainWindow_);

    fileOperationsDock_ = new QDockWidget("File Operations", mainWindow_);
    fileOperationsDock_->setObjectName("FileOperationsPanel");
    fileOperationsDock_->setWidget(fileOperationsPanel_);
    mainWindow_->addDockWidget(Qt::LeftDockWidgetArea, fileOperationsDock_);
    
    visualizationControlDock_ = new QDockWidget("Visualization Controls", mainWindow_);
    visualizationControlDock_->setObjectName("VisualizationControlPanel");
    visualizationControlDock_->setWidget(visualizationControlPanel_);
    mainWindow_->addDockWidget(Qt::LeftDockWidgetArea, visualizationControlDock_);
    
    seedSelectionDock_ = new QDockWidget("Seed Selection", mainWindow_);
    seedSelectionDock_->setObjectName("SeedSelectionPanel");
    seedSelectionDock_->setWidget(seedSelectionPanel_);
    mainWindow_->addDockWidget(Qt::LeftDockWidgetArea, seedSelectionDock_);
    
    algorithmConfigDock_ = new QDockWidget("Algorithm Configuration", mainWindow_);
    algorithmConfigDock_->setObjectName("AlgorithmConfigPanel");
    algorithmConfigDock_->setWidget(algorithmConfigPanel_);
    mainWindow_->addDockWidget(Qt::RightDockWidgetArea, algorithmConfigDock_);
    
    executionDock_ = new QDockWidget("Execution Control", mainWindow_);
    executionDock_->setObjectName("ExecutionPanel");
    executionDock_->setWidget(executionPanel_);
    mainWindow_->addDockWidget(Qt::RightDockWidgetArea, executionDock_);
    
    resultsDock_ = new QDockWidget("Results and Export", mainWindow_);
    resultsDock_->setObjectName("ResultsPanel");
    resultsDock_->setWidget(resultsPanel_);
    mainWindow_->addDockWidget(Qt::LeftDockWidgetArea, resultsDock_);
    
    nodeListPanel_ = new NodeListPanel(mainWindow_);
    nodeListDock_ = new QDockWidget("Node List", mainWindow_);
    nodeListDock_->setObjectName("NodeListPanel");
    nodeListDock_->setWidget(nodeListPanel_);
    mainWindow_->addDockWidget(Qt::RightDockWidgetArea, nodeListDock_);
    
    mainWindow_->tabifyDockWidget(fileOperationsDock_, visualizationControlDock_);
    mainWindow_->tabifyDockWidget(visualizationControlDock_, seedSelectionDock_);
    mainWindow_->tabifyDockWidget(seedSelectionDock_, resultsDock_);
    
    mainWindow_->tabifyDockWidget(algorithmConfigDock_, executionDock_);
    mainWindow_->tabifyDockWidget(executionDock_, nodeListDock_);
    
    fileOperationsDock_->show();
    fileOperationsDock_->raise();
    seedSelectionDock_->show();
    algorithmConfigDock_->show();
    executionDock_->show();
    executionDock_->raise();
    resultsDock_->show();
    nodeListDock_->show();
}

SeedSelectionPanel* UIManager::getSeedSelectionPanel() const
{
    return seedSelectionPanel_;
}

FileOperationsPanel* UIManager::getFileOperationsPanel() const
{
    return fileOperationsPanel_;
}

AlgorithmConfigPanel* UIManager::getAlgorithmConfigPanel() const
{
    return algorithmConfigPanel_;
}

ExecutionPanel* UIManager::getExecutionPanel() const
{
    return executionPanel_;
}

ResultsPanel* UIManager::getResultsPanel() const
{
    return resultsPanel_;
}

VisualizationControlPanel* UIManager::getVisualizationControlPanel() const
{
    return visualizationControlPanel_;
}

NodeListPanel* UIManager::getNodeListPanel() const
{
    return nodeListPanel_;
}

void UIManager::toggleFileOperationsPanel(bool visible)
{
    fileOperationsDock_->setVisible(visible);
    emit menuStatesChanged();
}

void UIManager::toggleVisualizationPanel(bool visible)
{
    visualizationControlDock_->setVisible(visible);
    emit menuStatesChanged();
}

void UIManager::toggleSeedSelectionPanel(bool visible)
{
    seedSelectionDock_->setVisible(visible);
    emit menuStatesChanged();
}

void UIManager::toggleAlgorithmConfigPanel(bool visible)
{
    algorithmConfigDock_->setVisible(visible);
    emit menuStatesChanged();
}

void UIManager::toggleExecutionPanel(bool visible)
{
    executionDock_->setVisible(visible);
    emit menuStatesChanged();
}

void UIManager::toggleResultsPanel(bool visible)
{
    resultsDock_->setVisible(visible);
    emit menuStatesChanged();
}

void UIManager::resetLayout()
{
    fileOperationsDock_->setVisible(true);
    fileOperationsDock_->setFloating(false);
    
    visualizationControlDock_->setVisible(false);
    visualizationControlDock_->setFloating(false);
    
    seedSelectionDock_->setVisible(true);
    seedSelectionDock_->setFloating(false);
    
    algorithmConfigDock_->setVisible(true);
    algorithmConfigDock_->setFloating(false);
    
    executionDock_->setVisible(true);
    executionDock_->setFloating(false);
    
    resultsDock_->setVisible(true);
    resultsDock_->setFloating(false);
    
    mainWindow_->removeDockWidget(fileOperationsDock_);
    mainWindow_->removeDockWidget(visualizationControlDock_);
    mainWindow_->removeDockWidget(seedSelectionDock_);
    mainWindow_->removeDockWidget(algorithmConfigDock_);
    mainWindow_->removeDockWidget(executionDock_);
    mainWindow_->removeDockWidget(resultsDock_);
    
    mainWindow_->addDockWidget(Qt::LeftDockWidgetArea, fileOperationsDock_);
    mainWindow_->addDockWidget(Qt::LeftDockWidgetArea, visualizationControlDock_);
    mainWindow_->addDockWidget(Qt::LeftDockWidgetArea, seedSelectionDock_);
    mainWindow_->addDockWidget(Qt::RightDockWidgetArea, algorithmConfigDock_);
    mainWindow_->addDockWidget(Qt::RightDockWidgetArea, executionDock_);
    mainWindow_->addDockWidget(Qt::BottomDockWidgetArea, resultsDock_);
    
    mainWindow_->tabifyDockWidget(fileOperationsDock_, visualizationControlDock_);
    mainWindow_->tabifyDockWidget(visualizationControlDock_, seedSelectionDock_);
    mainWindow_->tabifyDockWidget(algorithmConfigDock_, executionDock_);
    
    fileOperationsDock_->raise();
    executionDock_->raise();
    
    emit menuStatesChanged();
}

void UIManager::updateMenuStates()
{
    emit menuStatesChanged();
} 