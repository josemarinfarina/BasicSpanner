/**
 * @file GraphFilterPanel.cpp
 * @brief Implementation of the layer-based graph filter panel
 */

#include "GraphFilterPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QMessageBox>

QString FilterLayer::typeToString(FilterType type)
{
    switch (type) {
        case FilterType::Degree: return "Degree Range";
        case FilterType::Betweenness: return "Betweenness Centrality";
        case FilterType::LargestComponent: return "Largest Component";
        case FilterType::RemoveIsolated: return "Remove Isolated";
        case FilterType::NamePattern: return "Name Pattern";
    }
    return "Unknown";
}

QString FilterLayer::typeToIcon(FilterType type)
{
    switch (type) {
        case FilterType::Degree: return "#";
        case FilterType::Betweenness: return "BC";
        case FilterType::LargestComponent: return "CC";
        case FilterType::RemoveIsolated: return "0";
        case FilterType::NamePattern: return "Aa";
    }
    return "?";
}

FilterLayerWidget::FilterLayerWidget(const FilterLayer& layer, QWidget* parent)
    : QWidget(parent), layer_(layer)
{
    setupUI();
    updateDisplay();
}

void FilterLayerWidget::setupUI()
{
    setMinimumHeight(48);
    setMaximumHeight(52);
    
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(10);
    
    iconLabel_ = new QLabel;
    iconLabel_->setFixedSize(28, 28);
    iconLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel_);
    
    QVBoxLayout* textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(1);
    
    nameLabel_ = new QLabel;
    textLayout->addWidget(nameLabel_);
    
    detailsLabel_ = new QLabel;
    textLayout->addWidget(detailsLabel_);
    
    layout->addLayout(textLayout, 1);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(4);
    
    editButton_ = new QPushButton("Edit");
    editButton_->setFixedHeight(24);
    editButton_->setMinimumWidth(40);
    editButton_->setToolTip("Edit filter parameters");
    buttonLayout->addWidget(editButton_);
    
    moveUpButton_ = new QPushButton("\u25B2");
    moveUpButton_->setFixedSize(24, 24);
    moveUpButton_->setToolTip("Move up");
    buttonLayout->addWidget(moveUpButton_);
    
    moveDownButton_ = new QPushButton("\u25BC");
    moveDownButton_->setFixedSize(24, 24);
    moveDownButton_->setToolTip("Move down");
    buttonLayout->addWidget(moveDownButton_);
    
    removeButton_ = new QPushButton("\u2715");
    removeButton_->setFixedSize(24, 24);
    removeButton_->setToolTip("Remove filter");
    removeButton_->setObjectName("removeFilterButton");
    buttonLayout->addWidget(removeButton_);
    
    layout->addLayout(buttonLayout);
    
    connect(editButton_, &QPushButton::clicked, this, &FilterLayerWidget::editRequested);
    connect(moveUpButton_, &QPushButton::clicked, this, &FilterLayerWidget::moveUpRequested);
    connect(moveDownButton_, &QPushButton::clicked, this, &FilterLayerWidget::moveDownRequested);
    connect(removeButton_, &QPushButton::clicked, this, &FilterLayerWidget::removeRequested);
}

