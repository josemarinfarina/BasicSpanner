/**
 * @file ExecutionPanel.cpp
 * @brief Implementation of the execution panel
 */

#include "ExecutionPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QFormLayout>

ExecutionPanel::ExecutionPanel(QWidget *parent)
    : QWidget(parent)
    , runButton_(nullptr)
    , progressBar_(nullptr)
    , stageProgressBar_(nullptr)
    , statusLabel_(nullptr)
    , stageLabel_(nullptr)
    , currentPermutationLabel_(nullptr)
    , executionModeLabel_(nullptr)
    , overallProgressBar_(nullptr)
    , permutationTableWidget_(nullptr)
{
    setupUI();
    
    connect(runAnalysisButton_, &QPushButton::clicked, this, &ExecutionPanel::onRunAnalysisClicked);
    connect(stopAnalysisButton_, &QPushButton::clicked, this, &ExecutionPanel::onStopAnalysisClicked);
    connect(realTimeControlButton_, &QPushButton::clicked, this, &ExecutionPanel::onRealTimeControlClicked);
}

void ExecutionPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    QGroupBox* executionGroup = new QGroupBox("Analysis Execution");
    QVBoxLayout* executionLayout = new QVBoxLayout(executionGroup);
    executionLayout->setSpacing(8);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(8);

    runAnalysisButton_ = new QPushButton("Run Analysis");
    runAnalysisButton_->setToolTip("Start the basic network analysis");
    runAnalysisButton_->setMinimumHeight(35);

    stopAnalysisButton_ = new QPushButton("Stop Analysis");
    stopAnalysisButton_->setToolTip("Stop the current analysis");
    stopAnalysisButton_->setEnabled(false);
    stopAnalysisButton_->setMinimumHeight(35);

    realTimeControlButton_ = new QPushButton("Real-Time Control");
    realTimeControlButton_->setToolTip("Open real-time connector histogram window");
    realTimeControlButton_->setMinimumHeight(35);

    buttonsLayout->addWidget(runAnalysisButton_);
    buttonsLayout->addWidget(stopAnalysisButton_);
    buttonsLayout->addWidget(realTimeControlButton_);

    executionLayout->addLayout(buttonsLayout);

    QGroupBox* progressGroup = new QGroupBox("Progress");
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);
    progressLayout->setSpacing(8);

    statusLabel_ = new QLabel("Ready to start analysis");
    statusLabel_->setStyleSheet("font-weight: 600; color: #0078d4;");
    progressLayout->addWidget(statusLabel_);

    overallProgressBar_ = new QProgressBar;
    overallProgressBar_->setVisible(false);
    progressLayout->addWidget(overallProgressBar_);

    stageLabel_ = new QLabel("");
    stageLabel_->setStyleSheet("font-weight: 500; color: #666666;");
    stageLabel_->setVisible(false);
    progressLayout->addWidget(stageLabel_);

    stageProgressBar_ = new QProgressBar;
    stageProgressBar_->setVisible(false);
    progressLayout->addWidget(stageProgressBar_);

    currentPermutationLabel_ = new QLabel("");
    currentPermutationLabel_->setStyleSheet("font-weight: 500; color: #0078d4;");
    currentPermutationLabel_->setVisible(false);
    progressLayout->addWidget(currentPermutationLabel_);

    batchLabel_ = new QLabel("");
    batchLabel_->setStyleSheet("font-weight: 500; color: #444;");
    batchLabel_->setVisible(false);
    progressLayout->addWidget(batchLabel_);

    batchConnectorsLabel_ = new QLabel("");
    batchConnectorsLabel_->setStyleSheet("font-weight: 500; color: #444;");
    batchConnectorsLabel_->setVisible(false);
    progressLayout->addWidget(batchConnectorsLabel_);

    executionModeLabel_ = new QLabel("");
    executionModeLabel_->setStyleSheet("font-weight: 500; color: #666666;");
    executionModeLabel_->setVisible(false);
    progressLayout->addWidget(executionModeLabel_);

    permutationTableWidget_ = new QTableWidget(0, 3);
    permutationTableWidget_->setHorizontalHeaderLabels({"Permutation", "Status", "Connectors"});
    permutationTableWidget_->horizontalHeader()->setStretchLastSection(true);
    permutationTableWidget_->setMinimumHeight(150);
    permutationTableWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    permutationTableWidget_->setVisible(false);
    progressLayout->addWidget(permutationTableWidget_, 1);

    mainLayout->addWidget(executionGroup);
    mainLayout->addWidget(progressGroup, 1);

    setLayout(mainLayout);
}

