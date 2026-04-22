/**
 * @file SplitGraphView.cpp
 * @brief Implementation of split view for graph attribute comparison
 */

#include "SplitGraphView.h"
#include "DarkTheme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QLinearGradient>
#include <QWheelEvent>
#include <QScrollBar>
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <queue>
#include <stack>
#include <set>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <limits>

class ColorLegendWidget : public QWidget {
public:
    ColorLegendWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedHeight(30);
        setMinimumWidth(200);
    }
    
    void setRange(double minVal, double maxVal, const QString& label) {
        minVal_ = minVal;
        maxVal_ = maxVal;
        label_ = label;
        update();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int gradientWidth = width() - 80;
        int gradientHeight = 14;
        int y = (height() - gradientHeight) / 2;
        
        QLinearGradient gradient(40, 0, 40 + gradientWidth, 0);
        gradient.setColorAt(0.0, QColor(59, 130, 246));
        gradient.setColorAt(0.5, QColor(250, 204, 21));
        gradient.setColorAt(1.0, QColor(239, 68, 68));
        
        p.setPen(Qt::NoPen);
        p.setBrush(gradient);
        p.drawRoundedRect(40, y, gradientWidth, gradientHeight, 3, 3);
        
        p.setPen(QPen(QColor(100, 100, 100), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(40, y, gradientWidth, gradientHeight, 3, 3);
        
        p.setPen(QColor(200, 200, 200));
        p.setFont(QFont("Segoe UI", 8));
        
        QString minStr = QString::number(minVal_, 'f', 2);
        QString maxStr = QString::number(maxVal_, 'f', 2);
        
        p.drawText(QRect(0, 0, 38, height()), Qt::AlignRight | Qt::AlignVCenter, minStr);
        p.drawText(QRect(42 + gradientWidth, 0, 38, height()), Qt::AlignLeft | Qt::AlignVCenter, maxStr);
    }
    
private:
    double minVal_ = 0.0;
    double maxVal_ = 1.0;
    QString label_;
};

SplitGraphView::SplitGraphView(QWidget* parent)
    : QWidget(parent)
    , splitEnabled_(false)
    , syncLayout_(true)
    , primaryAttribute_(NodeColorAttribute::None)
    , secondaryAttribute_(NodeColorAttribute::Betweenness)
{
    setupUI();
    setupConnections();
}

SplitGraphView::~SplitGraphView()
{
}

void SplitGraphView::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);
    
    controlPanel_ = new QGroupBox("Split View Controls");
    controlPanel_->setStyleSheet(QString(
        "QGroupBox {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 6px;"
        "  margin-top: 8px;"
        "  padding: 8px;"
        "  font-weight: bold;"
        "  color: %3;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 5px;"
        "}"
    ).arg(DarkTheme::Colors::BG_SECONDARY)
     .arg(DarkTheme::Colors::BORDER)
     .arg(DarkTheme::Colors::TEXT_PRIMARY));
    
    QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel_);
    controlLayout->setSpacing(16);
    
    splitViewCheckBox_ = new QCheckBox("Enable Split View");
    splitViewCheckBox_->setStyleSheet(QString("color: %1; font-weight: normal;").arg(DarkTheme::Colors::TEXT_PRIMARY));
    controlLayout->addWidget(splitViewCheckBox_);
    
    controlLayout->addSpacing(20);
    
    primaryLabel_ = new QLabel("Left View:");
    primaryLabel_->setStyleSheet(QString("color: %1; font-weight: normal;").arg(DarkTheme::Colors::TEXT_SECONDARY));
    controlLayout->addWidget(primaryLabel_);
    
    primaryAttributeCombo_ = new QComboBox;
    primaryAttributeCombo_->addItem("Seeds/Connectors", static_cast<int>(NodeColorAttribute::None));
    primaryAttributeCombo_->addItem("Degree", static_cast<int>(NodeColorAttribute::Degree));
    primaryAttributeCombo_->addItem("Betweenness Centrality", static_cast<int>(NodeColorAttribute::Betweenness));
    primaryAttributeCombo_->addItem("Closeness Centrality", static_cast<int>(NodeColorAttribute::Closeness));
    primaryAttributeCombo_->addItem("Clustering Coefficient", static_cast<int>(NodeColorAttribute::ClusteringCoeff));
    primaryAttributeCombo_->addItem("PageRank", static_cast<int>(NodeColorAttribute::PageRank));
    primaryAttributeCombo_->setMinimumWidth(160);
    controlLayout->addWidget(primaryAttributeCombo_);
    
    controlLayout->addSpacing(20);
    
    secondaryLabel_ = new QLabel("Right View:");
    secondaryLabel_->setStyleSheet(QString("color: %1; font-weight: normal;").arg(DarkTheme::Colors::TEXT_SECONDARY));
    controlLayout->addWidget(secondaryLabel_);
    
    secondaryAttributeCombo_ = new QComboBox;
    secondaryAttributeCombo_->addItem("Seeds/Connectors", static_cast<int>(NodeColorAttribute::None));
    secondaryAttributeCombo_->addItem("Degree", static_cast<int>(NodeColorAttribute::Degree));
    secondaryAttributeCombo_->addItem("Betweenness Centrality", static_cast<int>(NodeColorAttribute::Betweenness));
    secondaryAttributeCombo_->addItem("Closeness Centrality", static_cast<int>(NodeColorAttribute::Closeness));
    secondaryAttributeCombo_->addItem("Clustering Coefficient", static_cast<int>(NodeColorAttribute::ClusteringCoeff));
    secondaryAttributeCombo_->addItem("PageRank", static_cast<int>(NodeColorAttribute::PageRank));
    secondaryAttributeCombo_->setCurrentIndex(2);
    secondaryAttributeCombo_->setMinimumWidth(160);
    controlLayout->addWidget(secondaryAttributeCombo_);
    
    controlLayout->addSpacing(20);
    
    syncLayoutCheckBox_ = new QCheckBox("Sync Layout");
    syncLayoutCheckBox_->setChecked(true);
    syncLayoutCheckBox_->setStyleSheet(QString("color: %1; font-weight: normal;").arg(DarkTheme::Colors::TEXT_PRIMARY));
    syncLayoutCheckBox_->setToolTip("Keep node positions synchronized between views");
    controlLayout->addWidget(syncLayoutCheckBox_);
    
    syncNowButton_ = new QPushButton("Sync Now");
    syncNowButton_->setFixedWidth(80);
    syncNowButton_->setStyleSheet(DarkTheme::Styles::primaryButton());
    controlLayout->addWidget(syncNowButton_);
    
    controlLayout->addStretch();
    
    mainLayout->addWidget(controlPanel_);
    
    splitter_ = new QSplitter(Qt::Horizontal);
    splitter_->setHandleWidth(6);
    splitter_->setStyleSheet(QString(
        "QSplitter::handle {"
        "  background: %1;"
        "  border-radius: 2px;"
        "}"
        "QSplitter::handle:hover {"
        "  background: %2;"
        "}"
    ).arg(DarkTheme::Colors::BORDER).arg(DarkTheme::Colors::ACCENT));
    
    primaryContainer_ = new QWidget;
    QVBoxLayout* primaryLayout = new QVBoxLayout(primaryContainer_);
    primaryLayout->setContentsMargins(0, 0, 0, 0);
    primaryLayout->setSpacing(2);
    
    primaryGraph_ = new GraphVisualization(this);
    primaryLayout->addWidget(primaryGraph_, 1);
    
    primaryLegend_ = new ColorLegendWidget(this);
    primaryLegend_->setVisible(false);
    primaryLayout->addWidget(primaryLegend_);
    
    splitter_->addWidget(primaryContainer_);
    
    secondaryContainer_ = new QWidget;
    QVBoxLayout* secondaryLayout = new QVBoxLayout(secondaryContainer_);
    secondaryLayout->setContentsMargins(0, 0, 0, 0);
    secondaryLayout->setSpacing(2);
    
    secondaryGraph_ = new GraphVisualization(this);
    secondaryGraph_->setVisualizationEnabled(true);
    secondaryLayout->addWidget(secondaryGraph_, 1);
    
    secondaryLegend_ = new ColorLegendWidget(this);
    secondaryLegend_->setVisible(false);
    secondaryLayout->addWidget(secondaryLegend_);
    
    splitter_->addWidget(secondaryContainer_);
    
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 1);
    
    secondaryContainer_->setVisible(false);
    
    controlPanel_->setVisible(true);
    
    mainLayout->addWidget(splitter_, 1);
    
    primaryLabel_->setEnabled(false);
    primaryAttributeCombo_->setEnabled(false);
    secondaryLabel_->setEnabled(false);
    secondaryAttributeCombo_->setEnabled(false);
    syncLayoutCheckBox_->setEnabled(false);
    syncNowButton_->setEnabled(false);
}

