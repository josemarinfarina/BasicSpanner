#include "SettingsDialog.h"
#include <QDialogButtonBox>
#include <QMessageBox>

SettingsDialog::SettingsDialog(GraphVisualization* graphViz, QWidget *parent)
    : QDialog(parent), graphVisualization_(graphViz)
{
    setWindowTitle("Settings");
    setModal(true);
    setMinimumSize(500, 400);
    
    setupUI();
    loadCurrentSettings();
    
    connect(nodeColorButton_, &QPushButton::clicked, this, &SettingsDialog::onNodeColorChanged);
    connect(seedColorButton_, &QPushButton::clicked, this, &SettingsDialog::onSeedColorChanged);
    connect(connectorColorButton_, &QPushButton::clicked, this, &SettingsDialog::onConnectorColorChanged);
    connect(extraConnectorColorButton_, &QPushButton::clicked, this, &SettingsDialog::onExtraConnectorColorChanged);
    connect(nodeSizeSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &SettingsDialog::onNodeSizeChanged);
    connect(nodeBorderColorButton_, &QPushButton::clicked, this, &SettingsDialog::onNodeBorderColorChanged);
    connect(nodeBorderWidthSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &SettingsDialog::onNodeBorderWidthChanged);
    connect(showLabelsCheckBox_, &QCheckBox::toggled, this, &SettingsDialog::onShowLabelsToggled);
    connect(fontSizeSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &SettingsDialog::onFontSizeChanged);
    connect(edgeColorButton_, &QPushButton::clicked, this, &SettingsDialog::onEdgeColorChanged);
    connect(edgeWidthSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &SettingsDialog::onEdgeWidthChanged);
    
    connect(resetButton_, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
    connect(applyButton_, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    connect(okButton_, &QPushButton::clicked, this, &SettingsDialog::accept);
    connect(cancelButton_, &QPushButton::clicked, this, &SettingsDialog::reject);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    tabWidget_ = new QTabWidget;
    
    setupAppearanceTab();
    setupGeneralTab();
    
    tabWidget_->addTab(appearanceTab_, "Appearance");
    tabWidget_->addTab(generalTab_, "General");
    
    mainLayout->addWidget(tabWidget_);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    
    resetButton_ = new QPushButton("Restore Defaults");
    resetButton_->setStyleSheet("QPushButton { background-color: #ff9800; color: white; padding: 5px 15px; }");
    
    applyButton_ = new QPushButton("Apply");
    applyButton_->setStyleSheet("QPushButton { background-color: #2196f3; color: white; padding: 5px 15px; }");
    
    okButton_ = new QPushButton("OK");
    okButton_->setStyleSheet("QPushButton { background-color: #4caf50; color: white; padding: 5px 15px; }");
    okButton_->setDefault(true);
    
    cancelButton_ = new QPushButton("Cancel");
    cancelButton_->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 5px 15px; }");
    
    buttonLayout->addWidget(resetButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton_);
    buttonLayout->addWidget(okButton_);
    buttonLayout->addWidget(cancelButton_);
    
    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::setupAppearanceTab()
{
    appearanceTab_ = new QWidget;
    QVBoxLayout* tabLayout = new QVBoxLayout(appearanceTab_);
    
    QGroupBox* nodeGroup = new QGroupBox("Node Appearance");
    QFormLayout* nodeLayout = new QFormLayout(nodeGroup);
    
    nodeColorButton_ = new QPushButton("Select");
    nodeColorButton_->setMinimumHeight(26);
    
    seedColorButton_ = new QPushButton("Select");
    seedColorButton_->setMinimumHeight(26);
    
    connectorColorButton_ = new QPushButton("Select");
    connectorColorButton_->setMinimumHeight(26);
    
    extraConnectorColorButton_ = new QPushButton("Select");
    extraConnectorColorButton_->setMinimumHeight(26);
    
    nodeSizeSpinBox_ = new QDoubleSpinBox;
    nodeSizeSpinBox_->setMinimum(3.0);
    nodeSizeSpinBox_->setMaximum(30.0);
    nodeSizeSpinBox_->setSuffix(" px");
    nodeSizeSpinBox_->setSingleStep(1.0);
    
    nodeBorderColorButton_ = new QPushButton("Select");
    nodeBorderColorButton_->setMinimumHeight(26);
    
    nodeBorderWidthSpinBox_ = new QDoubleSpinBox;
    nodeBorderWidthSpinBox_->setMinimum(0.5);
    nodeBorderWidthSpinBox_->setMaximum(5.0);
    nodeBorderWidthSpinBox_->setSuffix(" px");
    nodeBorderWidthSpinBox_->setSingleStep(0.5);
    
    showLabelsCheckBox_ = new QCheckBox("Show Node Labels");
    
    fontSizeSpinBox_ = new QSpinBox;
    fontSizeSpinBox_->setMinimum(6);
    fontSizeSpinBox_->setMaximum(18);
    fontSizeSpinBox_->setSuffix(" pt");
    
    nodeLayout->addRow("Default Color:", nodeColorButton_);
    nodeLayout->addRow("Seed Color:", seedColorButton_);
    nodeLayout->addRow("Connector Color:", connectorColorButton_);
    nodeLayout->addRow("Extra Connector Color:", extraConnectorColorButton_);
    nodeLayout->addRow("Node Size:", nodeSizeSpinBox_);
    nodeLayout->addRow("Border Color:", nodeBorderColorButton_);
    nodeLayout->addRow("Border Width:", nodeBorderWidthSpinBox_);
    nodeLayout->addRow("", showLabelsCheckBox_);
    nodeLayout->addRow("Font Size:", fontSizeSpinBox_);
    
    QGroupBox* edgeGroup = new QGroupBox("Connection Appearance");
    QFormLayout* edgeLayout = new QFormLayout(edgeGroup);
    
    edgeColorButton_ = new QPushButton("Select Color");
    edgeColorButton_->setMinimumHeight(30);
    
    edgeWidthSpinBox_ = new QDoubleSpinBox;
    edgeWidthSpinBox_->setMinimum(0.1);
    edgeWidthSpinBox_->setMaximum(10.0);
    edgeWidthSpinBox_->setSuffix(" px");
    edgeWidthSpinBox_->setSingleStep(0.1);
    
    edgeLayout->addRow("Connection Color:", edgeColorButton_);
    edgeLayout->addRow("Connection Width:", edgeWidthSpinBox_);
    
    tabLayout->addWidget(nodeGroup);
    tabLayout->addWidget(edgeGroup);
    tabLayout->addStretch();
}

void SettingsDialog::setupGeneralTab()
{
    generalTab_ = new QWidget;
    QVBoxLayout* tabLayout = new QVBoxLayout(generalTab_);
    
    QGroupBox* generalGroup = new QGroupBox("General Settings");
    QFormLayout* generalLayout = new QFormLayout(generalGroup);
    
    QLabel* infoLabel = new QLabel(
        "This tab is available for future general settings.\n\n"
        "For now, use the 'Appearance' tab to customize\n"
        "node and connection visualization."
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: #666; font-style: italic; padding: 20px; }");
    
    generalLayout->addRow(infoLabel);
    
    tabLayout->addWidget(generalGroup);
    tabLayout->addStretch();
}

void SettingsDialog::loadCurrentSettings()
{
    if (!graphVisualization_) return;
    
    NodeAppearance nodeApp = graphVisualization_->getNodeAppearance();
    EdgeAppearance edgeApp = graphVisualization_->getEdgeAppearance();
    
    currentNodeColor_ = nodeApp.color;
    currentSeedColor_ = nodeApp.seedColor;
    currentConnectorColor_ = nodeApp.connectorColor;
    currentExtraConnectorColor_ = nodeApp.extraConnectorColor;
    currentNodeBorderColor_ = nodeApp.borderColor;
    
    updateColorButtonStyle(nodeColorButton_, currentNodeColor_);
    updateColorButtonStyle(seedColorButton_, currentSeedColor_);
    updateColorButtonStyle(connectorColorButton_, currentConnectorColor_);
    updateColorButtonStyle(extraConnectorColorButton_, currentExtraConnectorColor_);
    updateColorButtonStyle(nodeBorderColorButton_, currentNodeBorderColor_);
    
    nodeSizeSpinBox_->setValue(nodeApp.radius);
    nodeBorderWidthSpinBox_->setValue(nodeApp.borderWidth);
    showLabelsCheckBox_->setChecked(nodeApp.showLabels);
    fontSizeSpinBox_->setValue(nodeApp.fontSize);
    
    currentEdgeColor_ = edgeApp.color;
    updateColorButtonStyle(edgeColorButton_, currentEdgeColor_);
    edgeWidthSpinBox_->setValue(edgeApp.width);
}

void SettingsDialog::updateColorButtonStyle(QPushButton* button, const QColor& color)
{
    QString textColor = (color.lightness() > 128) ? "black" : "white";
    button->setStyleSheet(QString(
        "QPushButton { "
        "background-color: %1; "
        "color: %2; "
        "border: 2px solid #333; "
        "padding: 8px; "
        "font-weight: bold; "
        "}"
    ).arg(color.name()).arg(textColor));
}

void SettingsDialog::onNodeColorChanged()
{
    QColor color = QColorDialog::getColor(currentNodeColor_, this, "Select Node Color");
    if (color.isValid()) {
        currentNodeColor_ = color;
        updateColorButtonStyle(nodeColorButton_, color);
    }
}

void SettingsDialog::onSeedColorChanged()
{
    QColor color = QColorDialog::getColor(currentSeedColor_, this, "Select Seed Node Color");
    if (color.isValid()) {
        currentSeedColor_ = color;
        updateColorButtonStyle(seedColorButton_, color);
    }
}

void SettingsDialog::onConnectorColorChanged()
{
    QColor color = QColorDialog::getColor(currentConnectorColor_, this, "Select Connector Node Color");
    if (color.isValid()) {
        currentConnectorColor_ = color;
        updateColorButtonStyle(connectorColorButton_, color);
    }
}

void SettingsDialog::onExtraConnectorColorChanged()
{
    QColor color = QColorDialog::getColor(currentExtraConnectorColor_, this, 
        "Select Extra Connector Color (connectors not in best result)");
    if (color.isValid()) {
        currentExtraConnectorColor_ = color;
        updateColorButtonStyle(extraConnectorColorButton_, color);
    }
}

void SettingsDialog::onNodeSizeChanged(double value)
{
    Q_UNUSED(value)
}

void SettingsDialog::onNodeBorderColorChanged()
{
    QColor color = QColorDialog::getColor(currentNodeBorderColor_, this, "Select Border Color");
    if (color.isValid()) {
        currentNodeBorderColor_ = color;
        updateColorButtonStyle(nodeBorderColorButton_, color);
    }
}

void SettingsDialog::onNodeBorderWidthChanged(double value)
{
    Q_UNUSED(value)
}

void SettingsDialog::onShowLabelsToggled(bool show)
{
    Q_UNUSED(show)
}

void SettingsDialog::onFontSizeChanged(int size)
{
    Q_UNUSED(size)
}

void SettingsDialog::onEdgeColorChanged()
{
    QColor color = QColorDialog::getColor(currentEdgeColor_, this, "Select Connection Color");
    if (color.isValid()) {
        currentEdgeColor_ = color;
        updateColorButtonStyle(edgeColorButton_, color);
    }
}

void SettingsDialog::onEdgeWidthChanged(double value)
{
    Q_UNUSED(value)
}

void SettingsDialog::resetToDefaults()
{
    NodeAppearance defaultNodeApp;
    EdgeAppearance defaultEdgeApp;
    
    currentNodeColor_ = defaultNodeApp.color;
    currentSeedColor_ = defaultNodeApp.seedColor;
    currentConnectorColor_ = defaultNodeApp.connectorColor;
    currentExtraConnectorColor_ = defaultNodeApp.extraConnectorColor;
    currentNodeBorderColor_ = defaultNodeApp.borderColor;
    currentEdgeColor_ = defaultEdgeApp.color;
    
    updateColorButtonStyle(nodeColorButton_, currentNodeColor_);
    updateColorButtonStyle(seedColorButton_, currentSeedColor_);
    updateColorButtonStyle(connectorColorButton_, currentConnectorColor_);
    updateColorButtonStyle(extraConnectorColorButton_, currentExtraConnectorColor_);
    updateColorButtonStyle(nodeBorderColorButton_, currentNodeBorderColor_);
    updateColorButtonStyle(edgeColorButton_, currentEdgeColor_);
    
    nodeSizeSpinBox_->setValue(defaultNodeApp.radius);
    nodeBorderWidthSpinBox_->setValue(defaultNodeApp.borderWidth);
    showLabelsCheckBox_->setChecked(defaultNodeApp.showLabels);
    fontSizeSpinBox_->setValue(defaultNodeApp.fontSize);
    edgeWidthSpinBox_->setValue(defaultEdgeApp.width);
    
    QMessageBox::information(this, "Restored", "Default values have been restored.");
}

void SettingsDialog::applySettings()
{
    if (!graphVisualization_) return;
    
    graphVisualization_->setNodeColor(currentNodeColor_);
    graphVisualization_->setSeedColor(currentSeedColor_);
    graphVisualization_->setConnectorColor(currentConnectorColor_);
    graphVisualization_->setExtraConnectorColor(currentExtraConnectorColor_);
    graphVisualization_->setNodeSize(nodeSizeSpinBox_->value());
    graphVisualization_->setNodeBorderColor(currentNodeBorderColor_);
    graphVisualization_->setNodeBorderWidth(nodeBorderWidthSpinBox_->value());
    graphVisualization_->setShowNodeLabels(showLabelsCheckBox_->isChecked());
    graphVisualization_->setNodeFontSize(fontSizeSpinBox_->value());
    
    graphVisualization_->setEdgeColor(currentEdgeColor_);
    graphVisualization_->setEdgeWidth(edgeWidthSpinBox_->value());
}

void SettingsDialog::accept()
{
    applySettings();
    QDialog::accept();
} 