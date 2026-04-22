#include "GraphVisualization.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QApplication>
#include <cmath>
#include <random>
#include <queue>
#include <stack>
#include <set>
#include <limits>
#include <thread>
#include <chrono>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GraphVisualization::GraphVisualization(QWidget *parent)
    : QGraphicsView(parent), scene_(nullptr), currentLayoutType_(LayoutType::Automatic)
{
    try {
        scene_ = new QGraphicsScene(this);
        if (!scene_) {
            return;
        }
        
        setScene(scene_);
        
        setRenderHint(QPainter::Antialiasing, true);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing | QGraphicsView::DontSavePainterState);
        setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
        setCacheMode(QGraphicsView::CacheBackground);
        setMouseTracking(true);
        viewport()->setMouseTracking(true);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        
        
        nodeAppearance_.color = QColor(0, 212, 170);
        nodeAppearance_.borderColor = QColor(45, 55, 72);
        nodeAppearance_.radius = 12.0;
        nodeAppearance_.borderWidth = 1.0;
        nodeAppearance_.textColor = QColor(226, 232, 240);
        nodeAppearance_.fontSize = 8;
        nodeAppearance_.showLabels = false;
        
        edgeAppearance_.color = QColor(74, 85, 104);
        edgeAppearance_.width = 0.8;
        edgeAppearance_.cosmetic = true;
        
        setStyleSheet(
            "QGraphicsView {"
            "    background-color: #0d1117;"
            "    border: 1px solid #2d3748;"
            "}"
        );
        scene_->setBackgroundBrush(QColor(13, 17, 23));
    } catch (...) {
        scene_ = nullptr;
    }
}

GraphVisualization::~GraphVisualization()
{
    if (scene_) {
        try {
            scene_->clear();
        } catch (...) {}
    }
    nodeItems_.clear();
    edgeItems_.clear();
    nodePositions_.clear();
    highlightedNodeIds_.clear();
}

void GraphVisualization::setGraph(const Graph& graph) {
    if (!scene_ || !visualizationEnabled_) {
        return;
    }
    
    int nodeCount = graph.getNodeCount();
    int edgeCount = graph.getEdgeCount();
    
    if (nodeCount == 0) {
        try {
            scene_->clear();
            nodeItems_.clear();
            edgeItems_.clear();
        } catch (...) {}
        return;
    }
    
    QApplication::processEvents();
    
    graph_ = graph;
    
    nodePositions_.clear();
    
    QApplication::processEvents();
    
    bool isLargeGraph = (nodeCount > 1000 || edgeCount > 5000);
    bool isVeryLargeGraph = (nodeCount > 5000 || edgeCount > 50000);
    
    if (isVeryLargeGraph) {
        setRenderHint(QPainter::Antialiasing, false);
        setRenderHint(QPainter::TextAntialiasing, false);
        setRenderHint(QPainter::SmoothPixmapTransform, false);
        nodeAppearance_.showLabels = false;
        nodeAppearance_.radius = 4.0;
        edgeAppearance_.width = 1.0;
    } else if (isLargeGraph) {
        setRenderHint(QPainter::Antialiasing, false);
        setRenderHint(QPainter::TextAntialiasing, false);
        nodeAppearance_.showLabels = false;
        nodeAppearance_.radius = 6.0;
        edgeAppearance_.width = 1.0;
    } else if (nodeCount > 100) {
        setRenderHint(QPainter::Antialiasing, true);
        nodeAppearance_.showLabels = false;
        edgeAppearance_.width = 1.2;
    } else {
        setRenderHint(QPainter::Antialiasing, true);
        edgeAppearance_.width = 1.5;
    }
    
    try {
        layoutGraph();
    } catch (...) {}
}

void GraphVisualization::displayGraph(const Graph& graph)
{
    if (!scene_) {
        return;
    }
    
    try {
        setGraph(graph);
    } catch (...) {}
}

void GraphVisualization::highlightNodes(const std::set<int>& nodeIds, const QColor& color)
{
    if (nodeItems_.empty()) return;
    
    highlightedNodeIds_.clear();
    for (int nodeId : nodeIds) {
        highlightedNodeIds_.insert(nodeId);
    }
    
    for (int nodeId : nodeIds) {
        auto it = nodeItems_.find(nodeId);
        if (it != nodeItems_.end() && it->second) {
            QPen pen(color.darker(120), 3);
            it->second->setPen(pen);
            
            it->second->setBrush(QBrush(color));
            
            QRectF rect = it->second->rect();
            double expand = 2.0;
            it->second->setRect(rect.adjusted(-expand, -expand, expand, expand));
            
            it->second->setZValue(10);
        }
    }
    
    updateEdgeOpacity();
}

void GraphVisualization::clearHighlights()
{
    highlightedNodeIds_.clear();
    for (auto& pair : nodeItems_) {
        QPen pen(nodeAppearance_.borderColor, nodeAppearance_.borderWidth);
        pair.second->setPen(pen);
        pair.second->setBrush(QBrush(nodeAppearance_.color));
    }
}

void GraphVisualization::setCurrentSeeds(const std::vector<std::string>& seeds)
{
    currentSeeds_ = seeds;
    if (!nodeItems_.empty()) {
        highlightSeedsByName(seeds);
    }
}

void GraphVisualization::highlightSeedsByName(const std::vector<std::string>& seedNames)
{
    currentSeeds_ = seedNames;
    
    clearHighlights();
    
    if (seedNames.empty() || graph_.getNodeCount() == 0 || nodeItems_.empty()) {
        return;
    }
    
    std::set<int> seedIds;
    for (const auto& name : seedNames) {
        int nodeId = graph_.getNodeId(name);
        if (nodeId >= 0) {
            seedIds.insert(nodeId);
        }
    }
    
    if (seedIds.empty()) {
        return;
    }
    
    highlightNodes(seedIds, nodeAppearance_.seedColor);
    
    if (scene_) {
        scene_->update();
    }
}

void GraphVisualization::highlightConnectorsWithFrequency(const std::map<int, int>& connectorFrequency,
                                                          int totalPermutations,
                                                          const std::set<int>& bestConnectors,
                                                          const QColor& connectorColor,
                                                          const QColor& extraConnectorColor)
{
    if (nodeItems_.empty() || connectorFrequency.empty() || totalPermutations <= 0) {
        return;
    }

    int coloredBest = 0, coloredExtra = 0, notFound = 0;
    for (const auto& [nodeId, count] : connectorFrequency) {
        auto it = nodeItems_.find(nodeId);
        if (it != nodeItems_.end() && it->second) {
            double frequency = static_cast<double>(count) / totalPermutations;

            bool isInBestResult = (bestConnectors.find(nodeId) != bestConnectors.end());
            const QColor& nodeColor = isInBestResult ? connectorColor : extraConnectorColor;

            QPen pen(nodeColor.darker(120), 3);
            it->second->setPen(pen);
            it->second->setBrush(QBrush(nodeColor));

            QRectF rect = it->second->rect();
            double expand = 2.0;
            it->second->setRect(rect.adjusted(-expand, -expand, expand, expand));
            it->second->setZValue(isInBestResult ? 10 : 9);

            highlightedNodeIds_.insert(nodeId);

            QString status = isInBestResult ? "Best Result Connector" : "Extra Connector";
            QString tooltip = QString("%1\n%2\nFrequency: %3/%4 (%5%)")
                .arg(QString::fromStdString(graph_.getNodeName(nodeId)))
                .arg(status)
                .arg(count)
                .arg(totalPermutations)
                .arg(QString::number(frequency * 100.0, 'f', 1));
            it->second->setToolTip(tooltip);
            
            it->second->setAcceptHoverEvents(true);
            if (isInBestResult) coloredBest++; else coloredExtra++;
        } else {
            notFound++;
        }
    }

    updateEdgeOpacity();

    if (scene_) {
        scene_->update();
    }
}

void GraphVisualization::layoutGraph()
{
    if (!visualizationEnabled_ || !scene_) {
        return;
    }
    
    int nodeCount = graph_.getNodeCount();
    if (nodeCount == 0) {
        return;
    }
    
    try {
        scene_->clear();
        nodeItems_.clear();
        edgeItems_.clear();
        edgePathItem_ = nullptr;
    } catch (...) {
        return;
    }
    
    calculateNodePositions();
    
    if (nodeCount < 300) {
        resolveOverlaps();
    }
    
    drawEdges();
    drawNodes();
    
    if (!currentSeeds_.empty()) {
        highlightSeedsByName(currentSeeds_);
    }
    
    fitToView();
}

void GraphVisualization::drawNodes()
{
    if (!visualizationEnabled_ || !scene_) {
        return;
    }

    auto nodes = graph_.getNodes();
    int nodeCount = nodes.size();
    
    bool isVeryLargeGraph = (nodeCount > 2000);
    bool isLargeGraph = (nodeCount > 500);
    
    std::unordered_map<int, int> nodeDegrees;
    if (!isLargeGraph) {
        for (int nodeId : nodes) {
            nodeDegrees[nodeId] = graph_.getNeighbors(nodeId).size();
        }
    }
    
    QPen sharedPen(nodeAppearance_.borderColor, isVeryLargeGraph ? 0.5 : nodeAppearance_.borderWidth);
    QBrush sharedBrush(nodeAppearance_.color);
    double baseRadius = nodeAppearance_.radius;

    for (int nodeId : nodes) {
        auto it = nodePositions_.find(nodeId);
        if (it == nodePositions_.end()) continue;
        
        QPointF pos = it->second;
        
        if (std::isnan(pos.x()) || std::isnan(pos.y()) || std::isinf(pos.x()) || std::isinf(pos.y())) {
            continue;
        }
        
        double finalRadius = baseRadius;
        
        if (!isLargeGraph) {
            int degree = nodeDegrees[nodeId];
            double degreeScale = 1.0 + 0.2 * std::log1p(degree);
            finalRadius = baseRadius * degreeScale;
            finalRadius = std::max(baseRadius * 0.7, std::min(finalRadius, baseRadius * 1.8));
        }
        
        QGraphicsEllipseItem* ellipse = scene_->addEllipse(
            pos.x() - finalRadius, pos.y() - finalRadius, 
            2 * finalRadius, 2 * finalRadius,
            sharedPen, sharedBrush
        );
        
        if (!ellipse) continue;
        
        ellipse->setZValue(2);
        ellipse->setData(0, nodeId);
        
        if (!isVeryLargeGraph) {
            ellipse->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        }
        
        if (!isLargeGraph) {
            ellipse->setFlag(QGraphicsItem::ItemIsMovable, true);
            ellipse->setFlag(QGraphicsItem::ItemIsSelectable, true);
            ellipse->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
            ellipse->setCursor(Qt::OpenHandCursor);
        }
        
        nodeItems_[nodeId] = ellipse;
        
        if (nodeAppearance_.showLabels && !isLargeGraph) {
            QString labelText = QString::fromStdString(graph_.getNodeName(nodeId));
            QGraphicsTextItem* label = scene_->addText(
                labelText,
                QFont("Arial", nodeAppearance_.fontSize, QFont::Bold)
            );
            if (label) {
                QRectF labelRect = label->boundingRect();
                label->setPos(
                    pos.x() - labelRect.width() / 2,
                    pos.y() - labelRect.height() / 2
                );
                label->setZValue(3);
                label->setDefaultTextColor(nodeAppearance_.textColor);
            }
        }
    }
}

