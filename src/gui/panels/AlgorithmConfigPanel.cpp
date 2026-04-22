/**
 * @file AlgorithmConfigPanel.cpp
 * @brief Implementation of the algorithm configuration panel
 */

#include "AlgorithmConfigPanel.h"
#include "BasicNetworkFinder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QLabel>
#include <QTimer>

AlgorithmConfigPanel::AlgorithmConfigPanel(QWidget *parent)
    : QWidget(parent)
    , permutationsSpinBox_(nullptr)
    , pruningCheckBox_(nullptr)
    , saveResultsCheckBox_(nullptr)
    , goTermNameEdit_(nullptr)
    , goDomainEdit_(nullptr)
    , statisticalAnalysisCheckBox_(nullptr)
    , parallelExecutionCheckBox_(nullptr)
    , maxThreadsSpinBox_(nullptr)
    , randomPruningCheckBox_(nullptr)
{
    setupUI();
    
    connect(permutationsSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmConfigPanel::onParameterChanged);
    connect(pruningCheckBox_, &QCheckBox::toggled, this, &AlgorithmConfigPanel::onParameterChanged);
    connect(saveResultsCheckBox_, &QCheckBox::toggled, this, &AlgorithmConfigPanel::onParameterChanged);
    connect(goTermNameEdit_, &QLineEdit::textChanged, this, &AlgorithmConfigPanel::onParameterChanged);
    connect(goDomainEdit_, &QLineEdit::textChanged, this, &AlgorithmConfigPanel::onParameterChanged);
    connect(statisticalAnalysisCheckBox_, &QCheckBox::toggled, this, &AlgorithmConfigPanel::onParameterChanged);
    connect(parallelExecutionCheckBox_, &QCheckBox::toggled, this, &AlgorithmConfigPanel::onParameterChanged);
    connect(maxThreadsSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmConfigPanel::onParameterChanged);
    connect(randomPruningCheckBox_, &QCheckBox::toggled, this, &AlgorithmConfigPanel::onParameterChanged);
    
    QTimer::singleShot(0, this, &AlgorithmConfigPanel::onParameterChanged);
}

void AlgorithmConfigPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    QGroupBox* configGroup = new QGroupBox("Algorithm Configuration");
    QVBoxLayout* configLayout = new QVBoxLayout(configGroup);
    configLayout->setSpacing(8);

    QFormLayout* topFormLayout = new QFormLayout;
    topFormLayout->setSpacing(8);

    pruningCheckBox_ = new QCheckBox;
    pruningCheckBox_->setChecked(true);
    pruningCheckBox_->setToolTip("Enable pruning optimization to improve performance");
    topFormLayout->addRow("Pruning:", pruningCheckBox_);

    randomPruningCheckBox_ = new QCheckBox;
    randomPruningCheckBox_->setChecked(false);
    randomPruningCheckBox_->setToolTip("Resolve ties randomly during pruning");
    topFormLayout->addRow("Solve Ties Randomly:", randomPruningCheckBox_);

    configLayout->addLayout(topFormLayout);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->setSpacing(8);

    permutationsSpinBox_ = new QSpinBox;
    permutationsSpinBox_->setRange(1, 10000);
    permutationsSpinBox_->setValue(100);
    permutationsSpinBox_->setToolTip("Number of permutations to run for statistical analysis");
    formLayout->addRow("Permutations:", permutationsSpinBox_);

    statisticalAnalysisCheckBox_ = new QCheckBox;
    statisticalAnalysisCheckBox_->setChecked(false);
    statisticalAnalysisCheckBox_->setToolTip("Enable statistical analysis of results");
    formLayout->addRow("Statistical Analysis:", statisticalAnalysisCheckBox_);

    parallelExecutionCheckBox_ = new QCheckBox;
    parallelExecutionCheckBox_->setChecked(true);
    parallelExecutionCheckBox_->setToolTip("Enable parallel execution for faster processing");
    formLayout->addRow("Parallel Execution:", parallelExecutionCheckBox_);

    maxThreadsSpinBox_ = new QSpinBox;
    maxThreadsSpinBox_->setRange(1, 1024);
    maxThreadsSpinBox_->setValue(4);
    maxThreadsSpinBox_->setToolTip("Maximum number of threads to use for parallel execution");
    formLayout->addRow("Max Threads:", maxThreadsSpinBox_);

    configLayout->addLayout(formLayout);

    QGroupBox* goGroup = new QGroupBox("GO Terms (Optional)");
    QFormLayout* goLayout = new QFormLayout(goGroup);
    goLayout->setSpacing(6);

    goTermNameEdit_ = new QLineEdit;
    goTermNameEdit_->setPlaceholderText("Enter GO term name...");
    goTermNameEdit_->setToolTip("Optional Gene Ontology term name for annotation");
    goLayout->addRow("GO Term Name:", goTermNameEdit_);

    goDomainEdit_ = new QLineEdit;
    goDomainEdit_->setPlaceholderText("Enter GO domain...");
    goDomainEdit_->setToolTip("Optional Gene Ontology domain for annotation");
    goLayout->addRow("GO Domain:", goDomainEdit_);

    configLayout->addWidget(goGroup);

    QGroupBox* outputGroup = new QGroupBox("Output Options");
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->setSpacing(6);

    saveResultsCheckBox_ = new QCheckBox("Automatically save results after analysis");
    saveResultsCheckBox_->setChecked(true);
    saveResultsCheckBox_->setToolTip("Automatically save analysis results to a file");
    outputLayout->addWidget(saveResultsCheckBox_);

    configLayout->addWidget(outputGroup);

    mainLayout->addWidget(configGroup);
    mainLayout->addStretch();

    setLayout(mainLayout);
}

