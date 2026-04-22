/**
 * @file VisualizationControlPanel.cpp
 * @brief Implementation of the visualization control panel
 */

#include "VisualizationControlPanel.h"
#include "GraphVisualization.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QStackedWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QLabel>

VisualizationControlPanel::VisualizationControlPanel(QWidget *parent)
    : QWidget(parent)
    , enableVisualizationCheckBox_(nullptr)
    , layoutTypeCombo_(nullptr)
    , applyLayoutButton_(nullptr)
    , parametersStackedWidget_(nullptr)
{
    setupUI();
    
    connect(enableVisualizationCheckBox_, &QCheckBox::toggled, this, &VisualizationControlPanel::onVisualizationEnabledChanged);
    connect(layoutTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VisualizationControlPanel::onLayoutTypeChanged);
    connect(applyLayoutButton_, &QPushButton::clicked, this, &VisualizationControlPanel::onApplyLayoutClicked);
}

void VisualizationControlPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    QGroupBox* visualGroup = new QGroupBox("Visualization Controls");
    QVBoxLayout* visualLayout = new QVBoxLayout(visualGroup);
    visualLayout->setSpacing(8);

    enableVisualizationCheckBox_ = new QCheckBox("Enable Graph Visualization");
    enableVisualizationCheckBox_->setChecked(false);
    enableVisualizationCheckBox_->setToolTip("Enable or disable graph visualization for better performance");
    visualLayout->addWidget(enableVisualizationCheckBox_);

    layoutTypeCombo_ = new QComboBox;
    layoutTypeCombo_->addItem("Automatic", static_cast<int>(LayoutType::Automatic));
    layoutTypeCombo_->addItem("Yifan Hu", static_cast<int>(LayoutType::YifanHu));
    layoutTypeCombo_->addItem("Force Atlas", static_cast<int>(LayoutType::ForceAtlas));
    layoutTypeCombo_->addItem("Spring Force", static_cast<int>(LayoutType::SpringForce));
    layoutTypeCombo_->addItem("Circular", static_cast<int>(LayoutType::Circular));
    layoutTypeCombo_->addItem("Hierarchical", static_cast<int>(LayoutType::Hierarchical));
    layoutTypeCombo_->addItem("Grid", static_cast<int>(LayoutType::Grid));
    layoutTypeCombo_->addItem("Random", static_cast<int>(LayoutType::Random));
    layoutTypeCombo_->setToolTip("Select the layout algorithm for graph visualization");
    
    QHBoxLayout* layoutSelectionLayout = new QHBoxLayout;
    layoutSelectionLayout->addWidget(new QLabel("Layout Algorithm:"));
    layoutSelectionLayout->addWidget(layoutTypeCombo_);
    visualLayout->addLayout(layoutSelectionLayout);

    applyLayoutButton_ = new QPushButton("Apply Layout");
    applyLayoutButton_->setToolTip("Apply the selected layout algorithm to the graph");
    visualLayout->addWidget(applyLayoutButton_);
    
    QHBoxLayout* seedLayout = new QHBoxLayout;
    seedLayout->addWidget(new QLabel("Layout Seed:"));
    layoutSeedSpinBox_ = new QSpinBox;
    layoutSeedSpinBox_->setRange(0, 999999999);
    layoutSeedSpinBox_->setValue(12345);
    layoutSeedSpinBox_->setToolTip("Random seed for layout algorithms (ensures reproducibility)");
    seedLayout->addWidget(layoutSeedSpinBox_);
    visualLayout->addLayout(seedLayout);

    setupParametersPanel();
    visualLayout->addWidget(parametersStackedWidget_);

    mainLayout->addWidget(visualGroup);
    
    viewModeGroup_ = new QGroupBox("View Mode (after analysis)");
    QVBoxLayout* viewModeLayout = new QVBoxLayout(viewModeGroup_);
    viewModeLayout->setSpacing(6);
    
    viewModeLabel_ = new QLabel("Select what to display:");
    viewModeLayout->addWidget(viewModeLabel_);
    
    viewModeCombo_ = new QComboBox;
    viewModeCombo_->addItem("Basic Network (Seeds + Connectors)", static_cast<int>(ViewMode::BasicNetwork));
    viewModeCombo_->addItem("Full Graph (highlight results)", static_cast<int>(ViewMode::FullGraph));
    viewModeCombo_->setToolTip(
        "Basic Network: Shows only seeds and connectors found\n"
        "Full Graph: Shows entire network with seeds (red) and connectors (green) highlighted"
    );
    viewModeLayout->addWidget(viewModeCombo_);
    
    frequencyHighlightCheckBox_ = new QCheckBox("Extra connectors (> 0%)");
    frequencyHighlightCheckBox_->setToolTip(
        "When enabled, shows all connectors that appeared at least once\n"
        "across permutation results, with varying brightness:\n"
        "• Brighter = appeared in more permutations (more reliable)\n"
        "• Dimmer = appeared in fewer permutations (less reliable)"
    );
    frequencyHighlightCheckBox_->setEnabled(false);
    frequencyHighlightCheckBox_->setVisible(false);
    viewModeLayout->addWidget(frequencyHighlightCheckBox_);
    
    viewModeLayout->addSpacing(8);
    batchSelectorLabel_ = new QLabel("Batch Filter:");
    viewModeLayout->addWidget(batchSelectorLabel_);
    
    batchSelectorCombo_ = new QComboBox;
    batchSelectorCombo_->addItem("All Batches (General)", -1);
    batchSelectorCombo_->setToolTip(
        "Select which batch to visualize:\n"
        "• All Batches (General): Shows seeds and connectors from all batches\n"
        "• Batch N: Shows only seeds and connectors from that specific batch"
    );
    viewModeLayout->addWidget(batchSelectorCombo_);
    
    batchSelectorLabel_->setVisible(false);
    batchSelectorCombo_->setVisible(false);
    
    connect(batchSelectorCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        int batchIndex = getSelectedBatch();
        emit batchSelectionChanged(batchIndex);
    });
    
    viewModeLayout->addSpacing(8);
    permutationSelectorLabel_ = new QLabel("Permutation:");
    viewModeLayout->addWidget(permutationSelectorLabel_);
    
    permutationSelectorCombo_ = new QComboBox;
    permutationSelectorCombo_->addItem("Best Result", -1);
    permutationSelectorCombo_->setToolTip(
        "Select which permutation to visualize:\n"
        "• Best Result: Shows the optimal network found\n"
        "• Permutation N: Shows the result from that specific permutation"
    );
    viewModeLayout->addWidget(permutationSelectorCombo_);
    
    permutationSelectorLabel_->setVisible(false);
    permutationSelectorCombo_->setVisible(false);
    
    connect(permutationSelectorCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        int permutationIndex = getSelectedPermutation();
        emit permutationSelectionChanged(permutationIndex);
    });
    
    viewModeLayout->addSpacing(8);
    nodeColorAttributeLabel_ = new QLabel("Node Color Attribute:");
    viewModeLayout->addWidget(nodeColorAttributeLabel_);
    
    nodeColorAttributeCombo_ = new QComboBox;
    nodeColorAttributeCombo_->addItem("Seeds/Connectors (default)", 0);
    nodeColorAttributeCombo_->addItem("Degree Centrality", 1);
    nodeColorAttributeCombo_->addItem("Betweenness Centrality", 2);
    nodeColorAttributeCombo_->addItem("Closeness Centrality", 3);
    nodeColorAttributeCombo_->addItem("Clustering Coefficient", 4);
    nodeColorAttributeCombo_->addItem("PageRank", 5);
    nodeColorAttributeCombo_->setToolTip(
        "Color nodes by different attributes:\n"
        "• Seeds/Connectors: Default view with analysis results\n"
        "• Degree: Number of connections\n"
        "• Betweenness: How often a node lies on shortest paths\n"
        "• Closeness: How close a node is to all others\n"
        "• Clustering: How connected a node's neighbors are\n"
        "• PageRank: Node importance in the network"
    );
    viewModeLayout->addWidget(nodeColorAttributeCombo_);
    
    viewModeGroup_->setEnabled(false);
    
    connect(viewModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, [this](int) {
        emit viewModeChanged(getViewMode());
    });
    
    connect(frequencyHighlightCheckBox_, &QCheckBox::toggled,
            this, &VisualizationControlPanel::frequencyHighlightChanged);
    
    connect(nodeColorAttributeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VisualizationControlPanel::nodeColorAttributeChanged);
    
    mainLayout->addWidget(viewModeGroup_);
    mainLayout->addStretch();

    setLayout(mainLayout);
}

