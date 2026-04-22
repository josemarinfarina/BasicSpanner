/**
 * @file FileOperationsPanel.cpp
 * @brief Implementation of the file operations panel
 */

#include "FileOperationsPanel.h"
#include "GraphFilterPanel.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QFileInfo>

FileOperationsPanel::FileOperationsPanel(QWidget *parent)
    : QWidget(parent)
    , loadGraphButton_(nullptr)
    , fileStatusLabel_(nullptr)
    , graphStatsLabel_(nullptr)
    , graphFilterPanel_(nullptr)
    , hasValidGraph_(false)
{
    setupUI();
    
    connect(loadGraphButton_, &QPushButton::clicked, this, &FileOperationsPanel::onLoadNetworkClicked);
    connect(recentNetworksButton_, &QPushButton::clicked, this, &FileOperationsPanel::onRecentNetworksClicked);
    connect(recentSeedsButton_, &QPushButton::clicked, this, &FileOperationsPanel::onRecentSeedsClicked);
    connect(exportStatsButton_, &QPushButton::clicked, this, &FileOperationsPanel::onExportStatisticsClicked);
}

void FileOperationsPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    QGroupBox* fileGroup = new QGroupBox("Step 1: Load Data");
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    fileLayout->setSpacing(8);

    loadGraphButton_ = new QPushButton("Load Network File");
    loadGraphButton_->setObjectName("loadGraphButton");
    loadGraphButton_->setToolTip("Load a network file (TXT, CSV, TSV)");
    loadGraphButton_->setMinimumHeight(40);

    recentNetworksButton_ = new QPushButton("Recent Networks");
    recentNetworksButton_->setObjectName("recentNetworksButton");
    recentNetworksButton_->setToolTip("Open a recently used network file");
    recentNetworksButton_->setMinimumHeight(35);

    recentSeedsButton_ = new QPushButton("Recent Seeds");
    recentSeedsButton_->setObjectName("recentSeedsButton");
    recentSeedsButton_->setToolTip("Open a recently used seeds file");
    recentSeedsButton_->setMinimumHeight(35);
    
    exportStatsButton_ = new QPushButton("Export Statistics");
    exportStatsButton_->setObjectName("exportStatsButton");
    exportStatsButton_->setToolTip("Export rejected self-loops and isolated nodes to files");
    exportStatsButton_->setMinimumHeight(35);
    exportStatsButton_->setEnabled(false);

    fileStatusLabel_ = new QLabel("No file loaded");
    fileStatusLabel_->setStyleSheet("font-weight: 500; color: #718096;");
    fileStatusLabel_->setWordWrap(true);

    graphStatsLabel_ = new QLabel("");
    graphStatsLabel_->setStyleSheet("font-weight: 500; color: #a0aec0;");
    graphStatsLabel_->setWordWrap(true);

    fileLayout->addWidget(loadGraphButton_);
    fileLayout->addWidget(recentNetworksButton_);
    fileLayout->addWidget(recentSeedsButton_);
    fileLayout->addWidget(exportStatsButton_);
    fileLayout->addWidget(fileStatusLabel_);
    fileLayout->addWidget(graphStatsLabel_);

    mainLayout->addWidget(fileGroup);
    
    graphFilterPanel_ = new GraphFilterPanel(this);
    mainLayout->addWidget(graphFilterPanel_);
    
    connect(graphFilterPanel_, &GraphFilterPanel::applyFiltersRequested,
            this, &FileOperationsPanel::applyFiltersRequested);
    connect(graphFilterPanel_, &GraphFilterPanel::resetFiltersRequested,
            this, &FileOperationsPanel::resetFiltersRequested);
    
    mainLayout->addStretch();

    setLayout(mainLayout);
}

QString FileOperationsPanel::getCurrentFilePath() const
{
    return currentFilePath_;
}

bool FileOperationsPanel::hasValidGraph() const
{
    return hasValidGraph_;
}

void FileOperationsPanel::updateFileStatus(const QString& fileName, const QString& filePath, int nodeCount, int edgeCount,
                                          int rejectedSelfLoops, int isolatedNodes,
                                          int initialConnections, int nonUniqueInteractions)
{
    currentFilePath_ = filePath;
    hasValidGraph_ = true;

    if (fileStatusLabel_) {
        fileStatusLabel_->setText(QString("File: %1").arg(fileName));
        fileStatusLabel_->setStyleSheet("font-weight: 500; color: #00d4aa;");
    }

    if (graphStatsLabel_) {
        QString statsText;
        
        if (initialConnections > 0) {
            statsText += QString("Initial connections: %1\n").arg(initialConnections);
            statsText += QString("Non-unique interactions: %1\n").arg(nonUniqueInteractions);
            statsText += QString("----------------------\n");
        }
        
        statsText += QString("Nodes: %1 | Edges: %2").arg(nodeCount).arg(edgeCount);
        
        if (rejectedSelfLoops > 0 || isolatedNodes > 0) {
            statsText += QString("\nRejected self-loops: %1").arg(rejectedSelfLoops);
            if (isolatedNodes > 0) {
                statsText += QString(" | Isolated nodes: %1").arg(isolatedNodes);
            }
        }
        
        graphStatsLabel_->setText(statsText);
    }
    
    if (exportStatsButton_) {
        exportStatsButton_->setEnabled(rejectedSelfLoops > 0 || isolatedNodes > 0);
    }
    
    if (graphFilterPanel_) {
        graphFilterPanel_->setFilterEnabled(true);
        graphFilterPanel_->resetFilters();
    }

    emit networkLoaded(filePath);
}

void FileOperationsPanel::clearFileStatus()
{
    currentFilePath_.clear();
    hasValidGraph_ = false;

    if (fileStatusLabel_) {
        fileStatusLabel_->setText("No file loaded");
        fileStatusLabel_->setStyleSheet("font-weight: 500; color: #718096;");
    }

    if (graphStatsLabel_) {
        graphStatsLabel_->clear();
    }
    
    if (exportStatsButton_) {
        exportStatsButton_->setEnabled(false);
    }
}

void FileOperationsPanel::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
    
    if (loadGraphButton_) {
        loadGraphButton_->setEnabled(enabled);
    }
    if (recentNetworksButton_) {
        recentNetworksButton_->setEnabled(enabled);
    }
    if (recentSeedsButton_) {
        recentSeedsButton_->setEnabled(enabled);
    }
}

void FileOperationsPanel::onLoadNetworkClicked()
{
    emit loadNetworkRequested();
}

void FileOperationsPanel::onRecentNetworksClicked()
{
    emit recentNetworksRequested();
}

void FileOperationsPanel::onRecentSeedsClicked()
{
    emit recentSeedsRequested();
}

void FileOperationsPanel::onExportStatisticsClicked()
{
    emit exportStatisticsRequested();
}

void FileOperationsPanel::updateGraphFilterStats(int maxDegree, int nodeCount, int edgeCount)
{
    if (graphFilterPanel_) {
        graphFilterPanel_->updateGraphStats(maxDegree, nodeCount, edgeCount);
    }
} 