void ExecutionPanel::updateProgress(int current, int total, const QString& status)
{
    if (statusLabel_) {
        statusLabel_->setText(status);
    }

    if (overallProgressBar_) {
        overallProgressBar_->setMaximum(total);
        overallProgressBar_->setValue(current);
        overallProgressBar_->setVisible(true);
    }
}

void ExecutionPanel::updateDetailedProgress(int permutation, int totalPermutations, 
                                           const QString& stage, int stageProgress, int stageTotal)
{
    if (currentPermutationLabel_) {
        currentPermutationLabel_->setText(QString("Permutation %1 of %2")
                                         .arg(permutation)
                                         .arg(totalPermutations));
        currentPermutationLabel_->setVisible(true);
    }

    if (stageLabel_) {
        stageLabel_->setText(stage);
        stageLabel_->setVisible(true);
    }

    if (stageProgressBar_) {
        stageProgressBar_->setMaximum(stageTotal);
        stageProgressBar_->setValue(stageProgress);
        stageProgressBar_->setVisible(true);
    }
}

void ExecutionPanel::onBatchChanged(int currentBatch, int totalBatches)
{
    if (batchLabel_) {
        batchLabel_->setText(QString("Batch %1 of %2").arg(currentBatch).arg(totalBatches));
        batchLabel_->setVisible(true);
    }

    if (permutationTableWidget_) {
        if (!permutationTableWidget_->isVisible()) {
            permutationTableWidget_->setVisible(true);
        }
        int sepRow = permutationTableWidget_->rowCount();
        permutationTableWidget_->insertRow(sepRow);
        auto sepItem = new QTableWidgetItem(QString("--- Batch %1 ---").arg(currentBatch));
        sepItem->setFlags(sepItem->flags() & ~Qt::ItemIsEditable);
        permutationTableWidget_->setItem(sepRow, 0, sepItem);
        permutationTableWidget_->setItem(sepRow, 1, new QTableWidgetItem(""));
        permutationTableWidget_->setItem(sepRow, 2, new QTableWidgetItem(""));
        nextRowIndex_ = permutationTableWidget_->rowCount();
    }

    if (batchConnectorsLabel_) {
        batchConnectorsLabel_->setText("Batch connectors: 0");
        batchConnectorsLabel_->setVisible(true);
    }
}

void ExecutionPanel::onPermutationCompleted(int permutation, int totalPermutations, int connectorsFound)
{
    if (!permutationTableWidget_) return;

    if (!permutationTableWidget_->isVisible()) {
        permutationTableWidget_->setVisible(true);
    }

    int groupStart = 0;
    for (int r = permutationTableWidget_->rowCount() - 1; r >= 0; --r) {
        auto item0 = permutationTableWidget_->item(r, 0);
        if (item0 && item0->text().startsWith("--- Batch ")) {
            groupStart = r + 1;
            break;
        }
    }

    int groupSize = (totalPermutations > 0 ? totalPermutations : (lastTotalPermutations_ > 0 ? lastTotalPermutations_ : permutation));
    int requiredRows = groupStart + groupSize;
    while (permutationTableWidget_->rowCount() < requiredRows) {
        int newRow = permutationTableWidget_->rowCount();
        permutationTableWidget_->insertRow(newRow);
        int relIndex = newRow - groupStart;
        if (relIndex >= 0 && relIndex < groupSize) {
            permutationTableWidget_->setItem(newRow, 0, new QTableWidgetItem(QString::number(relIndex + 1)));
            permutationTableWidget_->setItem(newRow, 1, new QTableWidgetItem("Pending"));
            permutationTableWidget_->setItem(newRow, 2, new QTableWidgetItem("-"));
        } else {
            permutationTableWidget_->setItem(newRow, 0, new QTableWidgetItem(""));
            permutationTableWidget_->setItem(newRow, 1, new QTableWidgetItem(""));
            permutationTableWidget_->setItem(newRow, 2, new QTableWidgetItem(""));
        }
    }

    int row = groupStart + qMax(0, permutation - 1);
    permutationTableWidget_->item(row, 0)->setText(QString::number(permutation));
    permutationTableWidget_->item(row, 1)->setText("Completed");
    permutationTableWidget_->item(row, 2)->setText(QString::number(connectorsFound));

    if (batchConnectorsLabel_) {
        int totalConnectors = 0;
        for (int r = groupStart; r < permutationTableWidget_->rowCount(); ++r) {
            auto item0 = permutationTableWidget_->item(r, 0);
            if (item0 && item0->text().startsWith("--- Batch ")) {
                break;
            }
            auto item = permutationTableWidget_->item(r, 2);
            if (item) {
                bool ok = false;
                int val = item->text().toInt(&ok);
                if (ok) totalConnectors += val;
            }
        }
        batchConnectorsLabel_->setText(QString("Batch connectors: %1").arg(totalConnectors));
        batchConnectorsLabel_->setVisible(true);
    }
}