AnalysisConfig AlgorithmConfigPanel::getConfig() const
{
    AnalysisConfig config;
    
    if (permutationsSpinBox_) {
        config.numPermutations = permutationsSpinBox_->value();
    }
    
    if (pruningCheckBox_) {
        config.enablePruning = pruningCheckBox_->isChecked();
    }

    if(randomPruningCheckBox_){
        config.randomPruning = randomPruningCheckBox_->isChecked();
    }

    
    if (statisticalAnalysisCheckBox_) {
        config.enableStatisticalAnalysis = statisticalAnalysisCheckBox_->isChecked();
    }
    
    if (saveResultsCheckBox_) {
        config.saveDetailedResults = saveResultsCheckBox_->isChecked();
    }
    
    if (parallelExecutionCheckBox_) {
        config.enableParallelExecution = parallelExecutionCheckBox_->isChecked();
    }
    
    if (maxThreadsSpinBox_) {
        config.maxThreads = maxThreadsSpinBox_->value();
    }
    
    if (goTermNameEdit_) {
        config.goTermName = goTermNameEdit_->text().trimmed().toStdString();
    }
    
    if (goDomainEdit_) {
        config.goDomain = goDomainEdit_->text().trimmed().toStdString();
    }
    
    return config;
}

void AlgorithmConfigPanel::setConfig(const AnalysisConfig& config)
{
    if (permutationsSpinBox_) {
        permutationsSpinBox_->setValue(config.numPermutations);
    }
    
    if (pruningCheckBox_) {
        pruningCheckBox_->setChecked(config.enablePruning);
    }

    if(randomPruningCheckBox_){
        randomPruningCheckBox_->setChecked(config.randomPruning);
    }
    
    if (statisticalAnalysisCheckBox_) {
        statisticalAnalysisCheckBox_->setChecked(config.enableStatisticalAnalysis);
    }
    
    if (saveResultsCheckBox_) {
        saveResultsCheckBox_->setChecked(config.saveDetailedResults);
    }
    
    if (parallelExecutionCheckBox_) {
        parallelExecutionCheckBox_->setChecked(config.enableParallelExecution);
    }
    
    if (maxThreadsSpinBox_) {
        maxThreadsSpinBox_->setValue(config.maxThreads);
    }
    
    if (goTermNameEdit_) {
        goTermNameEdit_->setText(QString::fromStdString(config.goTermName));
    }
    
    if (goDomainEdit_) {
        goDomainEdit_->setText(QString::fromStdString(config.goDomain));
    }
}

void AlgorithmConfigPanel::resetToDefaults()
{
    setConfig(AnalysisConfig());
}

void AlgorithmConfigPanel::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
    
    if (permutationsSpinBox_) permutationsSpinBox_->setEnabled(enabled);
    if (pruningCheckBox_) pruningCheckBox_->setEnabled(enabled);
    if (randomPruningCheckBox_) randomPruningCheckBox_->setEnabled(enabled);

    if (statisticalAnalysisCheckBox_) statisticalAnalysisCheckBox_->setEnabled(enabled);
    if (parallelExecutionCheckBox_) parallelExecutionCheckBox_->setEnabled(enabled);
    if (maxThreadsSpinBox_) maxThreadsSpinBox_->setEnabled(enabled);
    if (goTermNameEdit_) goTermNameEdit_->setEnabled(enabled);
    if (goDomainEdit_) goDomainEdit_->setEnabled(enabled);
    if (saveResultsCheckBox_) saveResultsCheckBox_->setEnabled(enabled);
}

void AlgorithmConfigPanel::onParameterChanged()
{
    emit configurationChanged();
} 