void GraphVisualization::drawNodesBatched(const std::vector<int>& nodes)
{
    
    QPainterPath nodePath;
    double radius = 1.5;
    
    for (int nodeId : nodes) {
        auto it = nodePositions_.find(nodeId);
        if (it == nodePositions_.end()) continue;
        
        const QPointF& pos = it->second;
        if (std::isnan(pos.x()) || std::isnan(pos.y())) continue;
        
        nodePath.addRect(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2);
    }
    
    QGraphicsPathItem* pathItem = scene_->addPath(nodePath);
    if (pathItem) {
        pathItem->setPen(Qt::NoPen);
        pathItem->setBrush(QBrush(nodeAppearance_.color));
        pathItem->setZValue(2);
        pathItem->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
    
}

void GraphVisualization::drawEdges()
{
    if (!visualizationEnabled_ || !scene_) {
        return;
    }

    auto edges = graph_.getEdges();
    int edgeCount = edges.size();
    
    if (edgeCount == 0) return;
    
    
    QPainterPath edgePath;
    
    int maxEdgesToDraw = 100000;
    int step = 1;
    if (edgeCount > maxEdgesToDraw) {
        step = (edgeCount + maxEdgesToDraw - 1) / maxEdgesToDraw;
    }
    
    int drawnCount = 0;
    int index = 0;
    
    for (const auto& edge : edges) {
        if (step > 1 && (index++ % step) != 0) {
            continue;
        }
        
        auto it1 = nodePositions_.find(edge.first);
        auto it2 = nodePositions_.find(edge.second);
        
        if (it1 != nodePositions_.end() && it2 != nodePositions_.end()) {
            edgePath.moveTo(it1->second);
            edgePath.lineTo(it2->second);
            drawnCount++;
        }
        
        if (drawnCount >= maxEdgesToDraw) break;
    }
    
    QGraphicsPathItem* pathItem = scene_->addPath(edgePath);
    if (pathItem) {
        QPen edgePen(edgeAppearance_.color, edgeAppearance_.width);
        edgePen.setCosmetic(edgeAppearance_.cosmetic);
        
        if (step > 1) {
            QColor c = edgeAppearance_.color;
            c.setAlpha(150);
            edgePen.setColor(c);
        }
        
        pathItem->setPen(edgePen);
        pathItem->setBrush(Qt::NoBrush);
        pathItem->setZValue(1);
        
        if (edgeCount < 50000) {
            pathItem->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        }
    }
    
    edgePathItem_ = pathItem;
}

void GraphVisualization::calculateNodePositions()
{
    auto nodes = graph_.getNodes();
    int nodeCount = nodes.size();
    
    if (nodes.empty()) {
        return;
    }
    
    try {
        LayoutType layoutToUse = currentLayoutType_;
        
        if (layoutToUse == LayoutType::Automatic) {
            if (nodeCount <= 20) {
                layoutToUse = LayoutType::Circular;
            } else if (nodeCount <= 100) {
                layoutToUse = LayoutType::ForceAtlas;
            } else {
                layoutToUse = LayoutType::YifanHu;
            }
        }
        
        switch (layoutToUse) {
            case LayoutType::Circular:
                layoutCircular(nodes);
                break;
            case LayoutType::SpringForce:
                layoutSpringForce(nodes);
                break;
            case LayoutType::SpringForceParallel:
                layoutSpringForceParallel(nodes);
                break;
            case LayoutType::Grid:
                layoutGrid(nodes);
                break;
            case LayoutType::Random:
                layoutRandom(nodes);
                break;
            case LayoutType::Hierarchical:
                layoutHierarchical(nodes);
                break;
            case LayoutType::Radial:
                layoutRadial(nodes);
                break;
            case LayoutType::ForceAtlas:
                layoutForceAtlas(nodes);
                break;
            case LayoutType::ForceAtlasParallel:
                layoutForceAtlasParallel(nodes);
                break;
            case LayoutType::YifanHu:
                layoutYifanHu(nodes);
                break;
            case LayoutType::YifanHuParallel:
                layoutYifanHuParallel(nodes);
                break;
            case LayoutType::SeedCentric:
                layoutSeedCentric(nodes);
                break;
            case LayoutType::FWTL:
                layoutFWTL(nodes);
                break;
            default:
                layoutForceAtlas(nodes);
                break;
        }
        
        bool hasInvalidPositions = false;
        for (int nodeId : nodes) {
            QPointF pos = nodePositions_[nodeId];
            if (std::isnan(pos.x()) || std::isnan(pos.y()) || std::isinf(pos.x()) || std::isinf(pos.y())) {
                hasInvalidPositions = true;
                break;
            }
        }
        
        if (hasInvalidPositions) {
            layoutGrid(nodes);
        }
        
    } catch (const std::exception&) {
        layoutGrid(nodes);
    } catch (...) {
        layoutGrid(nodes);
    }
}

void GraphVisualization::layoutCircular(const std::vector<int>& nodes)
{
    if (nodes.empty()) return;
    
    const double radius = layoutParameters_.circularRadius;
    const double startAngle = layoutParameters_.circularStartAngle * M_PI / 180.0;
    const bool clockwise = layoutParameters_.circularClockwise;
    const double direction = clockwise ? 1.0 : -1.0;
    
    const double angleStep = 2 * M_PI / static_cast<double>(nodes.size()) * direction;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        double angle = startAngle + static_cast<double>(i) * angleStep;
        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        
        if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y)) {
            double fallbackAngle = startAngle + static_cast<double>(i) * 6.28318 / static_cast<double>(nodes.size()) * direction;
            x = radius * std::cos(fallbackAngle);
            y = radius * std::sin(fallbackAngle);
            
            if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y)) {
                const double nodeSpacing = 60.0;
                int gridSize = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(nodes.size()))));
                x = (static_cast<int>(i) % gridSize) * nodeSpacing - (gridSize * nodeSpacing / 2.0);
                y = (static_cast<int>(i) / gridSize) * nodeSpacing - (gridSize * nodeSpacing / 2.0);
            }
        }
        
        nodePositions_[nodes[i]] = QPointF(x, y);
    }
}

void GraphVisualization::layoutSpringForce(const std::vector<int>& nodes)
{
    const double k = layoutParameters_.springOptimalDistance;
    const double area = 600.0 * 600.0;
    const double repulsion = std::sqrt(area / static_cast<double>(nodes.size())) * 1.5;
    const int iterations = layoutParameters_.springIterations;
    const double minDistance = 30.0;
    const double maxForce = 50.0;
    double temperature = layoutParameters_.springTemperature;
    const double coolingFactor = layoutParameters_.springCoolingFactor;
    
    std::mt19937 gen(layoutParameters_.layoutSeed);
    std::uniform_real_distribution<> dis(-150.0, 150.0);
    
    for (int nodeId : nodes) {
        nodePositions_[nodeId] = QPointF(dis(gen), dis(gen));
    }
    
    for (int iter = 0; iter < iterations; ++iter) {
        std::map<int, QPointF> forces;
        
        for (int nodeId : nodes) {
            forces[nodeId] = QPointF(0, 0);
        }
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i + 1; j < nodes.size(); ++j) {
                int node1 = nodes[i];
                int node2 = nodes[j];
                
                QPointF pos1 = nodePositions_[node1];
                QPointF pos2 = nodePositions_[node2];
                QPointF delta = pos1 - pos2;
                
                double distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
                
                if (distance < minDistance) {
                    distance = minDistance;
                    double offsetAngle = static_cast<double>(i + j) * 0.1;
                    delta = QPointF(minDistance * std::cos(offsetAngle), minDistance * std::sin(offsetAngle));
                }
                
                double force = std::min(maxForce, repulsion * repulsion / distance);
                    QPointF unitVector = delta / distance;
                    
                if (!std::isnan(force) && !std::isinf(force) && 
                    !std::isnan(unitVector.x()) && !std::isnan(unitVector.y())) {
                    QPointF forceVector = unitVector * force;
                    forces[node1] += forceVector;
                    forces[node2] -= forceVector;
                }
            }
        }
        
        auto edges = graph_.getEdges();
        for (const auto& edge : edges) {
            int node1 = edge.first;
            int node2 = edge.second;
            
            QPointF pos1 = nodePositions_[node1];
            QPointF pos2 = nodePositions_[node2];
            QPointF delta = pos2 - pos1;
            
            double distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
            
            if (distance < minDistance) {
                distance = minDistance;
            }
            
            double idealDistance = k;
            double force = std::min(maxForce, (distance - idealDistance) * 0.3);
            QPointF unitVector = delta / distance;
            
            if (!std::isnan(force) && !std::isinf(force) && 
                !std::isnan(unitVector.x()) && !std::isnan(unitVector.y())) {
                QPointF forceVector = unitVector * force;
                forces[node1] += forceVector;
                forces[node2] -= forceVector;
            }
        }
        
        temperature *= coolingFactor;
        double dampening = std::max(0.1, temperature / layoutParameters_.springTemperature * (1.0 - static_cast<double>(iter) / iterations));
        
        for (int nodeId : nodes) {
            QPointF forceVector = forces[nodeId];
            
            double forceMagnitude = std::sqrt(forceVector.x() * forceVector.x() + forceVector.y() * forceVector.y());
            if (forceMagnitude > maxForce) {
                forceVector = forceVector * (maxForce / forceMagnitude);
            }
            
            QPointF newForce = forceVector * dampening;
            
            if (!std::isnan(newForce.x()) && !std::isnan(newForce.y()) && 
                !std::isinf(newForce.x()) && !std::isinf(newForce.y())) {
                nodePositions_[nodeId] += newForce;
                
                QPointF& pos = nodePositions_[nodeId];
                pos.setX(std::max(-800.0, std::min(800.0, pos.x())));
                pos.setY(std::max(-800.0, std::min(800.0, pos.y())));
            }
        }
    }
}

void GraphVisualization::layoutGrid(const std::vector<int>& nodes)
{
    if (nodes.empty()) return;
    
    const double spacing = layoutParameters_.gridSpacing;
    int gridCols = layoutParameters_.gridColumns;
    
    if (gridCols <= 0) {
        gridCols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(nodes.size()))));
    }
    
    int gridRows = static_cast<int>(std::ceil(static_cast<double>(nodes.size()) / gridCols));
    
    const double startX = -(gridCols - 1) * spacing / 2.0;
    const double startY = -(gridRows - 1) * spacing / 2.0;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        int row = static_cast<int>(i) / gridCols;
        int col = static_cast<int>(i) % gridCols;
        
        double x = startX + col * spacing;
        double y = startY + row * spacing;
        
        nodePositions_[nodes[i]] = QPointF(x, y);
    }
}

