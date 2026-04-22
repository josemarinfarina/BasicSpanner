#ifndef GRAPHVISUALIZATION_H
#define GRAPHVISUALIZATION_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QToolTip>
#include <unordered_map>
#include <unordered_set>
#include "Graph.h"

class QVBoxLayout;
class QSlider;
class QLabel;
class QComboBox;

/**
 * @brief Enumeration of available layout algorithms
 */
enum class LayoutType {
    Automatic = 0,
    Circular,
    SpringForce,
    SpringForceParallel,
    Grid,
    Random,
    Hierarchical,
    Radial,
    ForceAtlas,
    ForceAtlasParallel,
    YifanHu,
    YifanHuParallel,
    ForceDirected,
    Spring,
    Circle,
    SeedCentric,
    FWTL
};

/**
 * @brief Configuration parameters for force-directed layouts
 */
struct ForceParameters {
    double repulsion = 1000.0;
    double attraction = 0.1;
    double damping = 0.9;
    double gravity = 0.01;
    double timeStep = 0.1;
    double threshold = 0.01;
    int maxIterations = 1000;
    
    bool operator==(const ForceParameters& other) const {
        return repulsion == other.repulsion &&
               attraction == other.attraction &&
               damping == other.damping &&
               gravity == other.gravity &&
               timeStep == other.timeStep &&
               threshold == other.threshold &&
               maxIterations == other.maxIterations;
    }
};

/**
 * @brief Parameters for ForceAtlas2 algorithm
 */
struct ForceAtlas2Parameters {
    float scalingRatio = 2.0f;
    float gravity = 1.0f;
    float edgeWeightInfluence = 1.0f;
    float jitterTolerance = 1.0f;
    bool preventOverlap = false;
    bool dissuadeHubs = false;
    bool linLogMode = false;
    bool strongGravityMode = false;
    bool adjustSizes = false;
    float barnesHutTheta = 1.2f;
    float outboundAttractionDistribution = false;
    
    bool operator==(const ForceAtlas2Parameters& other) const {
        return scalingRatio == other.scalingRatio &&
               gravity == other.gravity &&
               edgeWeightInfluence == other.edgeWeightInfluence &&
               jitterTolerance == other.jitterTolerance &&
               preventOverlap == other.preventOverlap &&
               dissuadeHubs == other.dissuadeHubs &&
               linLogMode == other.linLogMode &&
               strongGravityMode == other.strongGravityMode &&
               adjustSizes == other.adjustSizes &&
               barnesHutTheta == other.barnesHutTheta &&
               outboundAttractionDistribution == other.outboundAttractionDistribution;
    }
};

struct NodeAppearance {
    QColor color = QColor(0, 212, 170);
    QColor borderColor = QColor(45, 55, 72);
    QColor seedColor = QColor(255, 107, 107);
    QColor connectorColor = QColor(30, 64, 175);
    QColor extraConnectorColor = QColor(255, 193, 7);
    double radius = 12.0;
    double borderWidth = 1.0;
    QColor textColor = QColor(226, 232, 240);
    int fontSize = 8;
    bool showLabels = false;
};

struct EdgeAppearance {
    QColor color = QColor(100, 100, 100);
    double width = 1.2;
    bool cosmetic = true;
};

struct LayoutParameters {
    double forceAtlasRepulsion = 200.0;
    double forceAtlasAttraction = 0.5;
    double forceAtlasGravity = 10.0;
    int forceAtlasIterations = 250;
    double forceAtlasTemperature = 1.0;
    double forceAtlasCooling = 0.98;
    bool forceAtlasPreventOverlap = true;
    
    double springOptimalDistance = 100.0;
    double springTemperature = 200.0;
    double springCoolingFactor = 0.98;
    int springIterations = 200;
    double springConstant = 1.0;
    double springDamping = 0.9;
    
    double yifanOptimalDistance = 100.0;
    double yifanRelativeStrength = 0.2;
    double yifanInitialStepSize = 20.0;
    double yifanStepRatio = 0.95;
    bool yifanAdaptiveCooling = true;
    double yifanConvergenceThreshold = 1.0e-4;
    int yifanQuadtreeMaxLevel = 10;
    double yifanTheta = 1.2;
    
