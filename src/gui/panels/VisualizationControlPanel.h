/**
 * @file VisualizationControlPanel.h
 * @brief Panel for graph visualization controls
 */

#ifndef VISUALIZATIONCONTROLPANEL_H
#define VISUALIZATIONCONTROLPANEL_H

#include <QWidget>
#include "GraphVisualization.h"

class QVBoxLayout;
class QGroupBox;
class QComboBox;
class QPushButton;
class QCheckBox;
class QStackedWidget;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;

/**
 * @brief View mode for graph visualization
 */
enum class ViewMode {
    BasicNetwork = 0,
    FullGraph = 1
};

/**
 * @brief Panel for visualization controls and layout algorithms
 * 
 * This panel provides controls for graph visualization including
 * layout algorithms, visualization parameters, and display options.
 */
class VisualizationControlPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit VisualizationControlPanel(QWidget *parent = nullptr);

    /**
     * @brief Get the selected layout algorithm index
     * @return Index of the selected layout algorithm
     */
    int getSelectedLayoutAlgorithm() const;

    /**
     * @brief Get whether visualization is enabled
     * @return True if visualization is enabled
     */
    bool isVisualizationEnabled() const;

    /**
     * @brief Get current layout parameters from UI controls
     * @return LayoutParameters struct with current values
     */
    LayoutParameters getLayoutParameters() const;
    
    /**
     * @brief Get current view mode
     * @return Current ViewMode (BasicNetwork or FullGraph)
     */
    ViewMode getViewMode() const;
    
    /**
     * @brief Set whether results are available (enables view mode selector)
     * @param available True if analysis results are available
     */
    void setResultsAvailable(bool available);
    
    /**
     * @brief Check if frequency-based highlighting is enabled
     * @return True if frequency highlighting is enabled
     */
    bool isFrequencyHighlightEnabled() const;
    
    /**
     * @brief Set the number of batches available for selection
     * @param count Number of batches (0 to hide selector, >1 to show)
     */
    void setBatchCount(int count);
    
    /**
     * @brief Get the currently selected batch index
     * @return -1 for "All Batches (General)", 0-based index for specific batch
     */
    int getSelectedBatch() const;
    
    /**
     * @brief Set the number of permutations available for selection
     * @param count Number of permutations (0 to hide selector, >1 to show)
     */
    void setPermutationCount(int count);
    
    /**
     * @brief Get the currently selected permutation index
     * @return -1 for "Best Result", 0-based index for specific permutation
     */
    int getSelectedPermutation() const;

public slots:
    /**
     * @brief Enable or disable visualization
     * @param enabled Whether visualization should be enabled
     */
    void setVisualizationEnabled(bool enabled);

    /**
     * @brief Enable or disable the panel
     * @param enabled Whether the panel should be enabled
     */
    void setEnabled(bool enabled);

signals:
    /**
     * @brief Emitted when the layout type changes
     */
    void layoutTypeChanged();

    /**
     * @brief Emitted when layout algorithm changes
     * @param layoutType The new layout type
     */
    void layoutAlgorithmChanged(int layoutType);

    /**
     * @brief Emitted when layout parameters should be applied
     */
    void applyLayoutRequested();

    /**
     * @brief Emitted when visualization enabled state changes
     * @param enabled New enabled state
     */
    void visualizationEnabledChanged(bool enabled);

    /**
     * @brief Emitted when view mode changes
     * @param mode The new view mode
     */
    void viewModeChanged(ViewMode mode);
    
    /**
     * @brief Emitted when frequency-based highlighting is toggled
     * @param enabled True if frequency highlighting should be used
     */
    void frequencyHighlightChanged(bool enabled);
    
    /**
     * @brief Emitted when node color attribute changes
     * @param attributeIndex Index of the selected attribute (0=Seeds/Connectors, 1=Degree, etc.)
     */
    void nodeColorAttributeChanged(int attributeIndex);
    
    /**
     * @brief Emitted when batch selection changes
     * @param batchIndex -1 for all batches (general), 0-based for specific batch
     */
    void batchSelectionChanged(int batchIndex);
    
    /**
     * @brief Emitted when permutation selection changes
     * @param permutationIndex -1 for best result, 0-based for specific permutation
     */
    void permutationSelectionChanged(int permutationIndex);

private slots:
    /**
     * @brief Handle layout type selection change
     */
    void onLayoutTypeChanged();

    /**
     * @brief Handle apply layout button click
     */
    void onApplyLayoutClicked();

    /**
     * @brief Handle visualization enabled checkbox change
     */
    void onVisualizationEnabledChanged();