void SplitGraphView::setupConnections()
{
    connect(splitViewCheckBox_, &QCheckBox::toggled, this, &SplitGraphView::setSplitViewEnabled);
    connect(primaryAttributeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SplitGraphView::onPrimaryAttributeChanged);
    connect(secondaryAttributeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SplitGraphView::onSecondaryAttributeChanged);
    connect(syncLayoutCheckBox_, &QCheckBox::toggled, this, &SplitGraphView::onSyncLayoutToggled);
    connect(syncNowButton_, &QPushButton::clicked, this, &SplitGraphView::syncPositions);
    
    connect(primaryGraph_, &GraphVisualization::nodeClicked, this, &SplitGraphView::nodeClicked);
    connect(secondaryGraph_, &GraphVisualization::nodeClicked, this, &SplitGraphView::nodeClicked);
    
    primaryGraph_->viewport()->installEventFilter(this);
    secondaryGraph_->viewport()->installEventFilter(this);
}

bool SplitGraphView::eventFilter(QObject* watched, QEvent* event)
{
    if (!splitEnabled_ || !syncLayout_ || isSyncing_) {
        return QWidget::eventFilter(watched, event);
    }
    
    if (event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        
        double angle = wheelEvent->angleDelta().y();
        double factor = (angle > 0) ? 1.15 : 1.0 / 1.15;
        
        isSyncing_ = true;
        
        if (watched == primaryGraph_->viewport()) {
            secondaryGraph_->scale(factor, factor);
        } else if (watched == secondaryGraph_->viewport()) {
            primaryGraph_->scale(factor, factor);
        }
        
        isSyncing_ = false;
    }
    
    return QWidget::eventFilter(watched, event);
}

