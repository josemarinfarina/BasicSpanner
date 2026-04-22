/**
 * @file ExecutionPanel.h
 * @brief Panel for analysis execution and progress tracking
 */

#ifndef EXECUTIONPANEL_H
#define EXECUTIONPANEL_H

#include <QWidget>
#include <QString>

class QVBoxLayout;
class QGroupBox;
class QLabel;
class QPushButton;
class QProgressBar;
class QSpinBox;
class QCheckBox;
class QLineEdit;
class QTableWidget;

/**
 * @brief Panel for algorithm execution and progress monitoring
 * 
 * This panel handles the execution of the basic network analysis,
 * configuration of algorithm parameters, and progress tracking.
 */
class ExecutionPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit ExecutionPanel(QWidget *parent = nullptr);

public slots:
    /**
     * @brief Update analysis progress
     * @param current Current progress value
     * @param total Total progress value
     * @param status Status message
     */
    void updateProgress(int current, int total, const QString& status);

    /**
     * @brief Update detailed analysis progress
     * @param permutation Current permutation
     * @param totalPermutations Total permutations
     * @param stage Current stage
     * @param stageProgress Stage progress
     * @param stageTotal Stage total
     */
    void updateDetailedProgress(int permutation, int totalPermutations, 
                               const QString& stage, int stageProgress, int stageTotal);

    /**
     * @brief Handle permutation completion
     * @param permutation Permutation number
     * @param totalPermutations Total permutations
     * @param connectorsFound Number of connectors found
     */
    void onPermutationCompleted(int permutation, int totalPermutations, int connectorsFound);

    /**
     * @brief Handle permutation completion with connector names
     * @param permutation Permutation number
     * @param totalPermutations Total permutations
     * @param connectorsFound Number of connectors found
     * @param connectorNames Names of connector nodes found
     */
    void onPermutationCompletedWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames);

    /**
     * @brief Handle permutation completion with seeds
     * @param permutation Permutation number
     * @param totalPermutations Total permutations
     * @param connectorsFound Number of connectors found
     * @param connectorNames Names of connector nodes found
     * @param seedNames Names of seed nodes found
     */
    void onPermutationCompletedWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames);

    /**
     * @brief Handle analysis start
     */
    void onAnalysisStarted();

    /**
     * @brief Handle analysis start with multiple permutations
     * @param totalPermutations Total number of permutations that will be run
     */
    void onAnalysisStartedWithPermutations(int totalPermutations);

    /**
     * @brief Handle analysis completion
     * @param success Whether analysis was successful
     */
    void onAnalysisFinished(bool success);

    /**
     * @brief Enable or disable the panel
     * @param enabled Whether the panel should be enabled
     */
    void setEnabled(bool enabled);

    void onBatchChanged(int currentBatch, int totalBatches);

signals:
    /**
     * @brief Emitted when run analysis is requested
     */
    void runAnalysisRequested();
    
    /**
     * @brief Emitted when stop analysis is requested
     */
    void stopAnalysisRequested();
    
    /**
     * @brief Emitted when real-time control is requested
     */
    void realTimeControlRequested();

private slots:
    /**
     * @brief Handle run analysis button click
     */
    void onRunAnalysisClicked();
    
    /**
     * @brief Handle stop analysis button click
     */
    void onStopAnalysisClicked();
    
    /**
     * @brief Handle real-time control button click
     */
    void onRealTimeControlClicked();

private:
    /**
     * @brief Setup the user interface
     */
    void setupUI();

    /**
     * @brief Initialize permutation tracking
     * @param totalPermutations Total number of permutations
     */
    void initializePermutationTracking(int totalPermutations);

    QPushButton* runButton_;
    QProgressBar* progressBar_;
    QProgressBar* stageProgressBar_;
    QLabel* statusLabel_;
    QLabel* stageLabel_;
    QLabel* currentPermutationLabel_;
    QLabel* executionModeLabel_;
    QProgressBar* overallProgressBar_;
    QTableWidget* permutationTableWidget_;
    QPushButton* runAnalysisButton_;
    QPushButton* stopAnalysisButton_;
    QPushButton* realTimeControlButton_;
    QLabel* batchLabel_;
    QLabel* batchConnectorsLabel_;
    int lastTotalPermutations_ = 0;
    int nextRowIndex_ = 0;
};

#endif