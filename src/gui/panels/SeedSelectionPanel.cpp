/**
 * @file SeedSelectionPanel.cpp
 * @brief Implementation of the seed selection panel
 */

#include "SeedSelectionPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QCheckBox>

SeedSelectionPanel::SeedSelectionPanel(QWidget *parent) 
    : QWidget(parent)
    , seedTextEdit_(nullptr)
    , seedCountLabel_(nullptr)
    , loadSeedsButton_(nullptr)
    , clearSeedsButton_(nullptr)
    , selectAllButton_(nullptr)
    , generateRandomSeedsButton_(nullptr)
    , amountSpinBox_(nullptr)
    , batchesCheckBox_(nullptr)
    , batchesAmountSpinBox_(nullptr)
{
    setupUI();
    
    connect(seedTextEdit_, &QTextEdit::textChanged, this, &SeedSelectionPanel::onTextChanged);
    connect(loadSeedsButton_, &QPushButton::clicked, this, &SeedSelectionPanel::onLoadSeedsClicked);
    connect(clearSeedsButton_, &QPushButton::clicked, this, &SeedSelectionPanel::clearAllSeeds);
    connect(generateRandomSeedsButton_, &QPushButton::clicked, this, &SeedSelectionPanel::onGenerateRandomSeedsClicked);
    connect(batchesCheckBox_, &QCheckBox::toggled, this, &SeedSelectionPanel::onBatchesToggled);
}

void SeedSelectionPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    QGroupBox* seedGroup = new QGroupBox("Step 2: Define Seeds");
    QVBoxLayout* seedLayout = new QVBoxLayout(seedGroup);
    seedLayout->setSpacing(8);

    seedTextEdit_ = new QTextEdit;
    seedTextEdit_->setPlaceholderText("Enter seed nodes, one per line...");
    seedTextEdit_->setMaximumHeight(150);
    seedTextEdit_->setMinimumHeight(100);

    seedCountLabel_ = new QLabel("Seeds: 0");
    seedCountLabel_->setStyleSheet("font-weight: 600; color: #0078d4;");

    QHBoxLayout* randomSeedsLayout = new QHBoxLayout;
    randomSeedsLayout->setSpacing(8);

    QLabel* amountLabel = new QLabel("Seeds per batch:");
    amountLabel->setStyleSheet("font-weight: 500;");

    amountSpinBox_ = new QSpinBox;
    amountSpinBox_->setMinimum(1);
    amountSpinBox_->setMaximum(1000);
    amountSpinBox_->setValue(10);
    amountSpinBox_->setToolTip("Number of random seeds to generate (per batch if batches enabled)");

    generateRandomSeedsButton_ = new QPushButton("Generate");
    generateRandomSeedsButton_->setToolTip("Generate random seeds from the loaded dataset");
    generateRandomSeedsButton_->setEnabled(false);

    randomSeedsLayout->addWidget(amountLabel);
    randomSeedsLayout->addWidget(amountSpinBox_);
    randomSeedsLayout->addWidget(generateRandomSeedsButton_);
    randomSeedsLayout->addStretch();

    QHBoxLayout* batchesLayout = new QHBoxLayout;
    batchesLayout->setSpacing(8);

    batchesCheckBox_ = new QCheckBox("Enable batches");
    batchesCheckBox_->setToolTip("Enable batch processing for multiple random seed samples");
    batchesCheckBox_->setEnabled(false);

    QLabel* batchesAmountLabel = new QLabel("Number of batches:");
    batchesAmountLabel->setStyleSheet("font-weight: 500;");

    batchesAmountSpinBox_ = new QSpinBox;
    batchesAmountSpinBox_->setMinimum(1);
    batchesAmountSpinBox_->setMaximum(10000000);
    batchesAmountSpinBox_->setValue(10);
    batchesAmountSpinBox_->setToolTip("Number of batches to generate");
    batchesAmountSpinBox_->setEnabled(false);

    batchesLayout->addWidget(batchesCheckBox_);
    batchesLayout->addWidget(batchesAmountLabel);
    batchesLayout->addWidget(batchesAmountSpinBox_);
    batchesLayout->addStretch();

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(8);

    loadSeedsButton_ = new QPushButton("Load from File");
    loadSeedsButton_->setToolTip("Load seed nodes from a text file");
    
    clearSeedsButton_ = new QPushButton("Clear All");
    clearSeedsButton_->setToolTip("Clear all seed nodes");
    
    selectAllButton_ = new QPushButton("Select All Nodes");
    selectAllButton_->setToolTip("Select all nodes in the graph as seeds");
    selectAllButton_->setEnabled(false);

    buttonsLayout->addWidget(loadSeedsButton_);
    buttonsLayout->addWidget(clearSeedsButton_);
    buttonsLayout->addWidget(selectAllButton_);
    buttonsLayout->addStretch();

    seedLayout->addWidget(seedTextEdit_);
    seedLayout->addWidget(seedCountLabel_);
    seedLayout->addLayout(randomSeedsLayout);
    seedLayout->addLayout(batchesLayout);
    seedLayout->addLayout(buttonsLayout);

    mainLayout->addWidget(seedGroup);
    mainLayout->addStretch();

    setLayout(mainLayout);
}

