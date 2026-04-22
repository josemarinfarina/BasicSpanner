#ifndef LARGENETWORKSMANAGER_H
#define LARGENETWORKSMANAGER_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>

class AppController;

/**
 * @brief Manages the Large Networks tab functionality
 * 
 * The LargeNetworksManager class encapsulates all logic for the "Large Networks" tab,
 * including example network loading, network generation, and related UI components.
 * It communicates with the AppController to delegate business logic operations.
 * 
 */
class LargeNetworksManager : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param controller The application controller for business logic delegation
     * @param parent Parent widget
     */
    explicit LargeNetworksManager(AppController* controller, QWidget* parent = nullptr);

private slots:
    void onExampleNetworkSelected();
    void loadSelectedExample();
    void generateAndLoadNetwork();

private:
    void setupUI();
    void populateExampleList();

    AppController* appController_;
    
    QListWidget* exampleNetworksList_;
    QPushButton* loadExampleButton_;
    QLabel* exampleInfoLabel_;
    QTextEdit* exampleDescriptionText_;
    
    QComboBox* networkTypeCombo_;
    QSpinBox* networkSizeSpinBox_;
    QSpinBox* seedValueSpinBox_;
    QPushButton* generateNetworkButton_;

signals:
    void exampleNetworkRequested(const QString& filePath);
    void networkGenerationRequested(const QString& networkType, int size, int seed);
};

#endif