void GraphVisualization::layoutRandom(const std::vector<int>& nodes)
{
    if (nodes.empty()) return;
    
    const double width = layoutParameters_.randomWidth;
    const double height = layoutParameters_.randomHeight;
    const int seed = layoutParameters_.randomSeed;
    
    std::mt19937 gen(seed);
    
    std::uniform_real_distribution<> disX(-width / 2.0, width / 2.0);
    std::uniform_real_distribution<> disY(-height / 2.0, height / 2.0);
    
    const double minDistance = std::min(50.0, std::min(width, height) / 10.0);
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        QPointF position;
        bool positionValid = false;
        int attempts = 0;
        
        while (!positionValid && attempts < 100) {
            position = QPointF(disX(gen), disY(gen));
            positionValid = true;
            
            for (size_t j = 0; j < i; ++j) {
                QPointF existingPos = nodePositions_[nodes[j]];
                double distance = std::sqrt(
                    std::pow(position.x() - existingPos.x(), 2) + 
                    std::pow(position.y() - existingPos.y(), 2)
                );
                
                if (distance < minDistance) {
                    positionValid = false;
                    break;
                }
            }
            attempts++;
        }
        
        nodePositions_[nodes[i]] = position;
    }
}

void GraphVisualization::layoutRadial(const std::vector<int>& nodes)
{
    if (nodes.empty()) return;
    
    int rootNode = nodes[0];
    int maxDegree = 0;
    
    for (int nodeId : nodes) {
        int degree = graph_.getNeighbors(nodeId).size();
        if (degree > maxDegree) {
            maxDegree = degree;
            rootNode = nodeId;
        }
    }
    
    nodePositions_[rootNode] = QPointF(0, 0);
    
    std::map<int, int> levels;
    std::set<int> visited;
    std::queue<int> queue;
    
    levels[rootNode] = 0;
    visited.insert(rootNode);
    queue.push(rootNode);
    
    int maxLevel = 0;
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        auto neighbors = graph_.getNeighbors(current);
        for (int neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                levels[neighbor] = levels[current] + 1;
                maxLevel = std::max(maxLevel, levels[neighbor]);
                queue.push(neighbor);
            }
        }
    }
    
    std::map<int, std::vector<int>> nodesByLevel;
    for (int nodeId : nodes) {
        if (levels.find(nodeId) != levels.end()) {
            nodesByLevel[levels[nodeId]].push_back(nodeId);
        } else {
            nodesByLevel[maxLevel + 1].push_back(nodeId);
        }
    }
    
    for (auto& levelPair : nodesByLevel) {
        int level = levelPair.first;
        auto& levelNodes = levelPair.second;
        
        if (level == 0) continue;
        
        double radius = level * 80.0;
        double angleStep = 2 * M_PI / static_cast<double>(levelNodes.size());
        
        for (size_t i = 0; i < levelNodes.size(); ++i) {
            double angle = i * angleStep;
            double x = radius * std::cos(angle);
            double y = radius * std::sin(angle);
            nodePositions_[levelNodes[i]] = QPointF(x, y);
        }
    }
}

void GraphVisualization::layoutHierarchical(const std::vector<int>& nodes)
{
    if (nodes.empty()) return;
    
    std::map<int, int> inDegree;
    for (int nodeId : nodes) {
        inDegree[nodeId] = 0;
    }
    
    auto edges = graph_.getEdges();
    for (const auto& edge : edges) {
        inDegree[edge.second]++;
    }
    
    std::vector<int> roots;
    int minInDegree = INT_MAX;
    for (const auto& pair : inDegree) {
        if (pair.second < minInDegree) {
            minInDegree = pair.second;
        }
    }
    
    for (const auto& pair : inDegree) {
        if (pair.second == minInDegree) {
            roots.push_back(pair.first);
        }
    }
    
    if (roots.empty()) {
        roots.push_back(nodes[0]);
    }
    
    std::map<int, int> levels;
    std::set<int> visited;
    std::queue<int> queue;
    
    for (int root : roots) {
        levels[root] = 0;
        visited.insert(root);
        queue.push(root);
    }
    
    int maxLevel = 0;
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        auto neighbors = graph_.getNeighbors(current);
        for (int neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                levels[neighbor] = levels[current] + 1;
                maxLevel = std::max(maxLevel, levels[neighbor]);
                queue.push(neighbor);
            }
        }
    }
    
    for (int nodeId : nodes) {
        if (levels.find(nodeId) == levels.end()) {
            levels[nodeId] = maxLevel + 1;
        }
    }
    
    std::map<int, std::vector<int>> nodesByLevel;
    for (int nodeId : nodes) {
        nodesByLevel[levels[nodeId]].push_back(nodeId);
    }
    
    const double levelHeight = 120.0;
    const double nodeSpacing = 80.0;
    
    for (auto& levelPair : nodesByLevel) {
        int level = levelPair.first;
        auto& levelNodes = levelPair.second;
        
        double y = level * levelHeight;
        double totalWidth = (levelNodes.size() - 1) * nodeSpacing;
        double startX = -totalWidth / 2.0;
        
        for (size_t i = 0; i < levelNodes.size(); ++i) {
            double x = startX + i * nodeSpacing;
            nodePositions_[levelNodes[i]] = QPointF(x, y);
        }
    }
}

void GraphVisualization::layoutForceAtlas(const std::vector<int>& nodes)
{
    const double gravity = layoutParameters_.forceAtlasGravity;
    const double scalingRatio = layoutParameters_.forceAtlasRepulsion / 100.0;
    const int iterations = layoutParameters_.forceAtlasIterations;
    const double minDistance = layoutParameters_.forceAtlasPreventOverlap ? 40.0 : 20.0;
    const double maxForce = 30.0;
    double temperature = layoutParameters_.forceAtlasTemperature;
    const double cooling = layoutParameters_.forceAtlasCooling;
    
    std::mt19937 gen(layoutParameters_.layoutSeed);
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        double angle = (2 * M_PI * i) / nodes.size();
        double radius = std::sqrt(static_cast<double>(nodes.size())) * 20.0;
        
        std::uniform_real_distribution<> noiseDis(-20.0, 20.0);
        double x = radius * std::cos(angle) + noiseDis(gen);
        double y = radius * std::sin(angle) + noiseDis(gen);
        
        nodePositions_[nodes[i]] = QPointF(x, y);
    }
    
    for (int iter = 0; iter < iterations; ++iter) {
        std::map<int, QPointF> forces;
        
        for (int nodeId : nodes) {
            forces[nodeId] = QPointF(0, 0);
        }
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i + 1; j < nodes.size(); ++j) {
                int node1 = nodes[i];
                int node2 = nodes[j];
                
                QPointF pos1 = nodePositions_[node1];
                QPointF pos2 = nodePositions_[node2];
                QPointF delta = pos1 - pos2;
                
                double distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
                
                if (distance < minDistance) {
                    distance = minDistance;
                    double angle = std::atan2(delta.y(), delta.x());
                    delta = QPointF(minDistance * std::cos(angle), minDistance * std::sin(angle));
                }
                
                double repulsiveForce = scalingRatio / distance;
                repulsiveForce = std::min(repulsiveForce, maxForce);
                
                QPointF unitVector = delta / distance;
                QPointF forceVector = unitVector * repulsiveForce;
                
                forces[node1] += forceVector;
                forces[node2] -= forceVector;
            }
        }
        
        auto edges = graph_.getEdges();
        for (const auto& edge : edges) {
            int node1 = edge.first;
            int node2 = edge.second;
            
            QPointF pos1 = nodePositions_[node1];
            QPointF pos2 = nodePositions_[node2];
            QPointF delta = pos2 - pos1;
            
            double distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
            
            if (distance > minDistance) {
                double attractiveForce = distance * 0.01;
                attractiveForce = std::min(attractiveForce, maxForce);
                
                QPointF unitVector = delta / distance;
                QPointF forceVector = unitVector * attractiveForce;
                
                forces[node1] += forceVector;
                forces[node2] -= forceVector;
            }
        }
        
        for (int nodeId : nodes) {
            QPointF pos = nodePositions_[nodeId];
            double distFromCenter = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
            
            if (distFromCenter > 0) {
                QPointF gravityForce = -pos * (gravity / distFromCenter);
                forces[nodeId] += gravityForce;
            }
        }
        
        temperature *= cooling;
        double coolingFactor = std::max(0.05, temperature * (1.0 - static_cast<double>(iter) / iterations));
        
        for (int nodeId : nodes) {
            QPointF force = forces[nodeId];
            
            double forceMagnitude = std::sqrt(force.x() * force.x() + force.y() * force.y());
            if (forceMagnitude > maxForce) {
                force = force * (maxForce / forceMagnitude);
            }
            
            nodePositions_[nodeId] += force * coolingFactor;
            
            QPointF& pos = nodePositions_[nodeId];
            double maxRadius = std::sqrt(static_cast<double>(nodes.size())) * 50.0;
            double currentRadius = std::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
            
            if (currentRadius > maxRadius) {
                double angle = std::atan2(pos.y(), pos.x());
                pos.setX(maxRadius * std::cos(angle));
                pos.setY(maxRadius * std::sin(angle));
        }
    }
    }
}

namespace {

struct QuadTreeNode {
    double cx, cy;
    double mass;
    double x0, y0, x1, y1;
    int bodyIndex;
    QuadTreeNode* children[4];
    bool isLeaf;
    
    QuadTreeNode(double x0_, double y0_, double x1_, double y1_)
        : cx(0), cy(0), mass(0), x0(x0_), y0(y0_), x1(x1_), y1(y1_),
          bodyIndex(-1), isLeaf(true) {
        children[0] = children[1] = children[2] = children[3] = nullptr;
    }
    
    ~QuadTreeNode() {
        for (int i = 0; i < 4; ++i) delete children[i];
    }
    
    int getQuadrant(double px, double py) const {
        double midX = (x0 + x1) / 2.0;
        double midY = (y0 + y1) / 2.0;
        if (px <= midX) {
            return (py <= midY) ? 0 : 2;
        } else {
            return (py <= midY) ? 1 : 3;
        }
    }
    
    void subdivide() {
        double midX = (x0 + x1) / 2.0;
        double midY = (y0 + y1) / 2.0;
        children[0] = new QuadTreeNode(x0, y0, midX, midY);
        children[1] = new QuadTreeNode(midX, y0, x1, midY);
        children[2] = new QuadTreeNode(x0, midY, midX, y1);
        children[3] = new QuadTreeNode(midX, midY, x1, y1);
        isLeaf = false;
    }
    