std::vector<std::string> SeedSelectionPanel::getSeedNodes() const
{
    std::vector<std::string> seeds;
    if (!seedTextEdit_) {
        return seeds;
    }

    QString text = seedTextEdit_->toPlainText();
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            seeds.push_back(trimmed.toStdString());
        }
    }
    
    return seeds;
}

int SeedSelectionPanel::getSeedCount() const
{
    auto seeds = getSeedNodes();
    int count = 0;
    for (const auto& seed : seeds) {
        if (seed != "---BATCH_SEPARATOR---") {
            count++;
        }
    }
    return count;
}

void SeedSelectionPanel::clearAllSeeds()
{
    if (seedTextEdit_) {
        seedTextEdit_->clear();
    }
}

void SeedSelectionPanel::setAllNodesAsSeeds(const std::vector<std::string>& allNodeNames)
{
    if (!seedTextEdit_) {
        return;
    }

    QStringList qNodeNames;
    for (const auto& name : allNodeNames) {
        qNodeNames << QString::fromStdString(name);
    }
    
    seedTextEdit_->setPlainText(qNodeNames.join('\n'));
    
    if (selectAllButton_) {
        selectAllButton_->setEnabled(!allNodeNames.empty());
    }
}

void SeedSelectionPanel::setSeedContent(const QString& content)
{
    if (seedTextEdit_) {
        seedTextEdit_->setPlainText(content);
    }
}

void SeedSelectionPanel::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
    
    if (seedTextEdit_) seedTextEdit_->setEnabled(enabled);
    if (loadSeedsButton_) loadSeedsButton_->setEnabled(enabled);
    if (clearSeedsButton_) clearSeedsButton_->setEnabled(enabled);
    if (selectAllButton_) selectAllButton_->setEnabled(enabled);
    if (generateRandomSeedsButton_) generateRandomSeedsButton_->setEnabled(enabled);
    if (amountSpinBox_) amountSpinBox_->setEnabled(enabled);
    if (batchesCheckBox_) batchesCheckBox_->setEnabled(enabled);
    if (batchesAmountSpinBox_) batchesAmountSpinBox_->setEnabled(enabled && batchesCheckBox_->isChecked());
}

void SeedSelectionPanel::onTextChanged()
{
    auto seeds = getSeedNodes();
    int totalSeeds = 0;
    int batchCount = 1;
    int currentBatchSeeds = 0;
    
    for (const auto& seed : seeds) {
        if (seed == "---BATCH_SEPARATOR---") {
            batchCount++;
            currentBatchSeeds = 0;
        } else {
            totalSeeds++;
            currentBatchSeeds++;
        }
    }
    
    if (seedCountLabel_) {
        if (batchCount > 1) {
            int avgSeedsPerBatch = (totalSeeds > 0 && batchCount > 0) ? totalSeeds / batchCount : 0;
            seedCountLabel_->setText(QString("Seeds: %1 total (%2 batches, ~%3 seeds/batch)")
                .arg(totalSeeds).arg(batchCount).arg(avgSeedsPerBatch));
        } else {
            seedCountLabel_->setText(QString("Seeds: %1").arg(totalSeeds));
        }
    }
    emit seedCountChanged(totalSeeds);
    emit seedsChanged(seeds);
}

void SeedSelectionPanel::onLoadSeedsClicked()
{
    emit loadSeedsRequested();
} 

void SeedSelectionPanel::onGenerateRandomSeedsClicked()
{
    int amount = amountSpinBox_->value();
    bool batchesEnabled = batchesCheckBox_->isChecked();
    int batchesAmount = batchesAmountSpinBox_->value();
    
    if (batchesEnabled) {
        emit batchesRequested(true, batchesAmount, amount);
    } else {
        emit generateRandomSeedsRequested(amount);
    }
}

void SeedSelectionPanel::onBatchesToggled(bool checked)
{
    if (batchesAmountSpinBox_) {
        batchesAmountSpinBox_->setEnabled(checked);
    }
    
    if (checked && generateRandomSeedsButton_->isEnabled()) {
        int amount = amountSpinBox_->value();
        int batchesAmount = batchesAmountSpinBox_->value();
        emit batchesRequested(true, batchesAmount, amount);
    }
} 