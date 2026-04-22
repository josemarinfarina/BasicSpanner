/**
 * @file ResultsPanel.cpp
 * @brief Implementation of the results panel
 */

#include "ResultsPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QPushButton>

ResultsPanel::ResultsPanel(QWidget *parent)
    : QWidget(parent)
    , resultsTextEdit_(nullptr)
    , saveResultsButton_(nullptr)
    , exportNetworkButton_(nullptr)
    , hasResults_(false)
{
    setupUI();
    
    connect(saveResultsButton_, &QPushButton::clicked, this, &ResultsPanel::onSaveResultsClicked);
    connect(exportNetworkButton_, &QPushButton::clicked, this, &ResultsPanel::onExportNetworkClicked);
}

void ResultsPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    QGroupBox* resultsGroup = new QGroupBox("Analysis Results");
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    resultsLayout->setSpacing(8);

    resultsTextEdit_ = new QTextEdit;
    resultsTextEdit_->setPlaceholderText("Analysis results will appear here...");
    resultsTextEdit_->setReadOnly(true);
    resultsTextEdit_->setMinimumHeight(200);
    resultsTextEdit_->setFont(QFont("Consolas", 9));

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(8);

    saveResultsButton_ = new QPushButton("Save Results");
    saveResultsButton_->setToolTip("Save analysis results to a text file");
    saveResultsButton_->setEnabled(false);

    exportNetworkButton_ = new QPushButton("Export Network");
    exportNetworkButton_->setToolTip("Export the basic network to a file");
    exportNetworkButton_->setEnabled(false);

    buttonsLayout->addWidget(saveResultsButton_);
    buttonsLayout->addWidget(exportNetworkButton_);
    buttonsLayout->addStretch();

    resultsLayout->addWidget(resultsTextEdit_);
    resultsLayout->addLayout(buttonsLayout);

    mainLayout->addWidget(resultsGroup);

    setLayout(mainLayout);
}

bool ResultsPanel::hasResults() const
{
    return hasResults_;
}

QString ResultsPanel::getResultsText() const
{
    return resultsTextEdit_ ? resultsTextEdit_->toPlainText() : QString();
}

void ResultsPanel::displayResults(const QString& results)
{
    if (resultsTextEdit_) {
        resultsTextEdit_->setPlainText(results);
    }

    hasResults_ = !results.isEmpty();

    if (saveResultsButton_) {
        saveResultsButton_->setEnabled(hasResults_);
    }
    if (exportNetworkButton_) {
        exportNetworkButton_->setEnabled(hasResults_);
    }
}

void ResultsPanel::clearResults()
{
    if (resultsTextEdit_) {
        resultsTextEdit_->clear();
    }

    hasResults_ = false;

    if (saveResultsButton_) {
        saveResultsButton_->setEnabled(false);
    }
    if (exportNetworkButton_) {
        exportNetworkButton_->setEnabled(false);
    }
}

void ResultsPanel::appendResults(const QString& text)
{
    if (resultsTextEdit_) {
        resultsTextEdit_->append(text);
    }

    hasResults_ = true;

    if (saveResultsButton_) {
        saveResultsButton_->setEnabled(true);
    }
    if (exportNetworkButton_) {
        exportNetworkButton_->setEnabled(true);
    }
}

void ResultsPanel::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
    
    if (resultsTextEdit_) {
        resultsTextEdit_->setEnabled(enabled);
    }
    if (saveResultsButton_) {
        saveResultsButton_->setEnabled(enabled && hasResults_);
    }
    if (exportNetworkButton_) {
        exportNetworkButton_->setEnabled(enabled && hasResults_);
    }
}

void ResultsPanel::onSaveResultsClicked()
{
    emit saveResultsRequested();
}

void ResultsPanel::onExportNetworkClicked()
{
    emit exportNetworkRequested();
} 