void FilterLayerWidget::updateDisplay()
{
    QString iconBg;
    switch (layer_.type) {
        case FilterType::Degree:
            iconBg = "#00d4aa";
            break;
        case FilterType::Betweenness:
            iconBg = "#a78bfa";
            break;
        case FilterType::LargestComponent:
            iconBg = "#4ade80";
            break;
        case FilterType::RemoveIsolated:
            iconBg = "#fb923c";
            break;
        case FilterType::NamePattern:
            iconBg = "#f472b6";
            break;
    }
    iconLabel_->setStyleSheet(
        QString("background-color: %1; color: #0d1117; border-radius: 4px; font-weight: 600; font-size: 9px;").arg(iconBg)
    );
    iconLabel_->setText(FilterLayer::typeToIcon(layer_.type));
    
    nameLabel_->setText(layer_.displayName);
    nameLabel_->setStyleSheet("font-weight: 500; color: #e2e8f0;");
    
    QString details;
    switch (layer_.type) {
        case FilterType::Degree:
            details = QString("degree \u2265 %1 and \u2264 %2")
                .arg(layer_.parameters.value("min", 0).toInt())
                .arg(layer_.parameters.value("max", 999999).toInt());
            break;
        case FilterType::Betweenness:
            details = QString("BC \u2265 %1 and \u2264 %2")
                .arg(layer_.parameters.value("min", 0.0).toDouble(), 0, 'f', 4)
                .arg(layer_.parameters.value("max", 1.0).toDouble(), 0, 'f', 4);
            break;
        case FilterType::LargestComponent:
            details = "Keep only largest connected component";
            break;
        case FilterType::RemoveIsolated:
            details = "Remove nodes with degree = 0";
            break;
        case FilterType::NamePattern:
            details = QString("%1 matching: %2")
                .arg(layer_.parameters.value("exclude", false).toBool() ? "Remove" : "Keep")
                .arg(layer_.parameters.value("pattern", "").toString());
            break;
    }
    detailsLabel_->setText(details);
    detailsLabel_->setStyleSheet("color: #a0aec0; font-size: 8pt;");
    
    bool hasParams = (layer_.type == FilterType::Degree || 
                      layer_.type == FilterType::Betweenness || 
                      layer_.type == FilterType::NamePattern);
    editButton_->setVisible(hasParams);
}

void FilterLayerWidget::setLayer(const FilterLayer& layer)
{
    layer_ = layer;
    updateDisplay();
}

GraphFilterPanel::GraphFilterPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void GraphFilterPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);
    
    filterGroup_ = new QGroupBox("Node Filters");
    filterGroup_->setCheckable(false);
    QVBoxLayout* groupLayout = new QVBoxLayout(filterGroup_);
    groupLayout->setSpacing(8);
    groupLayout->setContentsMargins(8, 14, 8, 8);
    
    QHBoxLayout* headerLayout = new QHBoxLayout;
    QLabel* headerLabel = new QLabel("Filter Layers");
    headerLabel->setStyleSheet("font-weight: 500; color: #a0aec0;");
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    
    addFilterButton_ = new QPushButton("+ Add Filter");
    addFilterButton_->setObjectName("primaryButton");
    addFilterButton_->setFixedHeight(28);
    addFilterButton_->setMinimumWidth(90);
    addFilterButton_->setToolTip("Add a new filter layer");
    headerLayout->addWidget(addFilterButton_);
    groupLayout->addLayout(headerLayout);
    
    addFilterMenu_ = new QMenu(this);
    
    QAction* degreeAction = addFilterMenu_->addAction("Degree Range");
    degreeAction->setData(static_cast<int>(FilterType::Degree));
    
    QAction* betwAction = addFilterMenu_->addAction("Betweenness Centrality");
    betwAction->setData(static_cast<int>(FilterType::Betweenness));
    
    addFilterMenu_->addSeparator();
    
    QAction* largestAction = addFilterMenu_->addAction("Keep Largest Component");
    largestAction->setData(static_cast<int>(FilterType::LargestComponent));
    
    QAction* isolatedAction = addFilterMenu_->addAction("Remove Isolated Nodes");
    isolatedAction->setData(static_cast<int>(FilterType::RemoveIsolated));
    
    addFilterMenu_->addSeparator();
    
    QAction* nameAction = addFilterMenu_->addAction("Name Pattern Filter");
    nameAction->setData(static_cast<int>(FilterType::NamePattern));
    
    connect(addFilterButton_, &QPushButton::clicked, this, &GraphFilterPanel::onAddFilterClicked);
    connect(addFilterMenu_, &QMenu::triggered, this, &GraphFilterPanel::onFilterTypeSelected);
    
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMinimumHeight(120);
    scrollArea->setMaximumHeight(240);
    
    filterListContainer_ = new QWidget;
    filterListLayout_ = new QVBoxLayout(filterListContainer_);
    filterListLayout_->setContentsMargins(0, 0, 0, 0);
    filterListLayout_->setSpacing(6);
    filterListLayout_->addStretch();
    
    scrollArea->setWidget(filterListContainer_);
    groupLayout->addWidget(scrollArea);
    
    QLabel* emptyLabel = new QLabel("No filters applied.\nClick \"+ Add Filter\" to add a filter layer.");
    emptyLabel->setObjectName("emptyFiltersLabel");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #718096; font-style: italic; padding: 16px;");
    filterListLayout_->insertWidget(0, emptyLabel);
    
    QFrame* separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #2d3748; max-height: 1px;");
    groupLayout->addWidget(separator);
    
    statsLabel_ = new QLabel;
    statsLabel_->setStyleSheet("font-weight: 500; color: #a0aec0;");
    statsLabel_->setWordWrap(true);
    groupLayout->addWidget(statsLabel_);
    updateStats();
    
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(6);
    
    applyButton_ = new QPushButton("Apply Filters");
    applyButton_->setObjectName("applyFilterButton");
    applyButton_->setToolTip("Apply filters in order from top to bottom");
    applyButton_->setMinimumHeight(32);
    
    resetButton_ = new QPushButton("Reset");
    resetButton_->setObjectName("resetFilterButton");
    resetButton_->setToolTip("Clear all filters and restore original graph");
    resetButton_->setMinimumHeight(32);
    
    buttonLayout->addWidget(applyButton_, 2);
    buttonLayout->addWidget(resetButton_, 1);
    groupLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(filterGroup_);
    
    connect(applyButton_, &QPushButton::clicked, this, &GraphFilterPanel::onApplyClicked);
    connect(resetButton_, &QPushButton::clicked, this, &GraphFilterPanel::onResetClicked);
    
    setFilterEnabled(false);
}