void ExecutionPanel::onPermutationCompletedWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames)
{
    onPermutationCompleted(permutation, totalPermutations, connectorsFound);
}

void ExecutionPanel::onPermutationCompletedWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames)
{
    lastTotalPermutations_ = totalPermutations;
    onPermutationCompleted(permutation, totalPermutations, connectorsFound);
}

void ExecutionPanel::onAnalysisStarted()
{
    if (runButton_) {
        runButton_->setEnabled(false);
        runButton_->setText("Running...");
    }

    if (stopAnalysisButton_) {
        stopAnalysisButton_->setEnabled(true);
    }

    if (statusLabel_) {
        statusLabel_->setText("Analysis starting...");
    }

    if (overallProgressBar_) overallProgressBar_->setVisible(true);

    nextRowIndex_ = 0;
}

void ExecutionPanel::onAnalysisStartedWithPermutations(int totalPermutations)
{
    onAnalysisStarted();
    
    lastTotalPermutations_ = totalPermutations;
    if (permutationTableWidget_) {
        permutationTableWidget_->clearContents();
        permutationTableWidget_->setRowCount(0);
        permutationTableWidget_->setVisible(true);
    }
}

void ExecutionPanel::onAnalysisFinished(bool success)
{
    if (runButton_) {
        runButton_->setEnabled(true);
        runButton_->setText("Run Analysis");
    }

    if (stopAnalysisButton_) {
        stopAnalysisButton_->setEnabled(false);
    }

    if (statusLabel_) {
        statusLabel_->setText(success ? "Analysis completed successfully" : "Analysis failed");
        statusLabel_->setStyleSheet(success ? "font-weight: 600; color: #28a745;" : "font-weight: 600; color: #dc3545;");
    }

    if (permutationTableWidget_) {
        for (int r = 0; r < permutationTableWidget_->rowCount(); ++r) {
            auto statusItem = permutationTableWidget_->item(r, 1);
            if (statusItem && statusItem->text() == "Pending") {
                statusItem->setText("Completed");
            }
        }
    }
}

void ExecutionPanel::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
    
    if (runButton_) runButton_->setEnabled(enabled);
    if (stopAnalysisButton_) stopAnalysisButton_->setEnabled(enabled && stopAnalysisButton_->isEnabled());
}

void ExecutionPanel::onRunAnalysisClicked()
{
    emit runAnalysisRequested();
}

void ExecutionPanel::onStopAnalysisClicked()
{
    emit stopAnalysisRequested();
}

void ExecutionPanel::onRealTimeControlClicked()
{
    emit realTimeControlRequested();
}

void ExecutionPanel::initializePermutationTracking(int totalPermutations)
{
    if (!permutationTableWidget_) return;

    permutationTableWidget_->setRowCount(totalPermutations);
    permutationTableWidget_->setVisible(true);

    for (int i = 0; i < totalPermutations; ++i) {
        permutationTableWidget_->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        permutationTableWidget_->setItem(i, 1, new QTableWidgetItem("Pending"));
        permutationTableWidget_->setItem(i, 2, new QTableWidgetItem("-"));
    }
} 