void SplitGraphView::setGraph(const Graph& graph)
{
    graph_ = graph;
    cachedAttributes_.clear();
    
    primaryGraph_->setGraph(graph);
    secondaryGraph_->setGraph(graph);
    
    if (splitEnabled_) {
        secondaryGraph_->setLayoutParameters(primaryGraph_->getLayoutParameters());
        secondaryGraph_->visualizeGraph();
        
        auto positions = primaryGraph_->getNodePositions();
        if (!positions.empty()) {
            secondaryGraph_->setNodePositions(positions);
        }
        syncCamera();
        
        if (secondaryAttribute_ != NodeColorAttribute::None) {
            auto values = computeAttribute(secondaryAttribute_);
            applyAttributeColoring(secondaryGraph_, secondaryAttribute_, values);
        }
    }
}

void SplitGraphView::displayGraph(const Graph& graph)
{
    graph_ = graph;
    cachedAttributes_.clear();
    
    primaryGraph_->displayGraph(graph);
    
    if (splitEnabled_ && secondaryContainer_->isVisible()) {
        QTimer::singleShot(300, [this]() {
            if (splitEnabled_ && secondaryContainer_->isVisible() && graph_.getNodeCount() > 0) {
                secondaryGraph_->setVisualizationEnabled(true);
                secondaryGraph_->displayGraph(graph_);
                
                auto positions = primaryGraph_->getNodePositions();
                if (!positions.empty()) {
                    secondaryGraph_->setNodePositions(positions);
                }
                syncCamera();
                
                if (secondaryAttribute_ != NodeColorAttribute::None) {
                    auto values = computeAttribute(secondaryAttribute_);
                    applyAttributeColoring(secondaryGraph_, secondaryAttribute_, values);
                }
            }
        });
    }
}

