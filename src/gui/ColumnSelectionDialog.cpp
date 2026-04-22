/**
 * @file ColumnSelectionDialog.cpp
 * @brief Implementation of column selection dialog for data import
 * 
 * This file implements the dialog that allows users to configure how data
 * files are parsed, including column selection, separators, and headers.
 */

#include "ColumnSelectionDialog.h"
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include <QSplitter>
#include <QMessageBox>
#include <QApplication>
#include <QRegularExpression>
#include <iostream>

/**
 * @brief Constructor for ColumnSelectionDialog
 * 
 * Initializes the dialog with default settings and analyzes the provided file
 * to suggest appropriate parsing parameters.
 * 
 * @param filePath Path to the file to be analyzed
 * @param parent Parent widget
 * 
 */
ColumnSelectionDialog::ColumnSelectionDialog(const QString& filePath, QWidget *parent)
    : QDialog(parent), filePath_(filePath)
{
    std::cout << "=== ColumnSelectionDialog CONSTRUCTOR (NEW VERSION WITH CASE SENSITIVE) ===" << std::endl;
    
    result_.accepted = false;
    result_.sourceColumn = 0;
    result_.targetColumn = 1;
    result_.nodeNameColumn = -1;
    result_.applyNamesToTarget = true;
    result_.hasHeader = false;
    result_.skipLines = 0;
    result_.separator = " ";
    result_.caseSensitive = true;
    
    setWindowTitle("Column Selection - " + QFileInfo(filePath).fileName());
    setMinimumSize(800, 600);
    setModal(true);
    
    setupUI();
    
    std::cout << "After setupUI() - caseSensitiveCheckBox_ is: " 
              << (caseSensitiveCheckBox_ ? "VALID" : "NULL!!!") << std::endl;
    
    analyzeFile();
    updatePreview();
}

/**
 * @brief Destructor for ColumnSelectionDialog
 * 
 */
ColumnSelectionDialog::~ColumnSelectionDialog()
{
}

/**
 * @brief Sets up the user interface components
 * 
 * Creates and arranges all widgets including file info, separator selection,
 * column configuration, and preview table.
 * 
 */
void ColumnSelectionDialog::setupUI()
{
    mainLayout_ = new QVBoxLayout(this);
    
    fileInfoGroup_ = new QGroupBox("File Information");
    QVBoxLayout* fileInfoLayout = new QVBoxLayout(fileInfoGroup_);
    
    fileInfoLabel_ = new QLabel();
    fileInfoLabel_->setWordWrap(true);
    fileInfoLayout->addWidget(fileInfoLabel_);
    
    separatorGroup_ = new QGroupBox("Separator");
    QFormLayout* separatorLayout = new QFormLayout(separatorGroup_);
    
    separatorCombo_ = new QComboBox();
    separatorCombo_->addItem("Space", " ");
    separatorCombo_->addItem("Tab", "\t");
    separatorCombo_->addItem("Comma", ",");
    separatorCombo_->addItem("Semicolon", ";");
    separatorCombo_->addItem("Pipe", "|");
    separatorCombo_->setCurrentIndex(0);
    
    separatorLayout->addRow("Separator:", separatorCombo_);
    
    optionsGroup_ = new QGroupBox("Options");
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup_);
    
    hasHeaderCheckBox_ = new QCheckBox("First line contains column names");
    skipLinesSpinBox_ = new QSpinBox();
    skipLinesSpinBox_->setMinimum(0);
    skipLinesSpinBox_->setMaximum(100);
    skipLinesSpinBox_->setValue(0);
    
    caseSensitiveCheckBox_ = new QCheckBox("Case sensitive node names");
    caseSensitiveCheckBox_->setChecked(true);
    caseSensitiveCheckBox_->setToolTip("If checked, 'ABC' and 'abc' are different nodes.\nIf unchecked, all node names are converted to uppercase.");
    
    optionsLayout->addRow(hasHeaderCheckBox_);
    optionsLayout->addRow("Skip lines:", skipLinesSpinBox_);
    optionsLayout->addRow(caseSensitiveCheckBox_);
    
    columnsGroup_ = new QGroupBox("Column Selection");
    QFormLayout* columnsLayout = new QFormLayout(columnsGroup_);
    
    sourceColumnCombo_ = new QComboBox();
    targetColumnCombo_ = new QComboBox();
    nodeNameColumnCombo_ = new QComboBox();
    
    nodeNameColumnCombo_->addItem("(Use IDs as names)", -1);
    
    applyNamesToTargetCheckBox_ = new QCheckBox("Apply names to target column nodes");
    applyNamesToTargetCheckBox_->setChecked(true);
    applyNamesToTargetCheckBox_->setToolTip("If a name column is selected, this determines whether the names apply to the target or source column.");
    applyNamesToTargetCheckBox_->setEnabled(false);

    columnsLayout->addRow("Source column (node 1):", sourceColumnCombo_);
    columnsLayout->addRow("Target column (node 2):", targetColumnCombo_);
    columnsLayout->addRow("Node names column:", nodeNameColumnCombo_);
    columnsLayout->addRow(applyNamesToTargetCheckBox_);
    
    previewGroup_ = new QGroupBox("Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup_);
    
    previewTable_ = new QTableWidget();
    previewTable_->setMaximumHeight(200);
    previewTable_->setAlternatingRowColors(true);
    previewTable_->setSelectionBehavior(QAbstractItemView::SelectColumns);
    
    previewButton_ = new QPushButton("Update Preview");
    
    previewLayout->addWidget(previewTable_);
    previewLayout->addWidget(previewButton_);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    acceptButton_ = new QPushButton("Accept");
    acceptButton_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 10px; }");
    
    cancelButton_ = new QPushButton("Cancel");
    cancelButton_->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 10px; }");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(acceptButton_);
    buttonLayout->addWidget(cancelButton_);
    
    mainLayout_->addWidget(fileInfoGroup_);
    
    QHBoxLayout* configLayout = new QHBoxLayout();
    configLayout->addWidget(separatorGroup_);
    configLayout->addWidget(optionsGroup_);
    mainLayout_->addLayout(configLayout);
    
    mainLayout_->addWidget(columnsGroup_);
    mainLayout_->addWidget(previewGroup_);
    mainLayout_->addLayout(buttonLayout);
    
    connect(separatorCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ColumnSelectionDialog::onSeparatorChanged);
    connect(hasHeaderCheckBox_, &QCheckBox::toggled, this, &ColumnSelectionDialog::onPreviewData);
    connect(skipLinesSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &ColumnSelectionDialog::onPreviewData);
    connect(nodeNameColumnCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index){
        applyNamesToTargetCheckBox_->setEnabled(index > 0);
    });
    connect(previewButton_, &QPushButton::clicked, this, &ColumnSelectionDialog::onPreviewData);
    connect(acceptButton_, &QPushButton::clicked, this, &ColumnSelectionDialog::onAccept);
    connect(cancelButton_, &QPushButton::clicked, this, &ColumnSelectionDialog::onCancel);
}