void GraphFilterPanel::onAddFilterClicked()
{
    addFilterMenu_->exec(addFilterButton_->mapToGlobal(QPoint(0, addFilterButton_->height())));
}

void GraphFilterPanel::onFilterTypeSelected(QAction* action)
{
    FilterType type = static_cast<FilterType>(action->data().toInt());
    showFilterDialog(type);
}

void GraphFilterPanel::showFilterDialog(FilterType type, int editIndex)
{
    QDialog dialog(this);
    dialog.setWindowTitle(editIndex >= 0 ? "Edit Filter" : "Add Filter");
    dialog.setMinimumWidth(300);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(16, 16, 16, 16);
    
    QLabel* titleLabel = new QLabel(FilterLayer::typeToString(type));
    titleLabel->setStyleSheet("font-size: 11pt; font-weight: 600;");
    layout->addWidget(titleLabel);
    
    QFormLayout* formLayout = new QFormLayout;
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight);
    
    QSpinBox* degreeMinSpin = nullptr;
    QSpinBox* degreeMaxSpin = nullptr;
    QDoubleSpinBox* betwMinSpin = nullptr;
    QDoubleSpinBox* betwMaxSpin = nullptr;
    QLineEdit* patternEdit = nullptr;
    QComboBox* patternActionCombo = nullptr;
    
    FilterLayer existingLayer;
    if (editIndex >= 0 && editIndex < static_cast<int>(filters_.size())) {
        existingLayer = filters_[editIndex];
    }
    
    switch (type) {
        case FilterType::Degree:
            degreeMinSpin = new QSpinBox;
            degreeMinSpin->setRange(0, 999999);
            degreeMinSpin->setValue(editIndex >= 0 ? existingLayer.parameters.value("min", 0).toInt() : 0);
            formLayout->addRow("Min degree (\u2265):", degreeMinSpin);
            
            degreeMaxSpin = new QSpinBox;
            degreeMaxSpin->setRange(0, 999999);
            degreeMaxSpin->setValue(editIndex >= 0 ? existingLayer.parameters.value("max", originalMaxDegree_).toInt() : originalMaxDegree_);
            formLayout->addRow("Max degree (\u2264):", degreeMaxSpin);
            break;
            
        case FilterType::Betweenness:
            betwMinSpin = new QDoubleSpinBox;
            betwMinSpin->setRange(0.0, 1.0);
            betwMinSpin->setDecimals(6);
            betwMinSpin->setSingleStep(0.001);
            betwMinSpin->setValue(editIndex >= 0 ? existingLayer.parameters.value("min", 0.0).toDouble() : 0.0);
            formLayout->addRow("Min BC (\u2265):", betwMinSpin);
            
            betwMaxSpin = new QDoubleSpinBox;
            betwMaxSpin->setRange(0.0, 1.0);
            betwMaxSpin->setDecimals(6);
            betwMaxSpin->setSingleStep(0.001);
            betwMaxSpin->setValue(editIndex >= 0 ? existingLayer.parameters.value("max", 1.0).toDouble() : 1.0);
            formLayout->addRow("Max BC (\u2264):", betwMaxSpin);
            break;
            
        case FilterType::NamePattern:
            patternEdit = new QLineEdit;
            patternEdit->setPlaceholderText("e.g., BRCA* or *kinase*");
            patternEdit->setText(editIndex >= 0 ? existingLayer.parameters.value("pattern", "").toString() : "");
            formLayout->addRow("Pattern:", patternEdit);
            
            patternActionCombo = new QComboBox;
            patternActionCombo->addItem("Keep matching nodes");
            patternActionCombo->addItem("Remove matching nodes");
            patternActionCombo->setCurrentIndex(editIndex >= 0 && existingLayer.parameters.value("exclude", false).toBool() ? 1 : 0);
            formLayout->addRow("Action:", patternActionCombo);
            break;
            
        case FilterType::LargestComponent:
        case FilterType::RemoveIsolated:
            {
                QLabel* infoLabel = new QLabel;
                if (type == FilterType::LargestComponent) {
                    infoLabel->setText("This filter will keep only nodes\nin the largest connected component.");
                } else {
                    infoLabel->setText("This filter will remove all nodes\nwith zero connections.");
                }
                infoLabel->setStyleSheet("color: #a0aec0;");
                layout->addWidget(infoLabel);
            }
            break;
    }
    
    layout->addLayout(formLayout);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    if (dialog.exec() == QDialog::Accepted) {
        FilterLayer layer;
        layer.type = type;
        layer.displayName = FilterLayer::typeToString(type);
        
        switch (type) {
            case FilterType::Degree:
                layer.parameters["min"] = degreeMinSpin->value();
                layer.parameters["max"] = degreeMaxSpin->value();
                break;
            case FilterType::Betweenness:
                layer.parameters["min"] = betwMinSpin->value();
                layer.parameters["max"] = betwMaxSpin->value();
                break;
            case FilterType::NamePattern:
                if (patternEdit->text().isEmpty()) {
                    QMessageBox::warning(this, "Invalid Pattern", "Please enter a name pattern.");
                    return;
                }
                layer.parameters["pattern"] = patternEdit->text();
                layer.parameters["exclude"] = (patternActionCombo->currentIndex() == 1);
                break;
            case FilterType::LargestComponent:
            case FilterType::RemoveIsolated:
                break;
        }
        
        if (editIndex >= 0) {
            filters_[editIndex] = layer;
        } else {
            filters_.push_back(layer);
        }
        rebuildFilterList();
    }
}