void SplitGraphView::setCurrentSeeds(const std::vector<std::string>& seeds)
{
    primaryGraph_->setCurrentSeeds(seeds);
    secondaryGraph_->setCurrentSeeds(seeds);
}

void SplitGraphView::setConnectorFrequencyData(const std::map<int, int>& freq, int totalPerm)
{
    primaryGraph_->setConnectorFrequencyData(freq, totalPerm);
    secondaryGraph_->setConnectorFrequencyData(freq, totalPerm);
}

void SplitGraphView::highlightSeedsByName(const std::vector<std::string>& seedNames)
{
    primaryGraph_->highlightSeedsByName(seedNames);
    if (splitEnabled_ && primaryAttribute_ == NodeColorAttribute::None) {
        secondaryGraph_->highlightSeedsByName(seedNames);
    }
}

void SplitGraphView::highlightConnectorsWithFrequency(
    const std::map<int, int>& connectorFrequency,
    int totalPermutations,
    const std::set<int>& bestConnectors,
    const QColor& connectorColor,
    const QColor& extraConnectorColor)
{
    storedConnectorFrequency_ = connectorFrequency;
    storedTotalPermutations_ = totalPermutations;
    storedBestConnectors_ = bestConnectors;
    storedConnectorColor_ = connectorColor;
    storedExtraConnectorColor_ = extraConnectorColor;
    hasConnectorData_ = true;
    
    primaryGraph_->highlightConnectorsWithFrequency(connectorFrequency, totalPermutations,
                                                     bestConnectors, connectorColor, extraConnectorColor);
    
    if (splitEnabled_ && secondaryAttribute_ == NodeColorAttribute::None) {
        secondaryGraph_->highlightConnectorsWithFrequency(connectorFrequency, totalPermutations,
                                                          bestConnectors, connectorColor, extraConnectorColor);
    }
}

void SplitGraphView::setSplitViewEnabled(bool enabled)
{
    splitEnabled_ = enabled;
    secondaryContainer_->setVisible(enabled);
    
    primaryLabel_->setEnabled(enabled);
    primaryAttributeCombo_->setEnabled(enabled);
    secondaryLabel_->setEnabled(enabled);
    secondaryAttributeCombo_->setEnabled(enabled);
    syncLayoutCheckBox_->setEnabled(enabled);
    syncNowButton_->setEnabled(enabled);
    
    if (enabled) {
        
        QTimer::singleShot(50, [this]() {
            int totalWidth = splitter_->width();
            if (totalWidth > 100) {
                splitter_->setSizes({totalWidth / 2, totalWidth / 2});
            }
        });
        
        QTimer::singleShot(100, [this]() {
            if (!splitEnabled_ || !secondaryContainer_->isVisible()) return;
            
            secondaryGraph_->setVisualizationEnabled(true);
            
            if (graph_.getNodeCount() > 0) {
                secondaryGraph_->setGraph(graph_);
                secondaryGraph_->setLayoutParameters(primaryGraph_->getLayoutParameters());
                secondaryGraph_->setLayoutType(primaryGraph_->getLayoutType());
                
                secondaryGraph_->visualizeGraph();
                
                auto positions = primaryGraph_->getNodePositions();
                if (!positions.empty()) {
                    secondaryGraph_->setNodePositions(positions);
                }
                
                QRectF bounds = primaryGraph_->scene()->itemsBoundingRect();
                bounds.adjust(-50, -50, 50, 50);
                primaryGraph_->fitInView(bounds, Qt::KeepAspectRatio);
                secondaryGraph_->fitInView(bounds, Qt::KeepAspectRatio);
                
                if (hasConnectorData_ && secondaryAttribute_ == NodeColorAttribute::None) {
                    secondaryGraph_->highlightConnectorsWithFrequency(
                        storedConnectorFrequency_, storedTotalPermutations_,
                        storedBestConnectors_, storedConnectorColor_, storedExtraConnectorColor_);
                }
                
                if (secondaryAttribute_ != NodeColorAttribute::None) {
                    auto values = computeAttribute(secondaryAttribute_);
                    applyAttributeColoring(secondaryGraph_, secondaryAttribute_, values);
                }
                
            }
        });
    }
    
    splitViewCheckBox_->setChecked(enabled);
}

