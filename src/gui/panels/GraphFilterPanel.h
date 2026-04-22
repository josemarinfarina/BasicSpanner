/**
 * @file GraphFilterPanel.h
 * @brief Panel for filtering graph nodes with a layer-based system
 */

#ifndef GRAPHFILTERPANEL_H
#define GRAPHFILTERPANEL_H

#include <QWidget>
#include <QString>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QMenu>
#include <QVariant>
#include <QVBoxLayout>
#include <vector>

enum class FilterType {
    Degree,
    Betweenness,
    LargestComponent,
    RemoveIsolated,
    NamePattern
};

struct FilterLayer {
    FilterType type;
    QString displayName;
    QVariantMap parameters;
    
    static QString typeToString(FilterType type);
    static QString typeToIcon(FilterType type);
};

struct GraphFilterConfig {
    bool filterByDegree = false;
    int degreeMin = 0;
    int degreeMax = 999999;
    
    bool filterByBetweenness = false;
    double betweennessMin = 0.0;
    double betweennessMax = 1.0;
    
    bool keepLargestComponent = false;
    bool removeIsolated = false;
    
    bool filterByName = false;
    QString namePattern;
    bool nameExclude = false;
    
    std::vector<FilterLayer> orderedFilters;
};

class FilterLayerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FilterLayerWidget(const FilterLayer& layer, QWidget* parent = nullptr);
    FilterLayer getLayer() const { return layer_; }
    void setLayer(const FilterLayer& layer);

signals:
    void removeRequested();
    void moveUpRequested();
    void moveDownRequested();
    void editRequested();

private:
    void setupUI();
    void updateDisplay();
    
    FilterLayer layer_;
    QLabel* iconLabel_;
    QLabel* nameLabel_;
    QLabel* detailsLabel_;
    QPushButton* editButton_;
    QPushButton* moveUpButton_;
    QPushButton* moveDownButton_;
    QPushButton* removeButton_;
};

class GraphFilterPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GraphFilterPanel(QWidget *parent = nullptr);
    
    GraphFilterConfig getFilterConfig() const;
    void updateGraphStats(int maxDegree, int nodeCount, int edgeCount);
    void resetFilters();
    void setFilterEnabled(bool enabled);

signals:
    void applyFiltersRequested(const GraphFilterConfig& config);
    void resetFiltersRequested();

private slots:
    void onAddFilterClicked();
    void onApplyClicked();
    void onResetClicked();
    void onFilterTypeSelected(QAction* action);
    void removeFilter(int index);
    void moveFilterUp(int index);
    void moveFilterDown(int index);
    void editFilter(int index);

private:
    void setupUI();
    void rebuildFilterList();
    void updateStats();
    void showFilterDialog(FilterType type, int editIndex = -1);
    void addFilter(const FilterLayer& layer);
    
    std::vector<FilterLayer> filters_;
    
    QGroupBox* filterGroup_;
    QWidget* filterListContainer_;
    QVBoxLayout* filterListLayout_;
    QPushButton* addFilterButton_;
    QMenu* addFilterMenu_;
    
    QLabel* statsLabel_;
    QPushButton* applyButton_;
    QPushButton* resetButton_;
    
    int originalNodeCount_ = 0;
    int originalEdgeCount_ = 0;
    int originalMaxDegree_ = 0;
};

#endif