    double circularRadius = 200.0;
    double circularStartAngle = 0.0;
    bool circularClockwise = true;
    
    double hierarchicalLevelSeparation = 100.0;
    double hierarchicalNodeSeparation = 50.0;
    QString hierarchicalOrientation = "top-down";
    
    double gridSpacing = 50.0;
    int gridColumns = 10;
    
    double radialBaseRadius = 50.0;
    double radialRadiusIncrement = 30.0;
    
    double randomWidth = 800.0;
    double randomHeight = 600.0;
    int randomSeed = 42;
    
    int layoutSeed = 12345;
    
    double seedCentricSeedRadius = 300.0;
    double seedCentricSeedRepulsion = 500.0;
    double seedCentricConnectorAttraction = 2.0;
    double seedCentricSeedCohesion = 0.3;
    double seedCentricConnectorGravity = 0.05;
    double seedCentricSeedAntiGravity = 0.02;
    int seedCentricIterations = 300;
    double seedCentricCooling = 0.97;
    bool seedCentricAutoDetectConnectors = true;
    
    double fwtlBaseDistance = 100.0;
    double fwtlFrequencyWeight = 2.0;
    double fwtlSeedAttraction = 5.0;
    double fwtlEdgeAttraction = 1.0;
    double fwtlConfidenceRadius = 0.3;
    double fwtlUncertaintySpread = 2.0;
    int fwtlIterations = 250;
    double fwtlCooling = 0.96;
};

class GraphVisualization : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GraphVisualization(QWidget *parent = nullptr);
    ~GraphVisualization();
    
    void setGraph(const Graph& graph);
    void highlightNodes(const std::set<int>& nodeIds, const QColor& color);
    void highlightSeedsByName(const std::vector<std::string>& seedNames);
    void setCurrentSeeds(const std::vector<std::string>& seeds);
    
    /**
     * @brief Set connector frequency data for FWTL layout algorithm
     * @param connectorFrequency Map of node ID to appearance count in permutations
     * @param totalPermutations Total number of permutations run
     */
    void setConnectorFrequencyData(const std::map<int, int>& connectorFrequency, int totalPermutations);
    void clearHighlights();
    
    /**
     * @brief Highlight connectors with brightness based on frequency
     * @param connectorFrequency Map of connector node ID to appearance count
     * @param totalPermutations Total number of permutations run
     * @param bestConnectors Set of connector IDs from the best result (will use connectorColor)
     * @param connectorColor Color for connectors in the best result
     * @param extraConnectorColor Color for connectors NOT in best result (appeared in other permutations)
     */
    void highlightConnectorsWithFrequency(const std::map<int, int>& connectorFrequency, 
                                          int totalPermutations,
                                          const std::set<int>& bestConnectors,
                                          const QColor& connectorColor,
                                          const QColor& extraConnectorColor);
    void fitToView();
    void resetZoom();
    void setLayoutType(LayoutType type);
    
    /**
     * @brief Temporarily highlight a node (for hover effects)
     * @param nodeName Name of the node to highlight
     */
    void highlightNodeTemporary(const std::string& nodeName);
    
    /**
     * @brief Clear temporary highlight
     */
    void clearTemporaryHighlight();
    LayoutType getLayoutType() const { return currentLayoutType_; }
    void visualizeGraph();
    void showWelcomeMessage();