    void insert(int idx, double px, double py, const std::vector<double>& posX,
                const std::vector<double>& posY, int maxLevel, int level = 0) {
        if (level >= maxLevel) {
            double totalMass = mass + 1.0;
            cx = (cx * mass + px) / totalMass;
            cy = (cy * mass + py) / totalMass;
            mass = totalMass;
            return;
        }
        
        if (mass == 0) {
            bodyIndex = idx;
            cx = px;
            cy = py;
            mass = 1.0;
            return;
        }
        
        if (isLeaf && bodyIndex >= 0) {
            subdivide();
            int oldIdx = bodyIndex;
            double oldX = posX[oldIdx], oldY = posY[oldIdx];
            bodyIndex = -1;
            int q = getQuadrant(oldX, oldY);
            children[q]->insert(oldIdx, oldX, oldY, posX, posY, maxLevel, level + 1);
        } else if (isLeaf) {
            subdivide();
        }
        
        int q = getQuadrant(px, py);
        children[q]->insert(idx, px, py, posX, posY, maxLevel, level + 1);
        
        double totalMass = mass + 1.0;
        cx = (cx * mass + px) / totalMass;
        cy = (cy * mass + py) / totalMass;
        mass = totalMass;
    }
    
    void calculateForce(int idx, double px, double py, double theta,
                        double C, double K2, double& fx, double& fy) const {
        if (mass == 0) return;
        
        double dx = px - cx;
        double dy = py - cy;
        double dist2 = dx * dx + dy * dy;
        double dist = std::sqrt(dist2);
        
        if (dist < 0.01) return;
        
        double size = x1 - x0;
        
        if (isLeaf || (size / dist < theta)) {
            double force = C * K2 * mass / dist;
            fx += (dx / dist) * force;
            fy += (dy / dist) * force;
        } else {
            for (int i = 0; i < 4; ++i) {
                if (children[i]) {
                    children[i]->calculateForce(idx, px, py, theta, C, K2, fx, fy);
                }
            }
        }
    }
};

}

void GraphVisualization::layoutYifanHu(const std::vector<int>& nodes)
{
    if (nodes.empty()) return;
    
    int nodeCount = static_cast<int>(nodes.size());
    
    double K = layoutParameters_.yifanOptimalDistance;
    double C = layoutParameters_.yifanRelativeStrength;
    double initialStep = layoutParameters_.yifanInitialStepSize;
    double stepRatio = layoutParameters_.yifanStepRatio;
    bool adaptiveCooling = layoutParameters_.yifanAdaptiveCooling;
    double convergenceThreshold = layoutParameters_.yifanConvergenceThreshold;
    int quadtreeMaxLevel = layoutParameters_.yifanQuadtreeMaxLevel;
    double theta = layoutParameters_.yifanTheta;
    
    double K2 = K * K;
    bool useBarnesHut = (nodeCount > 100);
    int maxIterations = 2000;
    
    std::unordered_map<int, size_t> nodeIndex;
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodeIndex[nodes[i]] = i;
    }
    
    auto allEdges = graph_.getEdges();
    std::vector<std::pair<size_t, size_t>> indexedEdges;
    indexedEdges.reserve(allEdges.size());
    for (const auto& edge : allEdges) {
        auto it1 = nodeIndex.find(edge.first);
        auto it2 = nodeIndex.find(edge.second);
        if (it1 != nodeIndex.end() && it2 != nodeIndex.end()) {
            indexedEdges.emplace_back(it1->second, it2->second);
        }
    }
    
    std::vector<double> posX(nodeCount), posY(nodeCount);
    double spread = K * std::sqrt(static_cast<double>(nodeCount));
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(-spread, spread);
    for (int i = 0; i < nodeCount; ++i) {
        posX[i] = dist(gen);
        posY[i] = dist(gen);
    }
    
    std::vector<double> forceX(nodeCount), forceY(nodeCount);
    
    double step = initialStep;
    double energy = std::numeric_limits<double>::max();
    int progress = 0;
    
    for (int iter = 0; iter < maxIterations; ++iter) {
        std::fill(forceX.begin(), forceX.end(), 0.0);
        std::fill(forceY.begin(), forceY.end(), 0.0);
        
        if (useBarnesHut) {
            double minX = posX[0], maxX = posX[0], minY = posY[0], maxY = posY[0];
            for (int i = 1; i < nodeCount; ++i) {
                minX = std::min(minX, posX[i]);
                maxX = std::max(maxX, posX[i]);
                minY = std::min(minY, posY[i]);
                maxY = std::max(maxY, posY[i]);
            }
            double pad = std::max(maxX - minX, maxY - minY) * 0.01 + 1.0;
            double side = std::max(maxX - minX, maxY - minY) + 2 * pad;
            double cx = (minX + maxX) / 2.0, cy = (minY + maxY) / 2.0;
            
            QuadTreeNode root(cx - side / 2, cy - side / 2, cx + side / 2, cy + side / 2);
            for (int i = 0; i < nodeCount; ++i) {
                root.insert(i, posX[i], posY[i], posX, posY, quadtreeMaxLevel);
            }
            
            for (int i = 0; i < nodeCount; ++i) {
                root.calculateForce(i, posX[i], posY[i], theta, C, K2,
                                    forceX[i], forceY[i]);
            }
        } else {
            for (int i = 0; i < nodeCount; ++i) {
                for (int j = i + 1; j < nodeCount; ++j) {
                    double dx = posX[i] - posX[j];
                    double dy = posY[i] - posY[j];
                    double d = std::sqrt(dx * dx + dy * dy);
                    if (d < 0.01) d = 0.01;
                    
                    double force = C * K2 / d;
                    double fx = (dx / d) * force;
                    double fy = (dy / d) * force;
                    
                    forceX[i] += fx;
                    forceY[i] += fy;
                    forceX[j] -= fx;
                    forceY[j] -= fy;
                }
            }
        }
        
        for (const auto& edge : indexedEdges) {
            size_t i = edge.first;
            size_t j = edge.second;
            
            double dx = posX[j] - posX[i];
            double dy = posY[j] - posY[i];
            double d = std::sqrt(dx * dx + dy * dy);
            if (d < 0.01) d = 0.01;
            
            double force = (d * d) / K;
            double fx = (dx / d) * force;
            double fy = (dy / d) * force;
            
            forceX[i] += fx;
            forceY[i] += fy;
            forceX[j] -= fx;
            forceY[j] -= fy;
        }
        
        double newEnergy = 0.0;
        for (int i = 0; i < nodeCount; ++i) {
            double fLen = std::sqrt(forceX[i] * forceX[i] + forceY[i] * forceY[i]);
            newEnergy += fLen * fLen;
            
            if (fLen > 0.01) {
                double movement = std::min(fLen, step);
                posX[i] += (forceX[i] / fLen) * movement;
                posY[i] += (forceY[i] / fLen) * movement;
            }
        }
        
        if (adaptiveCooling) {
            if (newEnergy < energy) {
                progress++;
                if (progress >= 5) {
                    progress = 0;
                    step /= stepRatio;
                }
            } else {
                progress = 0;
                step *= stepRatio;
            }
            energy = newEnergy;
        } else {
            step *= stepRatio;
        }
        
        if (step < convergenceThreshold * K) {
            break;
        }
    }
    
    for (int i = 0; i < nodeCount; ++i) {
        nodePositions_[nodes[i]] = QPointF(posX[i], posY[i]);
    }
}

bool GraphVisualization::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            const double scaleFactor = 1.15;
            
            if (wheelEvent->angleDelta().y() > 0) {
                scale(scaleFactor, scaleFactor);
            } else {
                scale(1.0 / scaleFactor, 1.0 / scaleFactor);
            }
            return true;
        }
    }
    return QGraphicsView::eventFilter(obj, event);
}

void GraphVisualization::wheelEvent(QWheelEvent *event)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    
    const double scaleFactor = 1.15;
    
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void GraphVisualization::fitToView()
{
    if (!scene_ || !visualizationEnabled_) {
        return;
    }
    
    try {
        if (scene_->items().isEmpty()) {
            return;
        }
        
        QRectF itemsRect = scene_->itemsBoundingRect();
        
        double marginX = itemsRect.width() * 0.1;
        double marginY = itemsRect.height() * 0.1;
        itemsRect.adjust(-marginX, -marginY, marginX, marginY);
        
        setSceneRect(itemsRect);
        
        fitInView(itemsRect, Qt::KeepAspectRatio);
        
        centerOn(itemsRect.center());
        
        QTransform currentTransform = transform();
        double currentScale = currentTransform.m11();
        
        if (currentScale > 2.0) {
            resetZoom();
            scale(2.0, 2.0);
            centerOn(itemsRect.center());
        }
    } catch (...) {
        return;
    }
}

void GraphVisualization::resetZoom()
{
    resetTransform();
}