void SplitGraphView::setLayoutParameters(const LayoutParameters& params)
{
    primaryGraph_->setLayoutParameters(params);
    secondaryGraph_->setLayoutParameters(params);
}

void SplitGraphView::applyLayout()
{
    primaryGraph_->visualizeGraph();
    
    if (splitEnabled_) {
        QTimer::singleShot(150, [this]() {
            if (!splitEnabled_) return;
            
            auto positions = primaryGraph_->getNodePositions();
            if (!positions.empty()) {
                secondaryGraph_->setNodePositions(positions);
            }
            
            secondaryGraph_->setTransform(primaryGraph_->transform());
            
            primaryGraph_->fitInView(primaryGraph_->sceneRect(), Qt::KeepAspectRatio);
            secondaryGraph_->fitInView(secondaryGraph_->sceneRect(), Qt::KeepAspectRatio);
            
            if (secondaryAttribute_ != NodeColorAttribute::None) {
                auto values = computeAttribute(secondaryAttribute_);
                applyAttributeColoring(secondaryGraph_, secondaryAttribute_, values);
            }
        });
    }
}

void SplitGraphView::syncCamera()
{
    if (!splitEnabled_) return;
    
    secondaryGraph_->setTransform(primaryGraph_->transform());
}

void SplitGraphView::setLayoutType(LayoutType type)
{
    primaryGraph_->setLayoutType(type);
    secondaryGraph_->setLayoutType(type);
}

void SplitGraphView::toggleSplitView()
{
    setSplitViewEnabled(!splitEnabled_);
}

void SplitGraphView::syncPositions()
{
    if (!splitEnabled_) return;
    
    auto positions = primaryGraph_->getNodePositions();
    if (!positions.empty()) {
        secondaryGraph_->setNodePositions(positions);
    }
    
    syncCamera();
    
    if (secondaryAttribute_ != NodeColorAttribute::None) {
        auto values = computeAttribute(secondaryAttribute_);
        applyAttributeColoring(secondaryGraph_, secondaryAttribute_, values);
    }
}

void SplitGraphView::onPrimaryAttributeChanged(int index)
{
    primaryAttribute_ = static_cast<NodeColorAttribute>(primaryAttributeCombo_->itemData(index).toInt());
    
    if (primaryAttribute_ == NodeColorAttribute::None) {
        primaryGraph_->visualizeGraph();
        primaryLegend_->setVisible(false);
    } else {
        auto values = computeAttribute(primaryAttribute_);
        applyAttributeColoring(primaryGraph_, primaryAttribute_, values);
        
        if (!values.empty()) {
            double minVal = std::numeric_limits<double>::max();
            double maxVal = std::numeric_limits<double>::lowest();
            for (const auto& [id, val] : values) {
                if (val < minVal) minVal = val;
                if (val > maxVal) maxVal = val;
            }
            static_cast<ColorLegendWidget*>(primaryLegend_)->setRange(minVal, maxVal, 
                primaryAttributeCombo_->currentText());
            primaryLegend_->setVisible(true);
        }
    }
}

void SplitGraphView::onSecondaryAttributeChanged(int index)
{
    secondaryAttribute_ = static_cast<NodeColorAttribute>(secondaryAttributeCombo_->itemData(index).toInt());
    
    if (!splitEnabled_) return;
    
    if (secondaryAttribute_ == NodeColorAttribute::None) {
        secondaryGraph_->visualizeGraph();
        secondaryLegend_->setVisible(false);
    } else {
        auto values = computeAttribute(secondaryAttribute_);
        applyAttributeColoring(secondaryGraph_, secondaryAttribute_, values);
        
        if (!values.empty()) {
            double minVal = std::numeric_limits<double>::max();
            double maxVal = std::numeric_limits<double>::lowest();
            for (const auto& [id, val] : values) {
                if (val < minVal) minVal = val;
                if (val > maxVal) maxVal = val;
            }
            static_cast<ColorLegendWidget*>(secondaryLegend_)->setRange(minVal, maxVal,
                secondaryAttributeCombo_->currentText());
            secondaryLegend_->setVisible(true);
        }
    }
}