/**
 * @brief Analyzes the file to determine structure and content
 * 
 * Reads the first 100 lines of the file to analyze its structure,
 * detect separators, and provide file information to the user.
 * 
 */
void ColumnSelectionDialog::analyzeFile()
{
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open file");
        return;
    }
    
    QTextStream in(&file);
    fileLines_.clear();
    
    int lineCount = 0;
    while (!in.atEnd() && lineCount < 100) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            fileLines_.append(line);
            lineCount++;
        }
    }
    
    file.close();
    
    QFileInfo fileInfo(filePath_);
    QString info = QString("File: %1\nSize: %2 KB\nLines analyzed: %3")
                   .arg(fileInfo.fileName())
                   .arg(fileInfo.size() / 1024)
                   .arg(fileLines_.size());
    fileInfoLabel_->setText(info);
    
    detectSeparator();
}

/**
 * @brief Detects the most likely separator used in the file
 * 
 * Analyzes the first few lines to determine which separator gives
 * the most consistent column count across rows.
 * 
 */
void ColumnSelectionDialog::detectSeparator()
{
    if (fileLines_.isEmpty()) return;
    
    QMap<QString, int> separatorCounts;
    QStringList testSeparators = {" ", "\t", ",", ";", "|"};
    
    for (const QString& line : fileLines_.mid(0, qMin(10, fileLines_.size()))) {
        for (const QString& sep : testSeparators) {
            QStringList parts = line.split(sep, Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                separatorCounts[sep] += parts.size();
            }
        }
    }
    
    QString bestSeparator = " ";
    int maxCount = 0;
    
    for (auto it = separatorCounts.begin(); it != separatorCounts.end(); ++it) {
        if (it.value() > maxCount) {
            maxCount = it.value();
            bestSeparator = it.key();
        }
    }
    
    if (separatorCounts.contains("\t") && separatorCounts.contains(" ")) {
        int tabScore = separatorCounts["\t"];
        int spaceScore = separatorCounts[" "];
        if (tabScore >= spaceScore * 0.8) {
            bestSeparator = "\t";
        }
    }
    
    for (int i = 0; i < separatorCombo_->count(); ++i) {
        if (separatorCombo_->itemData(i).toString() == bestSeparator) {
            separatorCombo_->setCurrentIndex(i);
            break;
        }
    }
    
    result_.separator = bestSeparator;
}

/**
 * @brief Parses a row of data using the specified separator
 * 
 * @param line The line to parse
 * @param separator The separator character to use
 * @return List of parsed fields
 * 
 */
QStringList ColumnSelectionDialog::parseRow(const QString& line, const QString& separator)
{
    if (separator == " ") {
        return line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    } else {
        return line.split(separator, Qt::SkipEmptyParts);
    }
}

/**
 * @brief Updates the preview table and column selections
 * 
 * Analyzes the file structure with current settings and populates
 * the preview table and column selection dropdowns.
 * 
 */