void GraphVisualization::setLayoutType(LayoutType type)
{
    currentLayoutType_ = type;
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::visualizeGraph()
{
    layoutGraph();
}

void GraphVisualization::highlightNodeTemporary(const std::string& nodeName)
{
    clearTemporaryHighlight();
    
    int nodeId = graph_.getNodeId(nodeName);
    if (nodeId < 0) return;
    
    auto it = nodeItems_.find(nodeId);
    if (it == nodeItems_.end() || !it->second) return;
    
    QGraphicsEllipseItem* item = it->second;
    
    temporaryHighlightNodeId_ = nodeId;
    temporaryHighlightOriginalBrush_ = item->brush();
    temporaryHighlightOriginalPen_ = item->pen();
    temporaryHighlightOriginalRect_ = item->rect();
    temporaryHighlightOriginalZValue_ = item->zValue();
    
    QColor highlightColor(0, 255, 200);
    item->setPen(QPen(highlightColor, 4));
    item->setBrush(QBrush(highlightColor.lighter(120)));
    
    QRectF rect = item->rect();
    double expand = 6.0;
    item->setRect(rect.adjusted(-expand, -expand, expand, expand));
    
    item->setZValue(10000);
    
    centerOn(item);
}

void GraphVisualization::clearTemporaryHighlight()
{
    if (temporaryHighlightNodeId_ < 0) return;
    
    auto it = nodeItems_.find(temporaryHighlightNodeId_);
    if (it != nodeItems_.end() && it->second) {
        QGraphicsEllipseItem* item = it->second;
        item->setBrush(temporaryHighlightOriginalBrush_);
        item->setPen(temporaryHighlightOriginalPen_);
        item->setRect(temporaryHighlightOriginalRect_);
        item->setZValue(temporaryHighlightOriginalZValue_);
    }
    
    temporaryHighlightNodeId_ = -1;
}

void GraphVisualization::showWelcomeMessage()
{
    if (!scene_ || !visualizationEnabled_) {
        return;
    }
    
    try {
        scene_->clear();
        nodeItems_.clear();
        edgeItems_.clear();
        
        QGraphicsTextItem* welcomeText = nullptr;
        try {
            welcomeText = scene_->addText(
                "Welcome to BasicNetworkFinder!\n\n"
                "Load a network file to get started with your analysis.",
                QFont("Arial", 14, QFont::Bold)
            );
        } catch (...) {
            return;
        }
        
        if (!welcomeText) {
            return;
        }
        
        welcomeText->setDefaultTextColor(QColor(108, 117, 125));
        welcomeText->setPos(-welcomeText->boundingRect().width() / 2, -welcomeText->boundingRect().height() / 2);
        welcomeText->setTextWidth(400);
        
        QGraphicsEllipseItem* decoration = nullptr;
        try {
            decoration = scene_->addEllipse(-50, -120, 100, 100);
        } catch (...) {
            decoration = nullptr;
        }
        
        if (decoration) {
            decoration->setBrush(QBrush(QColor(173, 216, 230, 100)));
            decoration->setPen(QPen(QColor(173, 216, 230), 2));
        }
        
        setSceneRect(-300, -200, 600, 400);
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    } catch (...) {
        return;
    }
}

void GraphVisualization::setNodeAppearance(const NodeAppearance& appearance)
{
    nodeAppearance_ = appearance;
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setEdgeAppearance(const EdgeAppearance& appearance)
{
    edgeAppearance_ = appearance;
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setNodeColor(const QColor& color)
{
    nodeAppearance_.color = color;
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setSeedColor(const QColor& color)
{
    nodeAppearance_.seedColor = color;
}

void GraphVisualization::setConnectorColor(const QColor& color)
{
    nodeAppearance_.connectorColor = color;
}

void GraphVisualization::setExtraConnectorColor(const QColor& color)
{
    nodeAppearance_.extraConnectorColor = color;
}

void GraphVisualization::setNodeSize(double radius)
{
    nodeAppearance_.radius = std::max(5.0, std::min(50.0, radius));
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setNodeBorderColor(const QColor& color)
{
    nodeAppearance_.borderColor = color;
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setNodeBorderWidth(double width)
{
    nodeAppearance_.borderWidth = std::max(0.5, std::min(10.0, width));
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setShowNodeLabels(bool show)
{
    nodeAppearance_.showLabels = show;
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setNodeFontSize(int size)
{
    nodeAppearance_.fontSize = std::max(6, std::min(24, size));
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setEdgeColor(const QColor& color)
{
    edgeAppearance_.color = color;
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setEdgeWidth(double width)
{
    edgeAppearance_.width = std::max(0.1, std::min(10.0, width));
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::layoutSpringForceParallel(const std::vector<int>& nodes) {
    layoutSpringForce(nodes);
}

void GraphVisualization::layoutForceAtlasParallel(const std::vector<int>& nodes) {
    layoutForceAtlas(nodes);
}

void GraphVisualization::layoutYifanHuParallel(const std::vector<int>& nodes) {
    layoutYifanHu(nodes);
}

/**
 * @brief Seed-Centric Layout Algorithm
 * 
 * CONCEPT: Seeds form the CENTRAL CORE of the visualization.
 * 
 * 1. Seeds are attracted to the center and cluster together
 * 2. Non-seeds are pushed outward, further if they're far from seeds
 * 3. Connectors (nodes linking seeds) stay close to the center
 * 4. Peripheral nodes spread outward
 * 
 * This creates a layout where:
 * - Seeds form a dense central cluster (the focus of analysis)
 * - Direct neighbors of seeds surround them
 * - Distant nodes are pushed to the periphery
 * - The "importance" (proximity to seeds) is visible by position
 */
void GraphVisualization::layoutSeedCentric(const std::vector<int>& nodes)
{
    if (nodes.empty()) return;
    
    int nodeCount = nodes.size();
    
    double K = layoutParameters_.seedCentricSeedRadius;
    double seedAttraction = layoutParameters_.seedCentricSeedRepulsion;
    double edgeAttraction = layoutParameters_.seedCentricConnectorAttraction;
    double outerPush = layoutParameters_.seedCentricConnectorGravity;
    double seedWeight = layoutParameters_.seedCentricSeedCohesion;
    int maxIterations = layoutParameters_.seedCentricIterations;
    
    K = std::max(15.0, K * std::pow(nodeCount / 100.0, 0.35));
    
    std::unordered_map<int, size_t> nodeIndex;
    nodeIndex.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodeIndex[nodes[i]] = i;
    }
    
    std::vector<size_t> seedIndices;
    std::unordered_set<size_t> seedIndexSet;
    
    for (const auto& seedName : currentSeeds_) {
        int nodeId = graph_.getNodeId(seedName);
        if (nodeId >= 0) {
            auto it = nodeIndex.find(nodeId);
            if (it != nodeIndex.end()) {
                seedIndices.push_back(it->second);
                seedIndexSet.insert(it->second);
            }
        }
    }
    
    for (int nodeId : highlightedNodeIds_) {
        auto it = nodeIndex.find(nodeId);
        if (it != nodeIndex.end() && !seedIndexSet.count(it->second)) {
            seedIndices.push_back(it->second);
            seedIndexSet.insert(it->second);
        }
    }
    
    bool hasSeeds = !seedIndices.empty();
    
    auto allEdges = graph_.getEdges();
    std::vector<std::pair<size_t, size_t>> indexedEdges;
    indexedEdges.reserve(allEdges.size());
    for (const auto& edge : allEdges) {
        auto it1 = nodeIndex.find(edge.first);
        auto it2 = nodeIndex.find(edge.second);
        if (it1 != nodeIndex.end() && it2 != nodeIndex.end()) {
            indexedEdges.emplace_back(it1->second, it2->second);
        }
    }
    
    std::vector<std::vector<size_t>> adjacency(nodeCount);
    for (const auto& edge : indexedEdges) {
        adjacency[edge.first].push_back(edge.second);
        adjacency[edge.second].push_back(edge.first);
    }
    
    std::vector<int> distanceToSeed(nodeCount, INT_MAX);
    if (hasSeeds) {
        std::queue<size_t> bfsQueue;
        for (size_t seedIdx : seedIndices) {
            distanceToSeed[seedIdx] = 0;
            bfsQueue.push(seedIdx);
        }
        
        while (!bfsQueue.empty()) {
            size_t current = bfsQueue.front();
            bfsQueue.pop();
            
            for (size_t neighbor : adjacency[current]) {
                if (distanceToSeed[neighbor] > distanceToSeed[current] + 1) {
                    distanceToSeed[neighbor] = distanceToSeed[current] + 1;
                    bfsQueue.push(neighbor);
                }
            }
        }
    }
    
    int maxDist = 1;
    for (int d : distanceToSeed) {
        if (d != INT_MAX && d > maxDist) maxDist = d;
    }
    
    std::vector<double> posX(nodeCount), posY(nodeCount);
    std::vector<double> dispX(nodeCount), dispY(nodeCount);
    
    std::mt19937 rng(layoutParameters_.layoutSeed);
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_real_distribution<double> distAngle(0.0, 2.0 * M_PI);
    
    if (hasSeeds) {
        double seedSpread = K * 0.5;
        for (size_t i = 0; i < seedIndices.size(); ++i) {
            double angle = 2.0 * M_PI * i / seedIndices.size();
            double r = seedSpread * (0.5 + 0.5 * dist01(rng));
            posX[seedIndices[i]] = r * std::cos(angle);
            posY[seedIndices[i]] = r * std::sin(angle);
        }
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (seedIndexSet.count(i)) continue;
            
            int dist = distanceToSeed[i];
            if (dist == INT_MAX) dist = maxDist + 2;
            
            double baseRadius = K * (1.0 + dist * 1.5);
            double angle = distAngle(rng);
            double r = baseRadius * (0.8 + 0.4 * dist01(rng));
            
            posX[i] = r * std::cos(angle);
            posY[i] = r * std::sin(angle);
        }
    } else {
        for (size_t i = 0; i < nodes.size(); ++i) {
            double angle = distAngle(rng);
            double r = K * std::sqrt(nodeCount) * dist01(rng);
            posX[i] = r * std::cos(angle);
            posY[i] = r * std::sin(angle);
        }
    }
    
    double temperature = K * 5;
    
    for (int iter = 0; iter < maxIterations; ++iter) {
        std::fill(dispX.begin(), dispX.end(), 0.0);
        std::fill(dispY.begin(), dispY.end(), 0.0);
        
        if (hasSeeds) {
            for (size_t seedIdx : seedIndices) {
                double dist = std::sqrt(posX[seedIdx] * posX[seedIdx] + posY[seedIdx] * posY[seedIdx]);
                if (dist > 1.0) {
                    double force = seedAttraction * dist / K;
                    dispX[seedIdx] -= (posX[seedIdx] / dist) * force;
                    dispY[seedIdx] -= (posY[seedIdx] / dist) * force;
                }
            }
        }
        
        if (hasSeeds) {
            for (size_t i = 0; i < seedIndices.size(); ++i) {
                for (size_t j = i + 1; j < seedIndices.size(); ++j) {
                    size_t si = seedIndices[i];
                    size_t sj = seedIndices[j];
                    
                    double dx = posX[si] - posX[sj];
                    double dy = posY[si] - posY[sj];
                    double dist2 = dx * dx + dy * dy;
                    if (dist2 < 1.0) dist2 = 1.0;
                    double dist = std::sqrt(dist2);
                    
                    double force = K * K / dist2 * 2.0;
                    
                    dispX[si] += (dx / dist) * force;
                    dispY[si] += (dy / dist) * force;
                    dispX[sj] -= (dx / dist) * force;
                    dispY[sj] -= (dy / dist) * force;
                }
            }
        }
        
        if (hasSeeds && outerPush > 0) {
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (seedIndexSet.count(i)) continue;
                
                int dist = distanceToSeed[i];
                if (dist == INT_MAX) dist = maxDist + 2;
                
                double pushStrength = outerPush * dist / (double)maxDist;
                
                double r = std::sqrt(posX[i] * posX[i] + posY[i] * posY[i]);
                if (r > 1.0) {
                    dispX[i] += (posX[i] / r) * pushStrength * K;
                    dispY[i] += (posY[i] / r) * pushStrength * K;
                }
            }
        }
        
        for (const auto& edge : indexedEdges) {
            size_t i = edge.first;
            size_t j = edge.second;
            
            double dx = posX[j] - posX[i];
            double dy = posY[j] - posY[i];
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 0.1) dist = 0.1;
            
            double force = edgeAttraction * dist / K;
            
            if (hasSeeds) {
                bool iIsSeed = seedIndexSet.count(i);
                bool jIsSeed = seedIndexSet.count(j);
                if (iIsSeed && jIsSeed) {
                    force *= 3.0;
                } else if (iIsSeed || jIsSeed) {
                    force *= (1.0 + seedWeight);
                }
            }
            
            dispX[i] += (dx / dist) * force;
            dispY[i] += (dy / dist) * force;
            dispX[j] -= (dx / dist) * force;
            dispY[j] -= (dy / dist) * force;
        }
        
        for (const auto& edge : indexedEdges) {
            size_t i = edge.first;
            size_t j = edge.second;
            
            double dx = posX[i] - posX[j];
            double dy = posY[i] - posY[j];
            double dist2 = dx * dx + dy * dy;
            if (dist2 < 1.0) dist2 = 1.0;
            double dist = std::sqrt(dist2);
            
            double force = K * K / dist2;
            
            dispX[i] += (dx / dist) * force;
            dispY[i] += (dy / dist) * force;
            dispX[j] -= (dx / dist) * force;
            dispY[j] -= (dy / dist) * force;
        }
        
        double generalGravity = 0.01;
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (seedIndexSet.count(i)) continue;
            double dist = std::sqrt(posX[i] * posX[i] + posY[i] * posY[i]);
            if (dist > K * 5) {
                dispX[i] -= posX[i] * generalGravity;
                dispY[i] -= posY[i] * generalGravity;
            }
        }
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            double dispMag = std::sqrt(dispX[i] * dispX[i] + dispY[i] * dispY[i]);
            if (dispMag > 0.01) {
                double capped = std::min(dispMag, temperature);
                posX[i] += (dispX[i] / dispMag) * capped;
                posY[i] += (dispY[i] / dispMag) * capped;
            }
        }
        
        temperature *= 0.95;
        if (temperature < 0.1) break;
    }
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodePositions_[nodes[i]] = QPointF(posX[i], posY[i]);
    }
}

void GraphVisualization::setConnectorFrequencyData(const std::map<int, int>& connectorFrequency, int totalPermutations)
{
    connectorFrequencyData_ = connectorFrequency;
    totalPermutationsData_ = totalPermutations;
}

/**
 * FWTL - Frequency-Weighted Topological Layout
 * 
 * A NOVEL layout algorithm that integrates permutation-based network analysis
 * results into node positioning. Unlike traditional radial layouts that use only
 * graph-theoretic distance, FWTL incorporates the STATISTICAL CONFIDENCE of each
 * node's role as a connector, derived from multiple permutation runs.
 * 
 * Key innovation: The radial position of a node is determined by BOTH:
 *   1. Its topological distance to seeds (like Seed-Centric)
 *   2. Its frequency of appearance as a connector across permutations (NOVEL)
 * 
 * This creates a layout where:
 *   - Seeds cluster at the center
 *   - HIGH-CONFIDENCE connectors (100% frequency) form a tight inner ring
 *   - LOW-CONFIDENCE connectors (appeared rarely) spread to outer rings
 *   - The visual "spread" of connectors represents ANALYTICAL UNCERTAINTY
 *   - Non-connector neighbors occupy the periphery
 * 
 * This is the ONLY layout algorithm (to our knowledge) that incorporates
 * permutation analysis results into geometric positioning, enabling visual
 * assessment of both network topology AND statistical confidence simultaneously.
 */
void GraphVisualization::layoutFWTL(const std::vector<int>& nodes)
{
    if (nodes.empty()) return;
    
    int nodeCount = static_cast<int>(nodes.size());
    
    double K = layoutParameters_.fwtlBaseDistance;
    double freqWeight = layoutParameters_.fwtlFrequencyWeight;
    double seedAttraction = layoutParameters_.fwtlSeedAttraction;
    double edgeAttraction = layoutParameters_.fwtlEdgeAttraction;
    double confRadius = layoutParameters_.fwtlConfidenceRadius;
    double uncertSpread = layoutParameters_.fwtlUncertaintySpread;
    int maxIterations = layoutParameters_.fwtlIterations;
    double coolingFactor = layoutParameters_.fwtlCooling;
    
    K = std::max(20.0, K * std::pow(nodeCount / 100.0, 0.3));
    
    std::unordered_map<int, size_t> nodeIndex;
    nodeIndex.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodeIndex[nodes[i]] = i;
    }
    
    std::vector<size_t> seedIndices;
    std::unordered_set<size_t> seedIndexSet;
    
    for (const auto& seedName : currentSeeds_) {
        int nodeId = graph_.getNodeId(seedName);
        if (nodeId >= 0) {
            auto it = nodeIndex.find(nodeId);
            if (it != nodeIndex.end()) {
                seedIndices.push_back(it->second);
                seedIndexSet.insert(it->second);
            }
        }
    }
    
    for (int nodeId : highlightedNodeIds_) {
        auto it = nodeIndex.find(nodeId);
        if (it != nodeIndex.end() && !seedIndexSet.count(it->second)) {
            seedIndices.push_back(it->second);
            seedIndexSet.insert(it->second);
        }
    }
    
    bool hasSeeds = !seedIndices.empty();
    bool hasFrequencyData = !connectorFrequencyData_.empty() && totalPermutationsData_ > 0;
    
    std::vector<double> frequencyScore(nodeCount, 0.0);
    std::unordered_set<size_t> connectorIndexSet;
    
    if (hasFrequencyData) {
        for (const auto& [nodeId, count] : connectorFrequencyData_) {
            auto it = nodeIndex.find(nodeId);
            if (it != nodeIndex.end()) {
                frequencyScore[it->second] = static_cast<double>(count) / totalPermutationsData_;
                connectorIndexSet.insert(it->second);
            }
        }
    }
    
    auto allEdges = graph_.getEdges();
    std::vector<std::pair<size_t, size_t>> indexedEdges;
    indexedEdges.reserve(allEdges.size());
    for (const auto& edge : allEdges) {
        auto it1 = nodeIndex.find(edge.first);
        auto it2 = nodeIndex.find(edge.second);
        if (it1 != nodeIndex.end() && it2 != nodeIndex.end()) {
            indexedEdges.emplace_back(it1->second, it2->second);
        }
    }
    
    std::vector<std::vector<size_t>> adjacency(nodeCount);
    for (const auto& edge : indexedEdges) {
        adjacency[edge.first].push_back(edge.second);
        adjacency[edge.second].push_back(edge.first);
    }
    
    std::vector<int> distanceToSeed(nodeCount, INT_MAX);
    if (hasSeeds) {
        std::queue<size_t> bfsQueue;
        for (size_t seedIdx : seedIndices) {
            distanceToSeed[seedIdx] = 0;
            bfsQueue.push(seedIdx);
        }
        
        while (!bfsQueue.empty()) {
            size_t current = bfsQueue.front();
            bfsQueue.pop();
            
            for (size_t neighbor : adjacency[current]) {
                if (distanceToSeed[neighbor] > distanceToSeed[current] + 1) {
                    distanceToSeed[neighbor] = distanceToSeed[current] + 1;
                    bfsQueue.push(neighbor);
                }
            }
        }
    }
    
    int maxDist = 1;
    for (int d : distanceToSeed) {
        if (d != INT_MAX && d > maxDist) maxDist = d;
    }
    
    std::vector<double> targetRadius(nodeCount, 0.0);
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (seedIndexSet.count(i)) {
            targetRadius[i] = K * 0.3;
        } else if (connectorIndexSet.count(i)) {
            double freq = frequencyScore[i];
            
            double innerRadius = K * (1.0 + confRadius);
            double outerRadius = K * (1.0 + uncertSpread);
            
            targetRadius[i] = innerRadius + (outerRadius - innerRadius) * (1.0 - freq) * freqWeight;
        } else {
            int dist = distanceToSeed[i];
            if (dist == INT_MAX) dist = maxDist + 2;
            
            double baseOuterRadius = K * (2.0 + uncertSpread);
            targetRadius[i] = baseOuterRadius + K * dist * 0.5;
        }
    }
    
    std::vector<double> posX(nodeCount), posY(nodeCount);
    std::vector<double> dispX(nodeCount), dispY(nodeCount);
    
    std::mt19937 rng(layoutParameters_.layoutSeed);
    std::uniform_real_distribution<double> distAngle(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    
    if (hasSeeds) {
        double seedSpread = K * 0.4;
        for (size_t i = 0; i < seedIndices.size(); ++i) {
            double angle = 2.0 * M_PI * i / seedIndices.size() + dist01(rng) * 0.3;
            double r = seedSpread * (0.5 + 0.5 * dist01(rng));
            posX[seedIndices[i]] = r * std::cos(angle);
            posY[seedIndices[i]] = r * std::sin(angle);
        }
    }
    
    std::vector<size_t> connectorsByFreq(connectorIndexSet.begin(), connectorIndexSet.end());
    std::sort(connectorsByFreq.begin(), connectorsByFreq.end(),
              [&](size_t a, size_t b) { return frequencyScore[a] > frequencyScore[b]; });
    
    for (size_t i = 0; i < connectorsByFreq.size(); ++i) {
        size_t idx = connectorsByFreq[i];
        double angle = distAngle(rng);
        double r = targetRadius[idx] * (0.9 + 0.2 * dist01(rng));
        posX[idx] = r * std::cos(angle);
        posY[idx] = r * std::sin(angle);
    }
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (seedIndexSet.count(i) || connectorIndexSet.count(i)) continue;
        
        double angle = distAngle(rng);
        double r = targetRadius[i] * (0.8 + 0.4 * dist01(rng));
        posX[i] = r * std::cos(angle);
        posY[i] = r * std::sin(angle);
    }
    
    double temperature = K * 4;
    
    for (int iter = 0; iter < maxIterations; ++iter) {
        std::fill(dispX.begin(), dispX.end(), 0.0);
        std::fill(dispY.begin(), dispY.end(), 0.0);
        
        if (hasSeeds) {
            for (size_t seedIdx : seedIndices) {
                double dist = std::sqrt(posX[seedIdx] * posX[seedIdx] + posY[seedIdx] * posY[seedIdx]);
                if (dist > 1.0) {
                    double force = seedAttraction * dist / K;
                    dispX[seedIdx] -= (posX[seedIdx] / dist) * force;
                    dispY[seedIdx] -= (posY[seedIdx] / dist) * force;
                }
            }
        }
        
        if (hasSeeds) {
            for (size_t i = 0; i < seedIndices.size(); ++i) {
                for (size_t j = i + 1; j < seedIndices.size(); ++j) {
                    size_t si = seedIndices[i];
                    size_t sj = seedIndices[j];
                    
                    double dx = posX[si] - posX[sj];
                    double dy = posY[si] - posY[sj];
                    double dist2 = dx * dx + dy * dy;
                    if (dist2 < 1.0) dist2 = 1.0;
                    double dist = std::sqrt(dist2);
                    
                    double force = K * K / dist2 * 1.5;
                    
                    dispX[si] += (dx / dist) * force;
                    dispY[si] += (dy / dist) * force;
                    dispX[sj] -= (dx / dist) * force;
                    dispY[sj] -= (dy / dist) * force;
                }
            }
        }
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (seedIndexSet.count(i)) continue;
            
            double currentR = std::sqrt(posX[i] * posX[i] + posY[i] * posY[i]);
            if (currentR < 1.0) currentR = 1.0;
            
            double targetR = targetRadius[i];
            double radiusDiff = targetR - currentR;
            
            double radialForce = radiusDiff * 0.3;
            
            if (connectorIndexSet.count(i)) {
                double freq = frequencyScore[i];
                radialForce *= (1.0 + freq);
            }
            
            dispX[i] += (posX[i] / currentR) * radialForce;
            dispY[i] += (posY[i] / currentR) * radialForce;
        }
        
        for (const auto& edge : indexedEdges) {
            size_t i = edge.first;
            size_t j = edge.second;
            
            double dx = posX[j] - posX[i];
            double dy = posY[j] - posY[i];
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 0.1) dist = 0.1;
            
            double force = edgeAttraction * dist / K;
            
            bool iIsSeed = seedIndexSet.count(i);
            bool jIsSeed = seedIndexSet.count(j);
            bool iIsConn = connectorIndexSet.count(i);
            bool jIsConn = connectorIndexSet.count(j);
            
            if ((iIsSeed && jIsConn) || (jIsSeed && iIsConn)) {
                force *= 2.5;
            } else if (iIsSeed || jIsSeed) {
                force *= 1.5;
            }
            
            dispX[i] += (dx / dist) * force;
            dispY[i] += (dy / dist) * force;
            dispX[j] -= (dx / dist) * force;
            dispY[j] -= (dy / dist) * force;
        }
        
        for (const auto& edge : indexedEdges) {
            size_t i = edge.first;
            size_t j = edge.second;
            
            double dx = posX[i] - posX[j];
            double dy = posY[i] - posY[j];
            double dist2 = dx * dx + dy * dy;
            if (dist2 < 1.0) dist2 = 1.0;
            double dist = std::sqrt(dist2);
            
            double force = K * K / dist2;
            
            dispX[i] += (dx / dist) * force;
            dispY[i] += (dy / dist) * force;
            dispX[j] -= (dx / dist) * force;
            dispY[j] -= (dy / dist) * force;
        }
        
        for (const auto& edge : indexedEdges) {
            size_t i = edge.first;
            size_t j = edge.second;
            
            double ri = std::sqrt(posX[i] * posX[i] + posY[i] * posY[i]);
            double rj = std::sqrt(posX[j] * posX[j] + posY[j] * posY[j]);
            
            if (std::abs(ri - rj) < K * 0.5 && ri > 1.0 && rj > 1.0) {
                double cross = posX[i] * posY[j] - posY[i] * posX[j];
                double angularForce = (cross > 0 ? 1.0 : -1.0) * K * 0.1 / std::max(ri, rj);
                
                dispX[i] += -posY[i] / ri * angularForce;
                dispY[i] += posX[i] / ri * angularForce;
                dispX[j] += posY[j] / rj * angularForce;
                dispY[j] += -posX[j] / rj * angularForce;
            }
        }
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            double dispMag = std::sqrt(dispX[i] * dispX[i] + dispY[i] * dispY[i]);
            if (dispMag > 0.01) {
                double capped = std::min(dispMag, temperature);
                posX[i] += (dispX[i] / dispMag) * capped;
                posY[i] += (dispY[i] / dispMag) * capped;
            }
        }
        
        temperature *= coolingFactor;
        if (temperature < 0.1) break;
    }
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodePositions_[nodes[i]] = QPointF(posX[i], posY[i]);
    }
}