void VisualizationControlPanel::setupParametersPanel()
{
    parametersStackedWidget_ = new QStackedWidget;
    parametersStackedWidget_->setMaximumHeight(320);

    setupAutomaticParamsUI();
    setupSeedCentricParamsUI();
    setupFWTLParamsUI();
    setupYifanHuParamsUI();
    setupForceAtlasParamsUI();
    setupSpringForceParamsUI();
    setupCircularParamsUI();
    setupHierarchicalParamsUI();
    setupGridParamsUI();
    setupRandomParamsUI();
    
    parametersStackedWidget_->setCurrentIndex(0);
}

void VisualizationControlPanel::setupAutomaticParamsUI()
{
    automaticWidget_ = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(automaticWidget_);
    layout->setSpacing(4);
    
    QLabel* infoLabel = new QLabel(
        "<b>Automatic Layout</b><br>"
        "Selects the best algorithm based on graph size:<br>"
        "• ≤12 nodes: Circular<br>"
        "• ≤50 nodes: Yifan Hu<br>"
        "• ≤200 nodes: Force Atlas<br>"
        "• >200 nodes: Circular (fast)"
    );
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    layout->addStretch();
    
    parametersStackedWidget_->addWidget(automaticWidget_);
}

void VisualizationControlPanel::setupSeedCentricParamsUI()
{
    seedCentricWidget_ = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(seedCentricWidget_);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(4, 8, 4, 4);
    
    QLabel* infoLabel = new QLabel(
        "<b>Seed-Centric Layout</b><br>"
        "<small>Seeds cluster at CENTER, distant nodes spread outward</small>"
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("font-size: 9pt; padding: 2px; margin-bottom: 4px;");
    mainLayout->addWidget(infoLabel);
    
    QFormLayout* layout = new QFormLayout;
    layout->setSpacing(8);
    layout->setVerticalSpacing(10);
    layout->setLabelAlignment(Qt::AlignRight);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    
    seedCentricRadiusSpinBox_ = new QDoubleSpinBox;
    seedCentricRadiusSpinBox_->setRange(1.0, 10000.0);
    seedCentricRadiusSpinBox_->setValue(100.0);
    seedCentricRadiusSpinBox_->setSingleStep(10.0);
    seedCentricRadiusSpinBox_->setToolTip("Base spacing between nodes");
    seedCentricRadiusSpinBox_->setMinimumHeight(24);
    layout->addRow("Node Spacing:", seedCentricRadiusSpinBox_);
    
    seedCentricSeedRepulsionSpinBox_ = new QDoubleSpinBox;
    seedCentricSeedRepulsionSpinBox_->setRange(0.0, 100.0);
    seedCentricSeedRepulsionSpinBox_->setValue(5.0);
    seedCentricSeedRepulsionSpinBox_->setSingleStep(0.5);
    seedCentricSeedRepulsionSpinBox_->setDecimals(1);
    seedCentricSeedRepulsionSpinBox_->setToolTip("How strongly seeds are pulled to center");
    seedCentricSeedRepulsionSpinBox_->setMinimumHeight(24);
    layout->addRow("Seed→Center:", seedCentricSeedRepulsionSpinBox_);
    
    seedCentricConnectorAttractionSpinBox_ = new QDoubleSpinBox;
    seedCentricConnectorAttractionSpinBox_->setRange(0.0, 100.0);
    seedCentricConnectorAttractionSpinBox_->setValue(1.0);
    seedCentricConnectorAttractionSpinBox_->setSingleStep(0.1);
    seedCentricConnectorAttractionSpinBox_->setDecimals(2);
    seedCentricConnectorAttractionSpinBox_->setToolTip("Edge attraction strength");
    seedCentricConnectorAttractionSpinBox_->setMinimumHeight(24);
    layout->addRow("Edge Pull:", seedCentricConnectorAttractionSpinBox_);
    
    seedCentricConnectorGravitySpinBox_ = new QDoubleSpinBox;
    seedCentricConnectorGravitySpinBox_->setRange(0.0, 50.0);
    seedCentricConnectorGravitySpinBox_->setValue(1.0);
    seedCentricConnectorGravitySpinBox_->setSingleStep(0.1);
    seedCentricConnectorGravitySpinBox_->setDecimals(2);
    seedCentricConnectorGravitySpinBox_->setToolTip("Push distant nodes outward (higher = more spread)");
    seedCentricConnectorGravitySpinBox_->setMinimumHeight(24);
    layout->addRow("Outer Push:", seedCentricConnectorGravitySpinBox_);
    
    seedCentricSeedCohesionSpinBox_ = new QDoubleSpinBox;
    seedCentricSeedCohesionSpinBox_->setRange(0.0, 50.0);
    seedCentricSeedCohesionSpinBox_->setValue(3.0);
    seedCentricSeedCohesionSpinBox_->setSingleStep(0.5);
    seedCentricSeedCohesionSpinBox_->setDecimals(1);
    seedCentricSeedCohesionSpinBox_->setToolTip("How strongly seed neighbors are pulled toward seeds");
    seedCentricSeedCohesionSpinBox_->setMinimumHeight(24);
    layout->addRow("Seed Neighbors:", seedCentricSeedCohesionSpinBox_);
    
    seedCentricIterationsSpinBox_ = new QSpinBox;
    seedCentricIterationsSpinBox_->setRange(1, 10000);
    seedCentricIterationsSpinBox_->setValue(200);
    seedCentricIterationsSpinBox_->setToolTip("Number of iterations");
    seedCentricIterationsSpinBox_->setMinimumHeight(24);
    layout->addRow("Iterations:", seedCentricIterationsSpinBox_);
    
    mainLayout->addLayout(layout);
    mainLayout->addStretch();
    
    parametersStackedWidget_->addWidget(seedCentricWidget_);
}

void VisualizationControlPanel::setupFWTLParamsUI()
{
    fwtlWidget_ = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(fwtlWidget_);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(8, 12, 8, 8);
    
    QLabel* infoLabel = new QLabel(
        "<b>FWTL - Frequency-Weighted Topological Layout</b><br>"
        "<span style='color: #4ade80;'><i>Novel algorithm</i></span>"
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("font-size: 10pt; padding: 4px; margin-bottom: 8px;");
    mainLayout->addWidget(infoLabel);
    
    QFormLayout* layout = new QFormLayout;
    layout->setSpacing(12);
    layout->setVerticalSpacing(14);
    layout->setHorizontalSpacing(16);
    layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    
    fwtlBaseDistanceSpinBox_ = new QDoubleSpinBox;
    fwtlBaseDistanceSpinBox_->setRange(10.0, 10000.0);
    fwtlBaseDistanceSpinBox_->setValue(100.0);
    fwtlBaseDistanceSpinBox_->setSingleStep(10.0);
    fwtlBaseDistanceSpinBox_->setToolTip("Base spacing between nodes (K). Larger values spread nodes further apart.");
    fwtlBaseDistanceSpinBox_->setMinimumHeight(28);
    layout->addRow("Base Distance:", fwtlBaseDistanceSpinBox_);
    
    fwtlFrequencyWeightSpinBox_ = new QDoubleSpinBox;
    fwtlFrequencyWeightSpinBox_->setRange(0.0, 10.0);
    fwtlFrequencyWeightSpinBox_->setValue(2.0);
    fwtlFrequencyWeightSpinBox_->setSingleStep(0.1);
    fwtlFrequencyWeightSpinBox_->setDecimals(2);
    fwtlFrequencyWeightSpinBox_->setToolTip("How much connector frequency affects radial position.\nHigher = more separation between high/low frequency connectors.");
    fwtlFrequencyWeightSpinBox_->setMinimumHeight(28);
    layout->addRow("Frequency Weight:", fwtlFrequencyWeightSpinBox_);
    
    fwtlSeedAttractionSpinBox_ = new QDoubleSpinBox;
    fwtlSeedAttractionSpinBox_->setRange(0.0, 50.0);
    fwtlSeedAttractionSpinBox_->setValue(5.0);
    fwtlSeedAttractionSpinBox_->setSingleStep(0.5);
    fwtlSeedAttractionSpinBox_->setDecimals(1);
    fwtlSeedAttractionSpinBox_->setToolTip("How strongly seeds are pulled toward the center.");
    fwtlSeedAttractionSpinBox_->setMinimumHeight(28);
    layout->addRow("Seed → Center:", fwtlSeedAttractionSpinBox_);
    
    fwtlEdgeAttractionSpinBox_ = new QDoubleSpinBox;
    fwtlEdgeAttractionSpinBox_->setRange(0.0, 20.0);
    fwtlEdgeAttractionSpinBox_->setValue(1.0);
    fwtlEdgeAttractionSpinBox_->setSingleStep(0.1);
    fwtlEdgeAttractionSpinBox_->setDecimals(2);
    fwtlEdgeAttractionSpinBox_->setToolTip("Edge spring attraction strength between connected nodes.");
    fwtlEdgeAttractionSpinBox_->setMinimumHeight(28);
    layout->addRow("Edge Pull:", fwtlEdgeAttractionSpinBox_);
    
    fwtlConfidenceRadiusSpinBox_ = new QDoubleSpinBox;
    fwtlConfidenceRadiusSpinBox_->setRange(0.1, 5.0);
    fwtlConfidenceRadiusSpinBox_->setValue(0.3);
    fwtlConfidenceRadiusSpinBox_->setSingleStep(0.1);
    fwtlConfidenceRadiusSpinBox_->setDecimals(2);
    fwtlConfidenceRadiusSpinBox_->setToolTip("Radius multiplier for HIGH-confidence connectors (100% frequency).\nSmaller = closer to seeds.");
    fwtlConfidenceRadiusSpinBox_->setMinimumHeight(28);
    layout->addRow("Confidence Ring:", fwtlConfidenceRadiusSpinBox_);
    
    fwtlUncertaintySpreadSpinBox_ = new QDoubleSpinBox;
    fwtlUncertaintySpreadSpinBox_->setRange(0.5, 10.0);
    fwtlUncertaintySpreadSpinBox_->setValue(2.0);
    fwtlUncertaintySpreadSpinBox_->setSingleStep(0.1);
    fwtlUncertaintySpreadSpinBox_->setDecimals(2);
    fwtlUncertaintySpreadSpinBox_->setToolTip("Spread multiplier for LOW-frequency connectors.\nLarger = more spread in the uncertainty zone.");
    fwtlUncertaintySpreadSpinBox_->setMinimumHeight(28);
    layout->addRow("Uncertainty Spread:", fwtlUncertaintySpreadSpinBox_);
    
    fwtlIterationsSpinBox_ = new QSpinBox;
    fwtlIterationsSpinBox_->setRange(1, 5000);
    fwtlIterationsSpinBox_->setValue(250);
    fwtlIterationsSpinBox_->setToolTip("Number of force-directed iterations.");
    fwtlIterationsSpinBox_->setMinimumHeight(28);
    layout->addRow("Iterations:", fwtlIterationsSpinBox_);
    
    mainLayout->addLayout(layout);
    mainLayout->addStretch();
    
    parametersStackedWidget_->addWidget(fwtlWidget_);
}

void VisualizationControlPanel::setupYifanHuParamsUI()
{
    yifanHuWidget_ = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(yifanHuWidget_);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    QFormLayout* layout = new QFormLayout;
    layout->setSpacing(4);
    
    QLabel* headerLabel = new QLabel("<b>Yifan Hu's properties</b>");
    layout->addRow(headerLabel);
    
    yifanOptimalDistanceSpinBox_ = new QDoubleSpinBox;
    yifanOptimalDistanceSpinBox_->setRange(1.0, 1000.0);
    yifanOptimalDistanceSpinBox_->setValue(100.0);
    yifanOptimalDistanceSpinBox_->setSingleStep(10.0);
    yifanOptimalDistanceSpinBox_->setDecimals(1);
    yifanOptimalDistanceSpinBox_->setToolTip("Optimal distance between connected nodes");
    layout->addRow("Optimal Distance:", yifanOptimalDistanceSpinBox_);
    
    yifanRelativeStrengthSpinBox_ = new QDoubleSpinBox;
    yifanRelativeStrengthSpinBox_->setRange(0.01, 10.0);
    yifanRelativeStrengthSpinBox_->setValue(0.2);
    yifanRelativeStrengthSpinBox_->setSingleStep(0.1);
    yifanRelativeStrengthSpinBox_->setDecimals(2);
    yifanRelativeStrengthSpinBox_->setToolTip("Relative strength of repulsive vs attractive forces");
    layout->addRow("Relative Strength:", yifanRelativeStrengthSpinBox_);
    
    yifanInitialStepSizeSpinBox_ = new QDoubleSpinBox;
    yifanInitialStepSizeSpinBox_->setRange(0.1, 200.0);
    yifanInitialStepSizeSpinBox_->setValue(20.0);
    yifanInitialStepSizeSpinBox_->setSingleStep(1.0);
    yifanInitialStepSizeSpinBox_->setDecimals(1);
    yifanInitialStepSizeSpinBox_->setToolTip("Initial step size for node movement");
    layout->addRow("Initial Step size:", yifanInitialStepSizeSpinBox_);
    
    yifanStepRatioSpinBox_ = new QDoubleSpinBox;
    yifanStepRatioSpinBox_->setRange(0.1, 0.99);
    yifanStepRatioSpinBox_->setValue(0.95);
    yifanStepRatioSpinBox_->setSingleStep(0.01);
    yifanStepRatioSpinBox_->setDecimals(2);
    yifanStepRatioSpinBox_->setToolTip("Step ratio for cooling (lower = faster cooling)");
    layout->addRow("Step ratio:", yifanStepRatioSpinBox_);
    
    yifanAdaptiveCoolingCheckBox_ = new QCheckBox;
    yifanAdaptiveCoolingCheckBox_->setChecked(true);
    yifanAdaptiveCoolingCheckBox_->setToolTip("Use adaptive cooling (adjusts step based on energy)");
    layout->addRow("Adaptive Cooling:", yifanAdaptiveCoolingCheckBox_);
    
    yifanConvergenceThresholdSpinBox_ = new QDoubleSpinBox;
    yifanConvergenceThresholdSpinBox_->setRange(1.0e-6, 1.0e-1);
    yifanConvergenceThresholdSpinBox_->setValue(1.0e-4);
    yifanConvergenceThresholdSpinBox_->setSingleStep(1.0e-4);
    yifanConvergenceThresholdSpinBox_->setDecimals(6);
    yifanConvergenceThresholdSpinBox_->setToolTip("Convergence threshold (smaller = more precise)");
    layout->addRow("Convergence Threshold:", yifanConvergenceThresholdSpinBox_);
    
    mainLayout->addLayout(layout);
    
    QFormLayout* bhLayout = new QFormLayout;
    bhLayout->setSpacing(4);
    
    QLabel* bhHeaderLabel = new QLabel("<b>Barnes-Hut's properties</b>");
    bhLayout->addRow(bhHeaderLabel);
    
    yifanQuadtreeMaxLevelSpinBox_ = new QSpinBox;
    yifanQuadtreeMaxLevelSpinBox_->setRange(1, 30);
    yifanQuadtreeMaxLevelSpinBox_->setValue(10);
    yifanQuadtreeMaxLevelSpinBox_->setToolTip("Maximum depth of the Barnes-Hut quadtree");
    bhLayout->addRow("Quadtree Max Level:", yifanQuadtreeMaxLevelSpinBox_);
    
    yifanThetaSpinBox_ = new QDoubleSpinBox;
    yifanThetaSpinBox_->setRange(0.1, 10.0);
    yifanThetaSpinBox_->setValue(1.2);
    yifanThetaSpinBox_->setSingleStep(0.1);
    yifanThetaSpinBox_->setDecimals(1);
    yifanThetaSpinBox_->setToolTip("Barnes-Hut theta (higher = faster but less accurate)");
    bhLayout->addRow("Theta:", yifanThetaSpinBox_);
    
    mainLayout->addLayout(bhLayout);
    mainLayout->addStretch();
    
    parametersStackedWidget_->addWidget(yifanHuWidget_);
}

void VisualizationControlPanel::setupForceAtlasParamsUI()
{
    QWidget* forceAtlasWidget = new QWidget;
    QFormLayout* layout = new QFormLayout(forceAtlasWidget);
    layout->setSpacing(4);
    
    forceAtlasRepulsionSpinBox_ = new QDoubleSpinBox;
    forceAtlasRepulsionSpinBox_->setRange(10.0, 1000.0);
    forceAtlasRepulsionSpinBox_->setValue(200.0);
    forceAtlasRepulsionSpinBox_->setSuffix(" N");
    forceAtlasRepulsionSpinBox_->setToolTip("Repulsion strength between nodes");
    layout->addRow("Repulsion:", forceAtlasRepulsionSpinBox_);
    
    forceAtlasAttractionSpinBox_ = new QDoubleSpinBox;
    forceAtlasAttractionSpinBox_->setRange(0.1, 5.0);
    forceAtlasAttractionSpinBox_->setValue(0.5);
    forceAtlasAttractionSpinBox_->setSingleStep(0.1);
    forceAtlasAttractionSpinBox_->setToolTip("Attraction strength for connected nodes");
    layout->addRow("Attraction:", forceAtlasAttractionSpinBox_);
    
    forceAtlasGravitySpinBox_ = new QDoubleSpinBox;
    forceAtlasGravitySpinBox_->setRange(0.1, 50.0);
    forceAtlasGravitySpinBox_->setValue(10.0);
    forceAtlasGravitySpinBox_->setToolTip("Central gravity force");
    layout->addRow("Gravity:", forceAtlasGravitySpinBox_);
    
    forceAtlasIterationsSpinBox_ = new QSpinBox;
    forceAtlasIterationsSpinBox_->setRange(50, 1000);
    forceAtlasIterationsSpinBox_->setValue(250);
    forceAtlasIterationsSpinBox_->setToolTip("Number of iterations to run");
    layout->addRow("Iterations:", forceAtlasIterationsSpinBox_);
    
    parametersStackedWidget_->addWidget(forceAtlasWidget);
}

void VisualizationControlPanel::setupSpringForceParamsUI()
{
    QWidget* springForceWidget = new QWidget;
    QFormLayout* layout = new QFormLayout(springForceWidget);
    layout->setSpacing(4);
    
    springOptimalDistanceSpinBox_ = new QDoubleSpinBox;
    springOptimalDistanceSpinBox_->setRange(20.0, 300.0);
    springOptimalDistanceSpinBox_->setValue(100.0);
    springOptimalDistanceSpinBox_->setSuffix(" px");
    springOptimalDistanceSpinBox_->setToolTip("Optimal distance between connected nodes");
    layout->addRow("Optimal Distance:", springOptimalDistanceSpinBox_);
    
    springTemperatureSpinBox_ = new QDoubleSpinBox;
    springTemperatureSpinBox_->setRange(50.0, 500.0);
    springTemperatureSpinBox_->setValue(200.0);
    springTemperatureSpinBox_->setToolTip("Initial temperature for cooling schedule");
    layout->addRow("Temperature:", springTemperatureSpinBox_);
    
    springCoolingFactorSpinBox_ = new QDoubleSpinBox;
    springCoolingFactorSpinBox_->setRange(0.90, 0.99);
    springCoolingFactorSpinBox_->setValue(0.98);
    springCoolingFactorSpinBox_->setSingleStep(0.01);
    springCoolingFactorSpinBox_->setDecimals(3);
    springCoolingFactorSpinBox_->setToolTip("Temperature cooling rate per iteration");
    layout->addRow("Cooling Factor:", springCoolingFactorSpinBox_);
    
    springIterationsSpinBox_ = new QSpinBox;
    springIterationsSpinBox_->setRange(50, 500);
    springIterationsSpinBox_->setValue(200);
    springIterationsSpinBox_->setToolTip("Number of iterations to run");
    layout->addRow("Iterations:", springIterationsSpinBox_);
    
    parametersStackedWidget_->addWidget(springForceWidget);
}

void VisualizationControlPanel::setupCircularParamsUI()
{
    QWidget* circularWidget = new QWidget;
    QFormLayout* layout = new QFormLayout(circularWidget);
    layout->setSpacing(4);
    
    circularRadiusSpinBox_ = new QDoubleSpinBox;
    circularRadiusSpinBox_->setRange(50.0, 2000.0);
    circularRadiusSpinBox_->setValue(200.0);
    circularRadiusSpinBox_->setSuffix(" px");
    circularRadiusSpinBox_->setToolTip("Radius of the circle");
    layout->addRow("Radius:", circularRadiusSpinBox_);
    
    circularStartAngleSpinBox_ = new QDoubleSpinBox;
    circularStartAngleSpinBox_->setRange(0.0, 360.0);
    circularStartAngleSpinBox_->setValue(0.0);
    circularStartAngleSpinBox_->setSuffix("°");
    circularStartAngleSpinBox_->setToolTip("Starting angle in degrees");
    layout->addRow("Start Angle:", circularStartAngleSpinBox_);
    
    circularClockwiseCheckBox_ = new QCheckBox("Clockwise");
    circularClockwiseCheckBox_->setChecked(true);
    circularClockwiseCheckBox_->setToolTip("Direction of node placement");
    layout->addRow("Direction:", circularClockwiseCheckBox_);
    
    parametersStackedWidget_->addWidget(circularWidget);
}

void VisualizationControlPanel::setupHierarchicalParamsUI()
{
    QWidget* hierarchicalWidget = new QWidget;
    QFormLayout* layout = new QFormLayout(hierarchicalWidget);
    layout->setSpacing(4);
    
    QLabel* infoLabel = new QLabel("Hierarchical layout parameters");
    infoLabel->setStyleSheet("color: #666666; font-size: 9pt;");
    layout->addRow(infoLabel);
    
    parametersStackedWidget_->addWidget(hierarchicalWidget);
}

void VisualizationControlPanel::setupGridParamsUI()
{
    QWidget* gridWidget = new QWidget;
    QFormLayout* layout = new QFormLayout(gridWidget);
    layout->setSpacing(4);
    
    gridSpacingSpinBox_ = new QDoubleSpinBox;
    gridSpacingSpinBox_->setRange(10.0, 500.0);
    gridSpacingSpinBox_->setValue(50.0);
    gridSpacingSpinBox_->setSuffix(" px");
    gridSpacingSpinBox_->setToolTip("Spacing between nodes");
    layout->addRow("Spacing:", gridSpacingSpinBox_);
    
    gridColumnsSpinBox_ = new QSpinBox;
    gridColumnsSpinBox_->setRange(1, 100);
    gridColumnsSpinBox_->setValue(10);
    gridColumnsSpinBox_->setToolTip("Number of columns (0 = auto)");
    layout->addRow("Columns:", gridColumnsSpinBox_);
    
    parametersStackedWidget_->addWidget(gridWidget);
}

void VisualizationControlPanel::setupRandomParamsUI()
{
    QWidget* randomWidget = new QWidget;
    QFormLayout* layout = new QFormLayout(randomWidget);
    layout->setSpacing(4);
    
    randomWidthSpinBox_ = new QDoubleSpinBox;
    randomWidthSpinBox_->setRange(100.0, 5000.0);
    randomWidthSpinBox_->setValue(800.0);
    randomWidthSpinBox_->setSuffix(" px");
    randomWidthSpinBox_->setToolTip("Width of the random distribution area");
    layout->addRow("Width:", randomWidthSpinBox_);
    
    randomHeightSpinBox_ = new QDoubleSpinBox;
    randomHeightSpinBox_->setRange(100.0, 5000.0);
    randomHeightSpinBox_->setValue(600.0);
    randomHeightSpinBox_->setSuffix(" px");
    randomHeightSpinBox_->setToolTip("Height of the random distribution area");
    layout->addRow("Height:", randomHeightSpinBox_);
    
    randomSeedSpinBox_ = new QSpinBox;
    randomSeedSpinBox_->setRange(0, 999999);
    randomSeedSpinBox_->setValue(42);
    randomSeedSpinBox_->setToolTip("Random seed (same seed = same layout)");
    layout->addRow("Seed:", randomSeedSpinBox_);
    
    parametersStackedWidget_->addWidget(randomWidget);
}

int VisualizationControlPanel::getSelectedLayoutAlgorithm() const
{
    return layoutTypeCombo_ ? layoutTypeCombo_->currentIndex() : 0;
}

bool VisualizationControlPanel::isVisualizationEnabled() const
{
    return enableVisualizationCheckBox_ ? enableVisualizationCheckBox_->isChecked() : true;
}

ViewMode VisualizationControlPanel::getViewMode() const
{
    if (viewModeCombo_) {
        return static_cast<ViewMode>(viewModeCombo_->currentData().toInt());
    }
    return ViewMode::BasicNetwork;
}

void VisualizationControlPanel::setResultsAvailable(bool available)
{
    if (viewModeGroup_) {
        viewModeGroup_->setEnabled(available);
    }
    if (frequencyHighlightCheckBox_) {
        frequencyHighlightCheckBox_->setEnabled(available);
    }
    if (!available) {
        if (viewModeCombo_) {
            viewModeCombo_->setCurrentIndex(0);
        }
        if (frequencyHighlightCheckBox_) {
            frequencyHighlightCheckBox_->setChecked(false);
        }
        setBatchCount(0);
        setPermutationCount(0);
    }
}

bool VisualizationControlPanel::isFrequencyHighlightEnabled() const
{
    return frequencyHighlightCheckBox_ && frequencyHighlightCheckBox_->isChecked();
}

void VisualizationControlPanel::setBatchCount(int count)
{
    if (!batchSelectorCombo_ || !batchSelectorLabel_) return;
    
    batchSelectorCombo_->blockSignals(true);
    batchSelectorCombo_->clear();
    batchSelectorCombo_->addItem("All Batches (General)", -1);
    
    if (count > 1) {
        for (int i = 0; i < count; ++i) {
            batchSelectorCombo_->addItem(QString("Batch %1").arg(i + 1), i);
        }
        batchSelectorLabel_->setVisible(true);
        batchSelectorCombo_->setVisible(true);
    } else {
        batchSelectorLabel_->setVisible(false);
        batchSelectorCombo_->setVisible(false);
    }
    
    batchSelectorCombo_->setCurrentIndex(0);
    batchSelectorCombo_->blockSignals(false);
}

int VisualizationControlPanel::getSelectedBatch() const
{
    if (batchSelectorCombo_) {
        return batchSelectorCombo_->currentData().toInt();
    }
    return -1;
}

void VisualizationControlPanel::setPermutationCount(int count)
{
    if (!permutationSelectorCombo_ || !permutationSelectorLabel_) return;
    
    permutationSelectorCombo_->blockSignals(true);
    permutationSelectorCombo_->clear();
    permutationSelectorCombo_->addItem("Best Result", -1);
    
    if (count > 1) {
        for (int i = 0; i < count; ++i) {
            permutationSelectorCombo_->addItem(QString("Permutation %1").arg(i + 1), i);
        }
        permutationSelectorLabel_->setVisible(true);
        permutationSelectorCombo_->setVisible(true);
    } else {
        permutationSelectorLabel_->setVisible(false);
        permutationSelectorCombo_->setVisible(false);
    }
    
    permutationSelectorCombo_->setCurrentIndex(0);
    permutationSelectorCombo_->blockSignals(false);
}

int VisualizationControlPanel::getSelectedPermutation() const
{
    if (permutationSelectorCombo_) {
        return permutationSelectorCombo_->currentData().toInt();
    }
    return -1;
}

void VisualizationControlPanel::setVisualizationEnabled(bool enabled)
{
    if (enableVisualizationCheckBox_) {
        enableVisualizationCheckBox_->setChecked(enabled);
    }
}

void VisualizationControlPanel::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
    
    if (enableVisualizationCheckBox_) enableVisualizationCheckBox_->setEnabled(enabled);
    if (layoutTypeCombo_) layoutTypeCombo_->setEnabled(enabled);
    if (applyLayoutButton_) applyLayoutButton_->setEnabled(enabled);
}

void VisualizationControlPanel::onLayoutTypeChanged()
{
    int index = getSelectedLayoutAlgorithm();
    if (parametersStackedWidget_ && index >= 0 && index < parametersStackedWidget_->count()) {
        parametersStackedWidget_->setCurrentIndex(index);
    }
    
    if (layoutTypeCombo_) {
        LayoutType selectedType = static_cast<LayoutType>(layoutTypeCombo_->currentData().toInt());
        emit layoutAlgorithmChanged(static_cast<int>(selectedType));
    }
    
    emit layoutTypeChanged();
}

void VisualizationControlPanel::onApplyLayoutClicked()
{
    emit applyLayoutRequested();
}

void VisualizationControlPanel::onVisualizationEnabledChanged()
{
    bool enabled = isVisualizationEnabled();
    
    if (layoutTypeCombo_) layoutTypeCombo_->setEnabled(enabled);
    if (applyLayoutButton_) applyLayoutButton_->setEnabled(enabled);
    if (parametersStackedWidget_) parametersStackedWidget_->setEnabled(enabled);
    
    emit visualizationEnabledChanged(enabled);
}

 

LayoutParameters VisualizationControlPanel::getLayoutParameters() const
{
    LayoutParameters params;
    
    if (forceAtlasRepulsionSpinBox_) {
        params.forceAtlasRepulsion = forceAtlasRepulsionSpinBox_->value();
    }
    if (forceAtlasAttractionSpinBox_) {
        params.forceAtlasAttraction = forceAtlasAttractionSpinBox_->value();
    }
    if (forceAtlasGravitySpinBox_) {
        params.forceAtlasGravity = forceAtlasGravitySpinBox_->value();
    }
    if (forceAtlasIterationsSpinBox_) {
        params.forceAtlasIterations = forceAtlasIterationsSpinBox_->value();
    }
    
    if (springOptimalDistanceSpinBox_) {
        params.springOptimalDistance = springOptimalDistanceSpinBox_->value();
    }
    if (springTemperatureSpinBox_) {
        params.springTemperature = springTemperatureSpinBox_->value();
    }
    if (springCoolingFactorSpinBox_) {
        params.springCoolingFactor = springCoolingFactorSpinBox_->value();
    }
    if (springIterationsSpinBox_) {
        params.springIterations = springIterationsSpinBox_->value();
    }
    
    if (yifanOptimalDistanceSpinBox_) {
        params.yifanOptimalDistance = yifanOptimalDistanceSpinBox_->value();
    }
    if (yifanRelativeStrengthSpinBox_) {
        params.yifanRelativeStrength = yifanRelativeStrengthSpinBox_->value();
    }
    if (yifanInitialStepSizeSpinBox_) {
        params.yifanInitialStepSize = yifanInitialStepSizeSpinBox_->value();
    }
    if (yifanStepRatioSpinBox_) {
        params.yifanStepRatio = yifanStepRatioSpinBox_->value();
    }
    if (yifanAdaptiveCoolingCheckBox_) {
        params.yifanAdaptiveCooling = yifanAdaptiveCoolingCheckBox_->isChecked();
    }
    if (yifanConvergenceThresholdSpinBox_) {
        params.yifanConvergenceThreshold = yifanConvergenceThresholdSpinBox_->value();
    }
    if (yifanQuadtreeMaxLevelSpinBox_) {
        params.yifanQuadtreeMaxLevel = yifanQuadtreeMaxLevelSpinBox_->value();
    }
    if (yifanThetaSpinBox_) {
        params.yifanTheta = yifanThetaSpinBox_->value();
    }
    
    if (circularRadiusSpinBox_) {
        params.circularRadius = circularRadiusSpinBox_->value();
    }
    if (circularStartAngleSpinBox_) {
        params.circularStartAngle = circularStartAngleSpinBox_->value();
    }
    if (circularClockwiseCheckBox_) {
        params.circularClockwise = circularClockwiseCheckBox_->isChecked();
    }
    
    if (gridSpacingSpinBox_) {
        params.gridSpacing = gridSpacingSpinBox_->value();
    }
    if (gridColumnsSpinBox_) {
        params.gridColumns = gridColumnsSpinBox_->value();
    }
    
    if (randomWidthSpinBox_) {
        params.randomWidth = randomWidthSpinBox_->value();
    }
    if (randomHeightSpinBox_) {
        params.randomHeight = randomHeightSpinBox_->value();
    }
    if (randomSeedSpinBox_) {
        params.randomSeed = randomSeedSpinBox_->value();
    }
    
    if (seedCentricRadiusSpinBox_) {
        params.seedCentricSeedRadius = seedCentricRadiusSpinBox_->value();
    }
    if (seedCentricSeedRepulsionSpinBox_) {
        params.seedCentricSeedRepulsion = seedCentricSeedRepulsionSpinBox_->value();
    }
    if (seedCentricConnectorAttractionSpinBox_) {
        params.seedCentricConnectorAttraction = seedCentricConnectorAttractionSpinBox_->value();
    }
    if (seedCentricSeedCohesionSpinBox_) {
        params.seedCentricSeedCohesion = seedCentricSeedCohesionSpinBox_->value();
    }
    if (seedCentricConnectorGravitySpinBox_) {
        params.seedCentricConnectorGravity = seedCentricConnectorGravitySpinBox_->value();
    }
    if (seedCentricIterationsSpinBox_) {
        params.seedCentricIterations = seedCentricIterationsSpinBox_->value();
    }
    
    if (fwtlBaseDistanceSpinBox_) {
        params.fwtlBaseDistance = fwtlBaseDistanceSpinBox_->value();
    }
    if (fwtlFrequencyWeightSpinBox_) {
        params.fwtlFrequencyWeight = fwtlFrequencyWeightSpinBox_->value();
    }
    if (fwtlSeedAttractionSpinBox_) {
        params.fwtlSeedAttraction = fwtlSeedAttractionSpinBox_->value();
    }
    if (fwtlEdgeAttractionSpinBox_) {
        params.fwtlEdgeAttraction = fwtlEdgeAttractionSpinBox_->value();
    }
    if (fwtlConfidenceRadiusSpinBox_) {
        params.fwtlConfidenceRadius = fwtlConfidenceRadiusSpinBox_->value();
    }
    if (fwtlUncertaintySpreadSpinBox_) {
        params.fwtlUncertaintySpread = fwtlUncertaintySpreadSpinBox_->value();
    }
    if (fwtlIterationsSpinBox_) {
        params.fwtlIterations = fwtlIterationsSpinBox_->value();
    }
    
    if (layoutSeedSpinBox_) {
        params.layoutSeed = layoutSeedSpinBox_->value();
    }
    
    return params;
} 