private:
    /**
     * @brief Setup the user interface
     */
    void setupUI();

    /**
     * @brief Setup layout algorithm parameters panel
     */
    void setupParametersPanel();
    
    /**
     * @brief Setup Force Atlas parameter widgets
     */
    void setupForceAtlasParamsUI();
    
    /**
     * @brief Setup Spring Force parameter widgets
     */
    void setupSpringForceParamsUI();
    
    /**
     * @brief Setup Circular parameter widgets
     */
    void setupCircularParamsUI();
    
    /**
     * @brief Setup Hierarchical parameter widgets
     */
    void setupHierarchicalParamsUI();
    
    /**
     * @brief Setup Grid parameter widgets
     */
    void setupGridParamsUI();
    
    /**
     * @brief Setup Random parameter widgets
     */
    void setupRandomParamsUI();
    
    /**
     * @brief Setup Automatic (info) widget
     */
    void setupAutomaticParamsUI();
    
    /**
     * @brief Setup Yifan Hu parameter widgets
     */
    void setupYifanHuParamsUI();
    
    /**
     * @brief Setup Seed-Centric parameter widgets
     */
    void setupSeedCentricParamsUI();
    
    /**
     * @brief Setup FWTL (Frequency-Weighted Topological Layout) parameter widgets
     */
    void setupFWTLParamsUI();

    QCheckBox* enableVisualizationCheckBox_;
    QComboBox* layoutTypeCombo_;
    QPushButton* applyLayoutButton_;
    QStackedWidget* parametersStackedWidget_;
    
    QWidget* automaticWidget_;
    QWidget* yifanHuWidget_;
    QWidget* forceAtlasWidget_;
    QWidget* springForceWidget_;
    QWidget* circularWidget_;
    QWidget* hierarchicalWidget_;
    QWidget* gridWidget_;
    QWidget* randomWidget_;
    
    QDoubleSpinBox* forceAtlasRepulsionSpinBox_;
    QDoubleSpinBox* forceAtlasAttractionSpinBox_;
    QDoubleSpinBox* forceAtlasGravitySpinBox_;
    QSpinBox* forceAtlasIterationsSpinBox_;
    
    QDoubleSpinBox* springConstantSpinBox_;
    QDoubleSpinBox* springLengthSpinBox_;
    QDoubleSpinBox* springDampingSpinBox_;
    QSpinBox* springIterationsSpinBox_;
    
    QDoubleSpinBox* circularRadiusSpinBox_;
    QDoubleSpinBox* circularStartAngleSpinBox_;
    QCheckBox* circularClockwiseCheckBox_;
    
    QDoubleSpinBox* gridSpacingSpinBox_;
    QSpinBox* gridColumnsSpinBox_;
    
    QDoubleSpinBox* randomWidthSpinBox_;
    QDoubleSpinBox* randomHeightSpinBox_;
    QSpinBox* randomSeedSpinBox_;
    
    QDoubleSpinBox* forceAtlasTemperatureSpinBox_;
    QDoubleSpinBox* forceAtlasCoolingSpinBox_;
    QCheckBox* forceAtlasPreventOverlapCheckBox_;
    
    QDoubleSpinBox* springOptimalDistanceSpinBox_;
    QDoubleSpinBox* springTemperatureSpinBox_;
    QDoubleSpinBox* springCoolingFactorSpinBox_;
    
    QDoubleSpinBox* yifanOptimalDistanceSpinBox_;
    QDoubleSpinBox* yifanRelativeStrengthSpinBox_;
    QDoubleSpinBox* yifanInitialStepSizeSpinBox_;
    QDoubleSpinBox* yifanStepRatioSpinBox_;
    QCheckBox* yifanAdaptiveCoolingCheckBox_;
    QDoubleSpinBox* yifanConvergenceThresholdSpinBox_;
    QSpinBox* yifanQuadtreeMaxLevelSpinBox_;
    QDoubleSpinBox* yifanThetaSpinBox_;
    
    QWidget* seedCentricWidget_;
    QDoubleSpinBox* seedCentricRadiusSpinBox_;
    QDoubleSpinBox* seedCentricSeedRepulsionSpinBox_;
    QDoubleSpinBox* seedCentricConnectorAttractionSpinBox_;
    QDoubleSpinBox* seedCentricSeedCohesionSpinBox_;
    QDoubleSpinBox* seedCentricConnectorGravitySpinBox_;
    QSpinBox* seedCentricIterationsSpinBox_;
    
    QWidget* fwtlWidget_;
    QDoubleSpinBox* fwtlBaseDistanceSpinBox_;
    QDoubleSpinBox* fwtlFrequencyWeightSpinBox_;
    QDoubleSpinBox* fwtlSeedAttractionSpinBox_;
    QDoubleSpinBox* fwtlEdgeAttractionSpinBox_;
    QDoubleSpinBox* fwtlConfidenceRadiusSpinBox_;
    QDoubleSpinBox* fwtlUncertaintySpreadSpinBox_;
    QSpinBox* fwtlIterationsSpinBox_;
    
    QSpinBox* layoutSeedSpinBox_;
    
    QComboBox* viewModeCombo_;
    QLabel* viewModeLabel_;
    QGroupBox* viewModeGroup_;
    QCheckBox* frequencyHighlightCheckBox_;
    
    QComboBox* nodeColorAttributeCombo_;
    QLabel* nodeColorAttributeLabel_;
    
    QComboBox* batchSelectorCombo_;
    QLabel* batchSelectorLabel_;
    
    QComboBox* permutationSelectorCombo_;
    QLabel* permutationSelectorLabel_;
};

#endif