void GraphVisualization::setLayoutParameters(const LayoutParameters& parameters)
{
    layoutParameters_ = parameters;
    if (graph_.getNodeCount() > 0) {
        layoutGraph();
    }
}

void GraphVisualization::setForceAtlasParameters(double repulsion, double attraction, double gravity, int iterations)
{
    layoutParameters_.forceAtlasRepulsion = repulsion;
    layoutParameters_.forceAtlasAttraction = attraction;
    layoutParameters_.forceAtlasGravity = gravity;
    layoutParameters_.forceAtlasIterations = iterations;
    
    if (graph_.getNodeCount() > 0 && currentLayoutType_ == LayoutType::ForceAtlas) {
        layoutGraph();
    }
}

void GraphVisualization::setSpringForceParameters(double springConstant, double length, double damping, int iterations)
{
    layoutParameters_.springConstant = springConstant;
    layoutParameters_.springOptimalDistance = length;
    layoutParameters_.springDamping = damping;
    layoutParameters_.springIterations = iterations;
    
    if (graph_.getNodeCount() > 0 && currentLayoutType_ == LayoutType::SpringForce) {
        layoutGraph();
    }
}

void GraphVisualization::setCircularParameters(double radius, double startAngle, bool clockwise)
{
    layoutParameters_.circularRadius = radius;
    layoutParameters_.circularStartAngle = startAngle;
    layoutParameters_.circularClockwise = clockwise;
    
    if (graph_.getNodeCount() > 0 && currentLayoutType_ == LayoutType::Circular) {
        layoutGraph();
    }
}