void GraphFilterPanel::addFilter(const FilterLayer& layer)
{
    filters_.push_back(layer);
    rebuildFilterList();
}

void GraphFilterPanel::rebuildFilterList()
{
    QLayoutItem* item;
    while ((item = filterListLayout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    
    if (filters_.empty()) {
        QLabel* emptyLabel = new QLabel("No filters applied.\nClick \"+ Add Filter\" to add a filter layer.");
        emptyLabel->setObjectName("emptyFiltersLabel");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #718096; font-style: italic; padding: 16px;");
        filterListLayout_->addWidget(emptyLabel);
    } else {
        for (size_t i = 0; i < filters_.size(); ++i) {
            FilterLayerWidget* widget = new FilterLayerWidget(filters_[i], this);
            
            int index = static_cast<int>(i);
            connect(widget, &FilterLayerWidget::removeRequested, this, [this, index]() {
                removeFilter(index);
            });
            connect(widget, &FilterLayerWidget::moveUpRequested, this, [this, index]() {
                moveFilterUp(index);
            });
            connect(widget, &FilterLayerWidget::moveDownRequested, this, [this, index]() {
                moveFilterDown(index);
            });
            connect(widget, &FilterLayerWidget::editRequested, this, [this, index]() {
                editFilter(index);
            });
            
            filterListLayout_->addWidget(widget);
        }
    }
    
    filterListLayout_->addStretch();
    updateStats();
}

void GraphFilterPanel::removeFilter(int index)
{
    if (index >= 0 && index < static_cast<int>(filters_.size())) {
        filters_.erase(filters_.begin() + index);
        rebuildFilterList();
    }
}

void GraphFilterPanel::moveFilterUp(int index)
{
    if (index > 0 && index < static_cast<int>(filters_.size())) {
        std::swap(filters_[index], filters_[index - 1]);
        rebuildFilterList();
    }
}

void GraphFilterPanel::moveFilterDown(int index)
{
    if (index >= 0 && index < static_cast<int>(filters_.size()) - 1) {
        std::swap(filters_[index], filters_[index + 1]);
        rebuildFilterList();
    }
}

void GraphFilterPanel::editFilter(int index)
{
    if (index >= 0 && index < static_cast<int>(filters_.size())) {
        showFilterDialog(filters_[index].type, index);
    }
}

void GraphFilterPanel::updateStats()
{
    QString stats;
    if (originalNodeCount_ > 0) {
        stats = QString("Original: %1 nodes, %2 edges | Max degree: %3")
            .arg(originalNodeCount_)
            .arg(originalEdgeCount_)
            .arg(originalMaxDegree_);
        
        if (!filters_.empty()) {
            stats += QString("\n%1 filter(s) ready to apply").arg(filters_.size());
        }
    } else {
        stats = "Load a graph to enable filtering";
    }
    statsLabel_->setText(stats);
}

GraphFilterConfig GraphFilterPanel::getFilterConfig() const
{
    GraphFilterConfig config;
    config.orderedFilters = filters_;
    
    for (const auto& layer : filters_) {
        switch (layer.type) {
            case FilterType::Degree:
                config.filterByDegree = true;
                config.degreeMin = layer.parameters.value("min", 0).toInt();
                config.degreeMax = layer.parameters.value("max", 999999).toInt();
                break;
            case FilterType::Betweenness:
                config.filterByBetweenness = true;
                config.betweennessMin = layer.parameters.value("min", 0.0).toDouble();
                config.betweennessMax = layer.parameters.value("max", 1.0).toDouble();
                break;
            case FilterType::LargestComponent:
                config.keepLargestComponent = true;
                break;
            case FilterType::RemoveIsolated:
                config.removeIsolated = true;
                break;
            case FilterType::NamePattern:
                config.filterByName = true;
                config.namePattern = layer.parameters.value("pattern", "").toString();
                config.nameExclude = layer.parameters.value("exclude", false).toBool();
                break;
        }
    }
    
    return config;
}

void GraphFilterPanel::updateGraphStats(int maxDegree, int nodeCount, int edgeCount)
{
    originalMaxDegree_ = maxDegree;
    originalNodeCount_ = nodeCount;
    originalEdgeCount_ = edgeCount;
    updateStats();
}

void GraphFilterPanel::resetFilters()
{
    filters_.clear();
    rebuildFilterList();
}

void GraphFilterPanel::setFilterEnabled(bool enabled)
{
    filterGroup_->setEnabled(enabled);
}

void GraphFilterPanel::onApplyClicked()
{
    GraphFilterConfig config = getFilterConfig();
    emit applyFiltersRequested(config);
}

void GraphFilterPanel::onResetClicked()
{
    resetFilters();
    emit resetFiltersRequested();
}
