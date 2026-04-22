/**
 * @file ColumnSelectionDialog.h
 * @brief Dialog for configuring data file parsing parameters
 * 
 * This header defines the ColumnSelectionDialog class and related structures
 * for configuring how data files are parsed when loading networks.
 */

#ifndef COLUMN_SELECTION_DIALOG_H
#define COLUMN_SELECTION_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QFileInfo>

/**
 * @brief Structure to hold column selection and parsing configuration
 * 
 * Contains all parameters needed to parse a data file including column
 * assignments, separator configuration, and header handling.
 * 
 */
struct ColumnSelectionResult {
    bool accepted;
    int sourceColumn;
    int targetColumn;
    int nodeNameColumn;
    bool applyNamesToTarget;
    bool hasHeader;
    int skipLines;
    QString separator;
    bool caseSensitive;
};

/**
 * @brief Dialog for selecting columns and configuring data file parsing
 * 
 * This dialog allows users to specify how data files should be parsed
 * when loading networks, including column selection, separators, headers,
 * and provides a preview of the parsed data.
 * 
 */
class ColumnSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param filePath Path to the file to be configured
     * @param parent Parent widget
     * 
     */
    explicit ColumnSelectionDialog(const QString& filePath, QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     * 
     */
    ~ColumnSelectionDialog();
    
    /**
     * @brief Get the dialog result configuration
     * 
     * @return ColumnSelectionResult containing all parsing parameters
     * 
     */
    ColumnSelectionResult getResult() const { return result_; }

private slots:
    /**
     * @brief Handle preview data updates
     * 
     */
    void onPreviewData();
    
    /**
     * @brief Handle dialog acceptance
     * 
     */
    void onAccept();
    
    /**
     * @brief Handle dialog cancellation
     * 
     */
    void onCancel();
    
    /**
     * @brief Handle separator selection changes
     * 
     */
    void onSeparatorChanged();

private:
    /**
     * @brief Set up the user interface
     * 
     */
    void setupUI();
    
    /**
     * @brief Analyze the file structure
     * 
     */
    void analyzeFile();
    
    /**
     * @brief Update the preview table
     * 
     */
    void updatePreview();
    
    /**
     * @brief Detect the most likely separator
     * 
     */
    void detectSeparator();
    
    /**
     * @brief Parse a row using the specified separator
     * 
     * @param line Line to parse
     * @param separator Separator to use
     * @return List of parsed fields
     * 
     */
    QStringList parseRow(const QString& line, const QString& separator);
    
    QVBoxLayout* mainLayout_;
    QGroupBox* fileInfoGroup_;
    QGroupBox* separatorGroup_;
    QGroupBox* columnsGroup_;
    QGroupBox* previewGroup_;
    QGroupBox* optionsGroup_;
    
    QLabel* fileInfoLabel_;
    QComboBox* separatorCombo_;
    QComboBox* sourceColumnCombo_;
    QComboBox* targetColumnCombo_;
    QComboBox* nodeNameColumnCombo_;
    QCheckBox* applyNamesToTargetCheckBox_;
    QCheckBox* hasHeaderCheckBox_;
    QCheckBox* caseSensitiveCheckBox_;
    QSpinBox* skipLinesSpinBox_;
    
    QTableWidget* previewTable_;
    QPushButton* previewButton_;
    
    QPushButton* acceptButton_;
    QPushButton* cancelButton_;
    
    QString filePath_;
    QStringList fileLines_;
    QStringList detectedSeparators_;
    ColumnSelectionResult result_;
    
    /**
     * @brief Information about a column detected in the file
     * 
     */
    struct ColumnInfo {
        QString name;
        QString sample;
        int index;
    };
    QVector<ColumnInfo> columns_;
};

#endif