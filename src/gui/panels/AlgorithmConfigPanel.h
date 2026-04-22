/**
 * @file AlgorithmConfigPanel.h
 * @brief Panel for algorithm configuration parameters
 */

#ifndef ALGORITHMCONFIGPANEL_H
#define ALGORITHMCONFIGPANEL_H

#include <QWidget>
#include <QString>
#include "BasicNetworkFinder.h"

class QVBoxLayout;
class QGroupBox;
class QSpinBox;
class QCheckBox;
class QLineEdit;
class QFormLayout;

/**
 * @brief Panel for algorithm configuration parameters
 * 
 * This panel is responsible solely for managing the configuration
 * parameters of the basic network analysis algorithm. It provides
 * a clean interface to get/set all configuration values.
 */
class AlgorithmConfigPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit AlgorithmConfigPanel(QWidget *parent = nullptr);

    /**
     * @brief Get the current analysis configuration
     * @return AnalysisConfig struct with all current parameter values
     */
    AnalysisConfig getConfig() const;

    /**
     * @brief Set the analysis configuration
     * @param config Configuration to apply to the panel
     */
    void setConfig(const AnalysisConfig& config);

    /**
     * @brief Reset configuration to default values
     */
    void resetToDefaults();

public slots:
    /**
     * @brief Enable or disable the panel
     * @param enabled Whether the panel should be enabled
     */
    void setEnabled(bool enabled);

signals:
    /**
     * @brief Emitted when any configuration parameter changes
     */
    void configurationChanged();

private slots:
    /**
     * @brief Handle configuration parameter changes
     */
    void onParameterChanged();

private:
    /**
     * @brief Setup the user interface
     */
    void setupUI();

    QSpinBox* permutationsSpinBox_;
    QCheckBox* pruningCheckBox_;
    QCheckBox* saveResultsCheckBox_;
    QCheckBox* randomPruningCheckBox_;
    QLineEdit* goTermNameEdit_;
    QLineEdit* goDomainEdit_;
    QCheckBox* statisticalAnalysisCheckBox_;
    QCheckBox* parallelExecutionCheckBox_;
    QSpinBox* maxThreadsSpinBox_;
};

#endif