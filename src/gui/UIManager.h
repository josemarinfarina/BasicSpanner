#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <QObject>
#include <QDockWidget>

class QMainWindow;
class SeedSelectionPanel;
class FileOperationsPanel;
class AlgorithmConfigPanel;
class ExecutionPanel;
class ResultsPanel;
class VisualizationControlPanel;
class NodeListPanel;

/**
 * @brief Manages all dockable UI panels in the application
 * 
 * The UIManager class is responsible for creating, organizing, and managing
 * all the dockable panels in the main window. It handles panel visibility,
 * layout operations, and provides access to panel instances for signal/slot
 * connections.
 * 
 */
class UIManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent The main window that will contain the dock widgets
     */
    explicit UIManager(QMainWindow* parent);

    /**
     * @brief Destructor
     */
    ~UIManager();

    SeedSelectionPanel* getSeedSelectionPanel() const;
    FileOperationsPanel* getFileOperationsPanel() const;
    AlgorithmConfigPanel* getAlgorithmConfigPanel() const;
    ExecutionPanel* getExecutionPanel() const;
    ResultsPanel* getResultsPanel() const;
    VisualizationControlPanel* getVisualizationControlPanel() const;
    NodeListPanel* getNodeListPanel() const;
    
    QDockWidget* getFileOperationsDock() const { return fileOperationsDock_; }
    QDockWidget* getVisualizationControlDock() const { return visualizationControlDock_; }
    QDockWidget* getSeedSelectionDock() const { return seedSelectionDock_; }
    QDockWidget* getAlgorithmConfigDock() const { return algorithmConfigDock_; }
    QDockWidget* getExecutionDock() const { return executionDock_; }
    QDockWidget* getResultsDock() const { return resultsDock_; }
    QDockWidget* getNodeListDock() const { return nodeListDock_; }

public slots:
    void toggleFileOperationsPanel(bool visible);
    void toggleVisualizationPanel(bool visible);
    void toggleSeedSelectionPanel(bool visible);
    void toggleAlgorithmConfigPanel(bool visible);
    void toggleExecutionPanel(bool visible);
    void toggleResultsPanel(bool visible);
    void resetLayout();
    void updateMenuStates();

signals:
    void menuStatesChanged();

private:
    void createPanelsAndDockWidgets();
    void connectPanelSignals();

    QMainWindow* mainWindow_;
    
    SeedSelectionPanel* seedSelectionPanel_;
    FileOperationsPanel* fileOperationsPanel_;
    AlgorithmConfigPanel* algorithmConfigPanel_;
    ExecutionPanel* executionPanel_;
    ResultsPanel* resultsPanel_;
    VisualizationControlPanel* visualizationControlPanel_;
    NodeListPanel* nodeListPanel_;

    QDockWidget* seedSelectionDock_;
    QDockWidget* fileOperationsDock_;
    QDockWidget* algorithmConfigDock_;
    QDockWidget* executionDock_;
    QDockWidget* resultsDock_;
    QDockWidget* visualizationControlDock_;
    QDockWidget* nodeListDock_;
};

#endif