public slots:
    void displayGraph(const Graph& graph);
    
    void setNodeAppearance(const NodeAppearance& appearance);
    void setEdgeAppearance(const EdgeAppearance& appearance);
    NodeAppearance getNodeAppearance() const { return nodeAppearance_; }
    EdgeAppearance getEdgeAppearance() const { return edgeAppearance_; }
    
    void setNodeColor(const QColor& color);
    void setSeedColor(const QColor& color);
    void setConnectorColor(const QColor& color);
    void setExtraConnectorColor(const QColor& color);
    void setNodeSize(double radius);
    void setNodeBorderColor(const QColor& color);
    void setNodeBorderWidth(double width);
    void setShowNodeLabels(bool show);
    void setNodeFontSize(int size);
    
    void setEdgeColor(const QColor& color);
    void setEdgeWidth(double width);
    
    void setLayoutParameters(const LayoutParameters& parameters);
    LayoutParameters getLayoutParameters() const { return layoutParameters_; }
    
    void setForceAtlasParameters(double repulsion, double attraction, double gravity, int iterations);
    void setSpringForceParameters(double springConstant, double length, double damping, int iterations);
    void setCircularParameters(double radius, double startAngle, bool clockwise);
    void setGridParameters(double spacing, int columns);
    void setRandomParameters(double width, double height, int seed);

    void setVisualizationEnabled(bool enabled);
    
    /**
     * @brief Get current node positions
     * @return Map of node ID to position
     */
    std::map<int, QPointF> getNodePositions() const { return nodePositions_; }
    
    /**
     * @brief Set node positions directly (for syncing between views)
     * @param positions Map of node ID to position
     */
    void setNodePositions(const std::map<int, QPointF>& positions);
    
    /**
     * @brief Color nodes by a centrality/attribute measure
     * @param attributeIndex 0=Seeds/Connectors, 1=Degree, 2=Betweenness, 3=Closeness, 4=Clustering, 5=PageRank
     */
    void colorByAttribute(int attributeIndex);

signals:
    /**
     * @brief Emitted when a node is clicked
     * @param nodeName Name of the clicked node
     */
    void nodeClicked(const std::string& nodeName);
    
    /**
     * @brief Emitted when attribute coloring is applied
     * @param attributeName Name of the attribute
     * @param minVal Minimum value
     * @param maxVal Maximum value
     */
    void attributeColoringApplied(const QString& attributeName, double minVal, double maxVal);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void updateEdgePositions();
    void drawNodesBatched(const std::vector<int>& nodes);
    void layoutGraph();
    void drawNodes();
    void drawEdges();
    void calculateNodePositions();
    void layoutCircular(const std::vector<int>& nodes);
    void layoutSpringForce(const std::vector<int>& nodes);
    void layoutGrid(const std::vector<int>& nodes);
    void layoutRandom(const std::vector<int>& nodes);
    void layoutHierarchical(const std::vector<int>& nodes);
    void layoutRadial(const std::vector<int>& nodes);
    void layoutForceAtlas(const std::vector<int>& nodes);
    void layoutForceAtlasParallel(const std::vector<int>& nodes);
    void layoutYifanHu(const std::vector<int>& nodes);
    void layoutYifanHuParallel(const std::vector<int>& nodes);
    void layoutSpringForceParallel(const std::vector<int>& nodes);
    void layoutSeedCentric(const std::vector<int>& nodes);
    void layoutFWTL(const std::vector<int>& nodes);
    
    void resolveOverlaps();
    void updateEdgeOpacity();
    QGraphicsEllipseItem* getNodeItemAt(const QPointF& scenePos);
    int getNodeIdFromItem(QGraphicsEllipseItem* item);
    void showNodeTooltip(const QPointF& globalPos, int nodeId);
    
    QGraphicsScene* scene_;
    Graph graph_;
    std::map<int, QGraphicsEllipseItem*> nodeItems_;
    std::vector<QGraphicsLineItem*> edgeItems_;
    QGraphicsPathItem* edgePathItem_ = nullptr;
    std::map<int, QPointF> nodePositions_;
    LayoutType currentLayoutType_;
    
    NodeAppearance nodeAppearance_;
    EdgeAppearance edgeAppearance_;
    
    LayoutParameters layoutParameters_;
    bool visualizationEnabled_ = false;
    
    std::unordered_set<int> highlightedNodeIds_;
    QGraphicsEllipseItem* lastHoveredNode_ = nullptr;
    
    std::vector<std::string> currentSeeds_;
    
    std::map<int, int> connectorFrequencyData_;
    int totalPermutationsData_ = 0;
    
    int temporaryHighlightNodeId_ = -1;
    QBrush temporaryHighlightOriginalBrush_;
    QPen temporaryHighlightOriginalPen_;
    QRectF temporaryHighlightOriginalRect_;
    qreal temporaryHighlightOriginalZValue_ = 0;
    
    int hoverHighlightNodeId_ = -1;
    std::map<int, qreal> originalNodeOpacities_;
    qreal originalEdgePathOpacity_ = 1.0;
    bool hoverHighlightActive_ = false;
    
    void applyHoverHighlight(int nodeId);
    void clearHoverHighlight();
};

#endif