void GraphVisualization::setGridParameters(double spacing, int columns)
{
    layoutParameters_.gridSpacing = spacing;
    layoutParameters_.gridColumns = columns;
    
    if (graph_.getNodeCount() > 0 && currentLayoutType_ == LayoutType::Grid) {
        layoutGraph();
    }
}

void GraphVisualization::setRandomParameters(double width, double height, int seed)
{
    layoutParameters_.randomWidth = width;
    layoutParameters_.randomHeight = height;
    layoutParameters_.randomSeed = seed;
    
    if (graph_.getNodeCount() > 0 && currentLayoutType_ == LayoutType::Random) {
        layoutGraph();
    }
}

void GraphVisualization::setVisualizationEnabled(bool enabled)
{
    visualizationEnabled_ = enabled;
    if (visualizationEnabled_) {
        if (graph_.getNodeCount() > 0) {
            layoutGraph();
        }
    } else {
        if (scene_) {
            try {
                scene_->clear();
                nodeItems_.clear();
                edgeItems_.clear();
            } catch (...) {}
        }
    }
    update();
}

void GraphVisualization::resolveOverlaps()
{
    int nodeCount = graph_.getNodeCount();
    int maxPasses = nodeCount < 50 ? 10 : (nodeCount < 100 ? 5 : 2);
    const double minSeparation = nodeAppearance_.radius * 2.0;

    for (int pass = 0; pass < maxPasses; ++pass) {
        bool hasOverlaps = false;
        
        auto nodes = graph_.getNodes();
        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i + 1; j < nodes.size(); ++j) {
                int node1 = nodes[i];
                int node2 = nodes[j];
                
                QPointF pos1 = nodePositions_[node1];
                QPointF pos2 = nodePositions_[node2];
                
                double distance = std::sqrt(std::pow(pos1.x() - pos2.x(), 2) + std::pow(pos1.y() - pos2.y(), 2));
                
                if (distance < minSeparation && distance > 0) {
                    hasOverlaps = true;
                    
                    QPointF delta = pos1 - pos2;
                    double separation = (minSeparation - distance) / 2.0;
                    
                    QPointF unitVector = delta / distance;
                    QPointF separationVector = unitVector * separation;
                    
                    nodePositions_[node1] += separationVector;
                    nodePositions_[node2] -= separationVector;
                }
            }
        }
        
        if (!hasOverlaps) {
            break;
        }
    }
}

void GraphVisualization::updateEdgeOpacity()
{
    if (!visualizationEnabled_ || edgeItems_.empty()) {
        return;
    }
    
    auto edges = graph_.getEdges();
    size_t edgeIndex = 0;
    
    for (const auto& edge : edges) {
        if (edgeIndex >= edgeItems_.size()) break;
        
        int node1 = edge.first;
        int node2 = edge.second;
        
        bool isConnectedToHighlight = highlightedNodeIds_.count(node1) > 0 || highlightedNodeIds_.count(node2) > 0;
        
        QColor edgeColor = edgeAppearance_.color;
        if (isConnectedToHighlight) {
            edgeColor.setAlpha(200);
        } else {
            edgeColor.setAlpha(60);
        }
        
        QPen pen = edgeItems_[edgeIndex]->pen();
        pen.setColor(edgeColor);
        edgeItems_[edgeIndex]->setPen(pen);
        
        edgeIndex++;
    }
}

QGraphicsEllipseItem* GraphVisualization::getNodeItemAt(const QPointF& scenePos)
{
    QGraphicsItem* item = scene_->itemAt(scenePos, QTransform());
    return qgraphicsitem_cast<QGraphicsEllipseItem*>(item);
}

int GraphVisualization::getNodeIdFromItem(QGraphicsEllipseItem* item)
{
    if (!item) return -1;
    
    QVariant data = item->data(0);
    if (data.isValid()) {
        return data.toInt();
    }
    
    for (const auto& pair : nodeItems_) {
        if (pair.second == item) {
            return pair.first;
        }
    }
    return -1;
}

void GraphVisualization::showNodeTooltip(const QPointF& globalPos, int nodeId)
{
    if (nodeId == -1) return;
    
    std::string nodeName = graph_.getNodeName(nodeId);
    auto neighbors = graph_.getNeighbors(nodeId);
    int degree = neighbors.size();
    
    QString tooltipText = QString("Node: %1\nID: %2\nDegree: %3")
                         .arg(QString::fromStdString(nodeName))
                         .arg(nodeId)
                         .arg(degree);
    
    QToolTip::showText(globalPos.toPoint(), tooltipText);
}

