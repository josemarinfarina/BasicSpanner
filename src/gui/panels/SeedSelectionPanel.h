/**
 * @file SeedSelectionPanel.h
 * @brief Panel for selecting and managing seed nodes
 */

#ifndef SEEDSELECTIONPANEL_H
#define SEEDSELECTIONPANEL_H

#include <QWidget>
#include <vector>
#include <string>

class QVBoxLayout;
class QGroupBox;
class QTextEdit;
class QLabel;
class QPushButton;
class QSpinBox;
class QCheckBox;

/**
 * @brief Panel for seed node selection and management
 * 
 * This panel encapsulates all functionality related to selecting and managing
 * seed nodes for the basic network analysis. It provides methods to get/set
 * seed nodes and emits signals when the seed count changes.
 */
class SeedSelectionPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit SeedSelectionPanel(QWidget *parent = nullptr);

    /**
     * @brief Get the current list of seed nodes
     * @return Vector of seed node names
     */
    std::vector<std::string> getSeedNodes() const;

    /**
     * @brief Get the current seed count
     * @return Number of seed nodes
     */
    int getSeedCount() const;

public slots:
    /**
     * @brief Clear all seed nodes
     */
    void clearAllSeeds();

    /**
     * @brief Set all nodes in the graph as seed nodes
     * @param allNodeNames List of all node names in the graph
     */
    void setAllNodesAsSeeds(const std::vector<std::string>& allNodeNames);

    /**
     * @brief Load seed nodes from text content
     * @param content Text content with seed names, one per line
     */
    void setSeedContent(const QString& content);

    /**
     * @brief Enable or disable the panel
     * @param enabled Whether the panel should be enabled
     */
    void setEnabled(bool enabled);

signals:
    /**
     * @brief Emitted when the seed count changes
     * @param count New seed count
     */
    void seedCountChanged(int count);
    
    /**
     * @brief Emitted when seeds change (for visualization update)
     * @param seeds List of seed node names
     */
    void seedsChanged(const std::vector<std::string>& seeds);

    /**
     * @brief Emitted when load seeds from file is requested
     */
    void loadSeedsRequested();

    /**
     * @brief Emitted when generate random seeds is requested
     * @param amount Number of random seeds to generate
     */
    void generateRandomSeedsRequested(int amount);

    /**
     * @brief Emitted when batches are requested
     * @param enabled Whether batches are enabled
     * @param amount Number of batches
     * @param seedsPerBatch Number of seeds per batch
     */
    void batchesRequested(bool enabled, int amount, int seedsPerBatch);

private slots:
    /**
     * @brief Handle text changes in the seed editor
     */
    void onTextChanged();

    /**
     * @brief Handle load seeds button click
     */
    void onLoadSeedsClicked();

    /**
     * @brief Handle generate random seeds button click
     */
    void onGenerateRandomSeedsClicked();

    /**
     * @brief Handle batches checkbox toggle
     */
    void onBatchesToggled(bool checked);

private:
    /**
     * @brief Setup the user interface
     */
    void setupUI();

    QTextEdit* seedTextEdit_;
    QLabel* seedCountLabel_;
    QPushButton* loadSeedsButton_;
    QPushButton* clearSeedsButton_;
    QPushButton* selectAllButton_;
    QPushButton* generateRandomSeedsButton_;
    QSpinBox* amountSpinBox_;
    QCheckBox* batchesCheckBox_;
    QSpinBox* batchesAmountSpinBox_;
};

#endif