void ColumnSelectionDialog::updatePreview()
{
    if (fileLines_.isEmpty()) return;
    
    QString separator = separatorCombo_->currentData().toString();
    result_.separator = separator;
    
    columns_.clear();
    sourceColumnCombo_->clear();
    targetColumnCombo_->clear();
    
    nodeNameColumnCombo_->clear();
    nodeNameColumnCombo_->addItem("(Use IDs as names)", -1);
    
    int startLine = skipLinesSpinBox_->value();
    if (startLine >= fileLines_.size()) {
        QMessageBox::warning(this, "Warning", "Not enough lines to skip");
        return;
    }
    
    QString firstDataLine = fileLines_[startLine];
    if (hasHeaderCheckBox_->isChecked() && startLine + 1 < fileLines_.size()) {
        firstDataLine = fileLines_[startLine + 1];
    }
    
    QStringList firstRowData = parseRow(firstDataLine, separator);
    int columnCount = firstRowData.size();
    
    if (columnCount < 2) {
        QMessageBox::warning(this, "Warning", 
            "At least 2 columns are needed to create connections");
        return;
    }
    
    QStringList columnHeaders;
    if (hasHeaderCheckBox_->isChecked() && startLine < fileLines_.size()) {
        columnHeaders = parseRow(fileLines_[startLine], separator);
    }
    
    for (int i = 0; i < columnCount; ++i) {
        ColumnInfo info;
        info.index = i;
        
        if (i < columnHeaders.size() && hasHeaderCheckBox_->isChecked()) {
            info.name = columnHeaders[i];
        } else {
            info.name = QString("Column %1").arg(i + 1);
        }
        
        if (i < firstRowData.size()) {
            info.sample = firstRowData[i];
        }
        
        columns_.append(info);
        
        QString displayText = QString("%1 (%2)").arg(info.name, info.sample);
        sourceColumnCombo_->addItem(displayText, i);
        targetColumnCombo_->addItem(displayText, i);
        nodeNameColumnCombo_->addItem(displayText, i);
    }
    
    if (columnCount >= 2) {
        sourceColumnCombo_->setCurrentIndex(0);
        targetColumnCombo_->setCurrentIndex(1);
    }
    
    previewTable_->setRowCount(qMin(10, fileLines_.size() - startLine));
    previewTable_->setColumnCount(columnCount);
    
    QStringList headers;
    for (const auto& col : columns_) {
        headers << col.name;
    }
    previewTable_->setHorizontalHeaderLabels(headers);
    
    int previewStartLine = startLine;
    if (hasHeaderCheckBox_->isChecked()) {
        previewStartLine++;
    }
    
    for (int row = 0; row < previewTable_->rowCount() && 
         previewStartLine + row < fileLines_.size(); ++row) {
        
        QString line = fileLines_[previewStartLine + row];
        QStringList rowData = parseRow(line, separator);
        
        for (int col = 0; col < qMin(columnCount, rowData.size()); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem(rowData[col]);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            previewTable_->setItem(row, col, item);
        }
    }
    
    previewTable_->resizeColumnsToContents();
}

/**
 * @brief Handles separator selection changes
 * 
 */
void ColumnSelectionDialog::onSeparatorChanged()
{
    updatePreview();
}

/**
 * @brief Handles preview update requests
 * 
 */
void ColumnSelectionDialog::onPreviewData()
{
    updatePreview();
}

/**
 * @brief Handles dialog acceptance
 * 
 * Validates the configuration and accepts the dialog if valid.
 * 
 */
void ColumnSelectionDialog::onAccept()
{
    result_.sourceColumn = sourceColumnCombo_->currentIndex();
    result_.targetColumn = targetColumnCombo_->currentIndex();
    result_.hasHeader = hasHeaderCheckBox_->isChecked();
    result_.skipLines = skipLinesSpinBox_->value();
    result_.separator = separatorCombo_->currentData().toString();
    result_.caseSensitive = caseSensitiveCheckBox_ ? caseSensitiveCheckBox_->isChecked() : true;
    result_.accepted = true;
    
    std::cout << "=== ColumnSelectionDialog::onAccept() ===" << std::endl;
    std::cout << "caseSensitiveCheckBox_ pointer: " << (caseSensitiveCheckBox_ ? "valid" : "NULL") << std::endl;
    if (caseSensitiveCheckBox_) {
        std::cout << "caseSensitiveCheckBox_->isChecked(): " << (caseSensitiveCheckBox_->isChecked() ? "true" : "false") << std::endl;
    }
    std::cout << "result_.caseSensitive: " << (result_.caseSensitive ? "true" : "false") << std::endl;
    std::cout << "=========================================" << std::endl;

    if (nodeNameColumnCombo_->currentData().toInt() == -1) {
        result_.nodeNameColumn = -1;
        result_.applyNamesToTarget = true;
    } else {
        result_.nodeNameColumn = nodeNameColumnCombo_->currentData().toInt();
        result_.applyNamesToTarget = applyNamesToTargetCheckBox_->isChecked();
    }

    if (result_.sourceColumn == result_.targetColumn) {
        QMessageBox::warning(this, "Invalid Selection", "Source and target columns cannot be the same.");
        return;
    }

    accept();
}

/**
 * @brief Handles dialog cancellation
 * 
 */
void ColumnSelectionDialog::onCancel()
{
    result_.accepted = false;
    reject();
} 