void GraphVisualization::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsEllipseItem* nodeItem = getNodeItemAt(scenePos);
        
        if (nodeItem) {
            nodeItem->setCursor(Qt::ClosedHandCursor);
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void GraphVisualization::mouseMoveEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());

    if (event->buttons() & Qt::LeftButton && scene_ && !scene_->selectedItems().isEmpty()) {
        updateEdgePositions();
    }

    QGraphicsEllipseItem* nodeItem = getNodeItemAt(scenePos);

    if (nodeItem != lastHoveredNode_) {
        if (lastHoveredNode_) {
            QToolTip::hideText();
        }

        lastHoveredNode_ = nodeItem;

        if (nodeItem && !(event->buttons() & Qt::LeftButton)) {
            int nodeId = getNodeIdFromItem(nodeItem);
            if (nodeId != -1) {
                showNodeTooltip(event->globalPosition().toPoint(), nodeId);
                applyHoverHighlight(nodeId);
            }
        } else {
            clearHoverHighlight();
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

void GraphVisualization::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        for (auto& pair : nodeItems_) {
            int nodeId = pair.first;
            QGraphicsEllipseItem* item = pair.second;
            if (item) {
                QPointF center = item->sceneBoundingRect().center();
                nodePositions_[nodeId] = center;
                item->setCursor(Qt::OpenHandCursor);
            }
        }

        updateEdgePositions();
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GraphVisualization::leaveEvent(QEvent *event)
{
    clearHoverHighlight();
    lastHoveredNode_ = nullptr;
    QToolTip::hideText();
    QGraphicsView::leaveEvent(event);
}

void GraphVisualization::updateEdgePositions()
{
    if (!scene_ || !edgePathItem_) return;

    QPainterPath newPath;
    auto edges = graph_.getEdges();
    
    int edgeCount = edges.size();
    int maxEdgesToDraw = 100000;
    int step = (edgeCount > maxEdgesToDraw) ? (edgeCount + maxEdgesToDraw - 1) / maxEdgesToDraw : 1;
    
    int index = 0;
    int drawnCount = 0;
    
    for (const auto& edge : edges) {
        if (step > 1 && (index++ % step) != 0) continue;
        
        auto it1 = nodeItems_.find(edge.first);
        auto it2 = nodeItems_.find(edge.second);
        
        if (it1 != nodeItems_.end() && it2 != nodeItems_.end() && it1->second && it2->second) {
            QPointF pos1 = it1->second->sceneBoundingRect().center();
            QPointF pos2 = it2->second->sceneBoundingRect().center();
            newPath.moveTo(pos1);
            newPath.lineTo(pos2);
            drawnCount++;
        }
        
        if (drawnCount >= maxEdgesToDraw) break;
    }
    
    edgePathItem_->setPath(newPath);
}

void GraphVisualization::setNodePositions(const std::map<int, QPointF>& positions)
{
    nodePositions_ = positions;
    
    for (const auto& [nodeId, pos] : positions) {
        auto it = nodeItems_.find(nodeId);
        if (it != nodeItems_.end() && it->second) {
            double radius = nodeAppearance_.radius;
            it->second->setPos(pos.x() - radius, pos.y() - radius);
        }
    }
    
    updateEdgePositions();
}

void GraphVisualization::colorByAttribute(int attributeIndex)
{
    if (graph_.getNodeCount() == 0) return;
    
    auto nodes = graph_.getNodes();
    std::map<int, double> values;
    
    switch (attributeIndex) {
        case 0:
            for (auto& [nodeId, item] : nodeItems_) {
                if (item) {
                    item->setBrush(QBrush(nodeAppearance_.color));
                }
            }
            return;
            
        case 1:
            for (int node : nodes) {
                values[node] = static_cast<double>(graph_.getNeighbors(node).size());
            }
            break;
            
        case 2: {
            auto edges = graph_.getEdges();
            std::map<int, std::vector<int>> adj;
            for (int v : nodes) adj[v] = {};
            for (const auto& e : edges) {
                adj[e.first].push_back(e.second);
                adj[e.second].push_back(e.first);
            }
            
            for (int v : nodes) values[v] = 0.0;
            
            for (int s : nodes) {
                std::stack<int> S;
                std::map<int, std::vector<int>> P;
                std::map<int, double> sigma, d, delta;
                
                for (int t : nodes) {
                    P[t] = {};
                    sigma[t] = 0.0;
                    d[t] = -1.0;
                    delta[t] = 0.0;
                }
                sigma[s] = 1.0;
                d[s] = 0.0;
                
                std::queue<int> Q;
                Q.push(s);
                
                while (!Q.empty()) {
                    int v = Q.front(); Q.pop();
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
                
                while (!S.empty()) {
                    int w = S.top(); S.pop();
                    for (int v : P[w]) {
                        delta[v] += (sigma[v] / sigma[w]) * (1.0 + delta[w]);
                    }
                    if (w != s) {
                        values[w] += delta[w];
                    }
                }
            }
            
            int n = nodes.size();
            if (n > 2) {
                double norm = 2.0 / ((n - 1) * (n - 2));
                for (int v : nodes) values[v] *= norm;
            }
            break;
        }
            
        case 3: {
            auto edges = graph_.getEdges();
            std::map<int, std::vector<int>> adj;
            for (int v : nodes) adj[v] = {};
            for (const auto& e : edges) {
                adj[e.first].push_back(e.second);
                adj[e.second].push_back(e.first);
            }
            
            for (int s : nodes) {
                std::queue<int> Q;
                std::map<int, int> dist;
                for (int v : nodes) dist[v] = -1;
                dist[s] = 0;
                Q.push(s);
                
                double totalDist = 0.0;
                int reachable = 0;
                
                while (!Q.empty()) {
                    int v = Q.front(); Q.pop();
                    for (int w : adj[v]) {
                        if (dist[w] < 0) {
                            dist[w] = dist[v] + 1;
                            totalDist += dist[w];
                            reachable++;
                            Q.push(w);
                        }
                    }
                }
                
                values[s] = (reachable > 0) ? (reachable / totalDist) : 0.0;
            }
            break;
        }
            
        case 4: {
            auto edges = graph_.getEdges();
            std::map<int, std::set<int>> adj;
            for (int v : nodes) adj[v] = {};
            for (const auto& e : edges) {
                adj[e.first].insert(e.second);
                adj[e.second].insert(e.first);
            }
            
            for (int v : nodes) {
                auto& neighbors = adj[v];
                int k = neighbors.size();
                
                if (k < 2) {
                    values[v] = 0.0;
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
                
                values[v] = (2.0 * triangles) / (k * (k - 1));
            }
            break;
        }
            
        case 5: {
            auto edges = graph_.getEdges();
            std::map<int, std::vector<int>> inLinks;
            std::map<int, int> outDegree;
            
            for (int v : nodes) {
                inLinks[v] = {};
                outDegree[v] = 0;
            }
            
            for (const auto& e : edges) {
                inLinks[e.second].push_back(e.first);
                inLinks[e.first].push_back(e.second);
                outDegree[e.first]++;
                outDegree[e.second]++;
            }
            
            int n = nodes.size();
            double d = 0.85;
            for (int v : nodes) values[v] = 1.0 / n;
            
            for (int iter = 0; iter < 50; ++iter) {
                std::map<int, double> newPR;
                for (int v : nodes) {
                    double sum = 0.0;
                    for (int u : inLinks[v]) {
                        if (outDegree[u] > 0) {
                            sum += values[u] / outDegree[u];
                        }
                    }
                    newPR[v] = (1.0 - d) / n + d * sum;
                }
                values = newPR;
            }
            break;
        }
            
        default:
            return;
    }
    
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();
    for (const auto& [node, val] : values) {
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
    }
    
    QString attributeName;
    switch (attributeIndex) {
        case 1: attributeName = "Degree"; break;
        case 2: attributeName = "Betweenness"; break;
        case 3: attributeName = "Closeness"; break;
        case 4: attributeName = "Clustering"; break;
        case 5: attributeName = "PageRank"; break;
        default: attributeName = "Value"; break;
    }
    
    for (const auto& [nodeId, val] : values) {
        auto it = nodeItems_.find(nodeId);
        if (it == nodeItems_.end() || !it->second) continue;
        
        double normalized = (maxVal > minVal) ? (val - minVal) / (maxVal - minVal) : 0.5;
        
        QColor color;
        if (normalized < 0.5) {
            double t = normalized * 2.0;
            color = QColor(
                static_cast<int>(59 + t * (250 - 59)),
                static_cast<int>(130 + t * (204 - 130)),
                static_cast<int>(246 + t * (21 - 246))
            );
        } else {
            double t = (normalized - 0.5) * 2.0;
            color = QColor(
                static_cast<int>(250 + t * (239 - 250)),
                static_cast<int>(204 + t * (68 - 204)),
                static_cast<int>(21 + t * (68 - 21))
            );
        }
        
        it->second->setBrush(QBrush(color));
        
        it->second->setPen(QPen(QColor(30, 30, 30), 1.0));
        
        QString tooltip = QString("Node: %1\n%2: %3")
            .arg(QString::fromStdString(graph_.getNodeName(nodeId)))
            .arg(attributeName)
            .arg(val, 0, 'f', 4);
        it->second->setToolTip(tooltip);
    }
    
    emit attributeColoringApplied(attributeName, minVal, maxVal);
}

void GraphVisualization::applyHoverHighlight(int nodeId)
{
    if (hoverHighlightActive_ && hoverHighlightNodeId_ == nodeId) {
        return;
    }
    
    clearHoverHighlight();
    
    if (nodeId < 0) return;
    
    hoverHighlightNodeId_ = nodeId;
    hoverHighlightActive_ = true;
    
    auto neighborsVec = graph_.getNeighbors(nodeId);
    std::set<int> neighbors(neighborsVec.begin(), neighborsVec.end());
    neighbors.insert(nodeId);
    
    for (auto& pair : nodeItems_) {
        if (!pair.second) continue;
        
        originalNodeOpacities_[pair.first] = pair.second->opacity();
        
        if (neighbors.find(pair.first) != neighbors.end()) {
            pair.second->setOpacity(1.0);
            pair.second->setZValue(pair.second->zValue() + 1000);
        } else {
            pair.second->setOpacity(0.15);
        }
    }
    
    if (edgePathItem_) {
        originalEdgePathOpacity_ = edgePathItem_->opacity();
        edgePathItem_->setOpacity(0.08);
    }
    
    for (auto* edgeItem : edgeItems_) {
        if (!edgeItem) continue;
        edgeItem->setOpacity(0.08);
    }
    
    if (!edgeItems_.empty()) {
        auto edges = graph_.getEdges();
        size_t idx = 0;
        for (const auto& edge : edges) {
            if (idx >= edgeItems_.size()) break;
            
            bool connected = (edge.first == nodeId || edge.second == nodeId);
            if (connected && edgeItems_[idx]) {
                edgeItems_[idx]->setOpacity(1.0);
                edgeItems_[idx]->setZValue(edgeItems_[idx]->zValue() + 500);
                edgeItems_[idx]->setPen(QPen(QColor(100, 200, 255), 2.0));
            }
            idx++;
        }
    }
}

void GraphVisualization::clearHoverHighlight()
{
    if (!hoverHighlightActive_) return;
    
    for (auto& pair : nodeItems_) {
        if (!pair.second) continue;
        
        auto it = originalNodeOpacities_.find(pair.first);
        if (it != originalNodeOpacities_.end()) {
            pair.second->setOpacity(it->second);
        } else {
            pair.second->setOpacity(1.0);
        }
        
        if (hoverHighlightNodeId_ >= 0) {
            auto neighborsVec = graph_.getNeighbors(hoverHighlightNodeId_);
            std::set<int> neighbors(neighborsVec.begin(), neighborsVec.end());
            neighbors.insert(hoverHighlightNodeId_);
            if (neighbors.find(pair.first) != neighbors.end()) {
                pair.second->setZValue(pair.second->zValue() - 1000);
            }
        }
    }
    
    if (edgePathItem_) {
        edgePathItem_->setOpacity(originalEdgePathOpacity_);
    }
    
    for (auto* edgeItem : edgeItems_) {
        if (!edgeItem) continue;
        edgeItem->setOpacity(1.0);
        edgeItem->setZValue(edgeItem->zValue() - 500);
        edgeItem->setPen(QPen(edgeAppearance_.color, edgeAppearance_.width));
    }
    
    originalNodeOpacities_.clear();
    hoverHighlightNodeId_ = -1;
    hoverHighlightActive_ = false;
}