void SplitGraphView::onSyncLayoutToggled(bool sync)
{
    syncLayout_ = sync;
    if (sync && splitEnabled_) {
        syncPositions();
    }
}

std::map<int, double> SplitGraphView::computeAttribute(NodeColorAttribute attr)
{
    auto it = cachedAttributes_.find(attr);
    if (it != cachedAttributes_.end()) {
        return it->second;
    }
    
    std::map<int, double> result;
    
    switch (attr) {
        case NodeColorAttribute::Degree:
            result = computeDegree();
            break;
        case NodeColorAttribute::Betweenness:
            result = computeBetweenness();
            break;
        case NodeColorAttribute::Closeness:
            result = computeCloseness();
            break;
        case NodeColorAttribute::ClusteringCoeff:
            result = computeClusteringCoeff();
            break;
        case NodeColorAttribute::PageRank:
            result = computePageRank();
            break;
        default:
            break;
    }
    
    cachedAttributes_[attr] = result;
    return result;
}

void SplitGraphView::applyAttributeColoring(GraphVisualization* view,
                                             NodeColorAttribute attr,
                                             const std::map<int, double>& values)
{
    if (values.empty()) return;
    
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();
    
    for (const auto& [id, val] : values) {
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    
    double range = maxVal - minVal;
    if (range < 1e-10) range = 1.0;
    
    for (const auto& [nodeId, value] : values) {
        double normalized = (value - minVal) / range;
        QColor color = valueToColor(normalized);
        view->highlightNodes({nodeId}, color);
    }
}

QColor SplitGraphView::valueToColor(double normalizedValue)
{
    normalizedValue = std::clamp(normalizedValue, 0.0, 1.0);
    
    int r, g, b;
    
    if (normalizedValue < 0.5) {
        double t = normalizedValue * 2.0;
        r = static_cast<int>(59 + (250 - 59) * t);
        g = static_cast<int>(130 + (204 - 130) * t);
        b = static_cast<int>(246 + (21 - 246) * t);
    } else {
        double t = (normalizedValue - 0.5) * 2.0;
        r = static_cast<int>(250 + (239 - 250) * t);
        g = static_cast<int>(204 + (68 - 204) * t);
        b = static_cast<int>(21 + (68 - 21) * t);
    }
    
    return QColor(r, g, b);
}

std::map<int, double> SplitGraphView::computeDegree()
{
    std::map<int, double> result;
    auto nodes = graph_.getNodes();
    
    for (int nodeId : nodes) {
        result[nodeId] = static_cast<double>(graph_.getNeighbors(nodeId).size());
    }
    
    return result;
}

std::map<int, double> SplitGraphView::computeBetweenness()
{
    std::map<int, double> result;
    auto nodes = graph_.getNodes();
    auto edges = graph_.getEdges();
    
    for (int nodeId : nodes) {
        result[nodeId] = 0.0;
    }
    
    std::map<int, std::vector<int>> adj;
    for (const auto& edge : edges) {
        adj[edge.first].push_back(edge.second);
        adj[edge.second].push_back(edge.first);
    }
    
    for (int s : nodes) {
        std::stack<int> S;
        std::map<int, std::vector<int>> P;
        std::map<int, double> sigma;
        std::map<int, int> d;
        
        for (int v : nodes) {
            P[v] = {};
            sigma[v] = 0.0;
            d[v] = -1;
        }
        
        sigma[s] = 1.0;
        d[s] = 0;
        
        std::queue<int> Q;
        Q.push(s);
        
        while (!Q.empty()) {
            int v = Q.front();
            Q.pop();
            S.push(v);
            
            for (int w : adj[v]) {
                if (d[w] < 0) {
                    Q.push(w);
                    d[w] = d[v] + 1;
                }
                if (d[w] == d[v] + 1) {
                    sigma[w] += sigma[v];
                    P[w].push_back(v);
                }
            }
        }
        
        std::map<int, double> delta;
        for (int v : nodes) {
            delta[v] = 0.0;
        }
        
        while (!S.empty()) {
            int w = S.top();
            S.pop();
            for (int v : P[w]) {
                delta[v] += (sigma[v] / sigma[w]) * (1.0 + delta[w]);
            }
            if (w != s) {
                result[w] += delta[w];
            }
        }
    }
    
    int n = nodes.size();
    if (n > 2) {
        double norm = 2.0 / ((n - 1) * (n - 2));
        for (auto& [id, val] : result) {
            val *= norm;
        }
    }
    
    return result;
}

std::map<int, double> SplitGraphView::computeCloseness()
{
    std::map<int, double> result;
    auto nodes = graph_.getNodes();
    auto edges = graph_.getEdges();
    
    std::map<int, std::vector<int>> adj;
    for (const auto& edge : edges) {
        adj[edge.first].push_back(edge.second);
        adj[edge.second].push_back(edge.first);
    }
    
    for (int s : nodes) {
        std::map<int, int> dist;
        for (int v : nodes) dist[v] = -1;
        
        std::queue<int> Q;
        Q.push(s);
        dist[s] = 0;
        
        int totalDist = 0;
        int reachable = 0;
        
        while (!Q.empty()) {
            int v = Q.front();
            Q.pop();
            
            for (int w : adj[v]) {
                if (dist[w] < 0) {
                    dist[w] = dist[v] + 1;
                    totalDist += dist[w];
                    reachable++;
                    Q.push(w);
                }
            }
        }
        
        if (totalDist > 0) {
            result[s] = static_cast<double>(reachable) / totalDist;
        } else {
            result[s] = 0.0;
        }
    }
    
    return result;
}

std::map<int, double> SplitGraphView::computeClusteringCoeff()
{
    std::map<int, double> result;
    auto nodes = graph_.getNodes();
    auto edges = graph_.getEdges();
    
    std::map<int, std::set<int>> adj;
    for (const auto& edge : edges) {
        adj[edge.first].insert(edge.second);
        adj[edge.second].insert(edge.first);
    }
    
    for (int v : nodes) {
        const auto& neighbors = adj[v];
        int k = neighbors.size();
        
        if (k < 2) {
            result[v] = 0.0;
            continue;
        }
        
        int triangles = 0;
        for (int u : neighbors) {
            for (int w : neighbors) {
                if (u < w && adj[u].count(w)) {
                    triangles++;
                }
            }
        }
        
        result[v] = (2.0 * triangles) / (k * (k - 1));
    }
    
    return result;
}

std::map<int, double> SplitGraphView::computePageRank()
{
    std::map<int, double> result;
    auto nodes = graph_.getNodes();
    auto edges = graph_.getEdges();
    
    int n = nodes.size();
    if (n == 0) return result;
    
    std::map<int, std::vector<int>> inLinks;
    std::map<int, int> outDegree;
    
    for (int v : nodes) {
        inLinks[v] = {};
        outDegree[v] = 0;
    }
    
    for (const auto& edge : edges) {
        inLinks[edge.second].push_back(edge.first);
        inLinks[edge.first].push_back(edge.second);
        outDegree[edge.first]++;
        outDegree[edge.second]++;
    }
    
    double d = 0.85;
    for (int v : nodes) {
        result[v] = 1.0 / n;
    }
    
    for (int iter = 0; iter < 100; ++iter) {
        std::map<int, double> newPR;
        
        for (int v : nodes) {
            double sum = 0.0;
            for (int u : inLinks[v]) {
                if (outDegree[u] > 0) {
                    sum += result[u] / outDegree[u];
                }
            }
            newPR[v] = (1.0 - d) / n + d * sum;
        }
        
        result = newPR;
    }
    
    return result;
}
