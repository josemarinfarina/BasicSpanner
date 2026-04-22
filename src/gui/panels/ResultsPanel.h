/**
 * @file ResultsPanel.h
 * @brief Panel for displaying and exporting analysis results
 */

#ifndef RESULTSPANEL_H
#define RESULTSPANEL_H

#include <QWidget>
#include <QString>

class QVBoxLayout;
class QGroupBox;
class QTextEdit;
class QPushButton;
class QTabWidget;

/**
 * @brief Panel for results display and export operations
 * 
 * This panel displays analysis results, provides export functionality,
 * and manages results history.
 */
class ResultsPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit ResultsPanel(QWidget *parent = nullptr);

    /**
     * @brief Check if results are available
     * @return True if results are available for export
     */
    bool hasResults() const;

    /**
     * @brief Get the current results text
     * @return Current results as formatted text
     */
    QString getResultsText() const;

public slots:
    /**
     * @brief Display analysis results
     * @param results Formatted results text
     */
    void displayResults(const QString& results);

    /**
     * @brief Clear all results
     */
    void clearResults();

    /**
     * @brief Append text to results
     * @param text Text to append
     */
    void appendResults(const QString& text);

    /**
     * @brief Enable or disable the panel
     * @param enabled Whether the panel should be enabled
     */
    void setEnabled(bool enabled);

signals:
    /**
     * @brief Emitted when save results is requested
     */
    void saveResultsRequested();

    /**
     * @brief Emitted when export network is requested
     */
    void exportNetworkRequested();

private slots:
    /**
     * @brief Handle save results button click
     */
    void onSaveResultsClicked();

    /**
     * @brief Handle export network button click
     */
    void onExportNetworkClicked();

private:
    /**
     * @brief Setup the user interface
     */
    void setupUI();

    QTextEdit* resultsTextEdit_;
    QPushButton* saveResultsButton_;
    QPushButton* exportNetworkButton_;
    
    bool hasResults_;
};

#endif