/**
 * @file RecentFilesManager.h
 * @brief Manager for handling recent files (seeds and networks)
 * 
 * This header defines the RecentFilesManager class which manages the list of
 * recently opened files for both seeds and networks, providing functionality
 * to add, retrieve, and persist recent file lists.
 */

#ifndef RECENTFILESMANAGER_H
#define RECENTFILESMANAGER_H

#include <QObject>
#include <QStringList>
#include <QMenu>
#include <QAction>
#include <QSettings>

/**
 * @brief Manager for recent files functionality
 * 
 * The RecentFilesManager class handles the management of recently opened files
 * for both seeds and networks. It provides functionality to:
 * - Add new files to the recent lists
 * - Retrieve recent file lists
 * - Create and manage submenus for recent files
 * - Persist recent file lists across application sessions
 * 
 */
class RecentFilesManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent Parent object
     */
    explicit RecentFilesManager(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~RecentFilesManager();

    /**
     * @brief Add a network file to recent list
     * 
     * @param filePath Path to the network file
     */
    void addRecentNetwork(const QString& filePath);
    
    /**
     * @brief Add a seeds file to recent list
     * 
     * @param filePath Path to the seeds file
     */
    void addRecentSeeds(const QString& filePath);
    
    /**
     * @brief Get recent networks list
     * 
     * @return List of recent network file paths
     */
    QStringList getRecentNetworks() const;
    
    /**
     * @brief Get recent seeds list
     * 
     * @return List of recent seeds file paths
     */
    QStringList getRecentSeeds() const;
    
    /**
     * @brief Create submenu for recent networks
     * 
     * @param parentMenu Parent menu to add the submenu to
     * @return Pointer to the created submenu
     */
    QMenu* createRecentNetworksSubmenu(QMenu* parentMenu);
    
    /**
     * @brief Create submenu for recent seeds
     * 
     * @param parentMenu Parent menu to add the submenu to
     * @return Pointer to the created submenu
     */
    QMenu* createRecentSeedsSubmenu(QMenu* parentMenu);
    
    /**
     * @brief Clear recent networks list
     */
    void clearRecentNetworks();
    
    /**
     * @brief Clear recent seeds list
     */
    void clearRecentSeeds();
    
    /**
     * @brief Clear all recent files
     */
    void clearAllRecentFiles();

private slots:
    /**
     * @brief Handle recent network file selection
     * 
     * @param filePath Path to the selected network file
     */
    void onRecentNetworkSelected(const QString& filePath);
    
    /**
     * @brief Handle recent seeds file selection
     * 
     * @param filePath Path to the selected seeds file
     */
    void onRecentSeedsSelected(const QString& filePath);

private:
    /**
     * @brief Load recent files from settings
     */
    void loadRecentFiles();
    
    /**
     * @brief Save recent files to settings
     */
    void saveRecentFiles();
    
    /**
     * @brief Update submenu actions
     * 
     * @param submenu Submenu to update
     * @param fileList List of files to show
     * @param slotFunction Slot function to connect actions to
     */
    void updateSubmenuActions(QMenu* submenu, const QStringList& fileList, 
                             void (RecentFilesManager::*slotFunction)(const QString&));
    
    /**
     * @brief Get display name for file path
     * 
     * @param filePath Full file path
     * @return Display name (filename with path if needed)
     */
    QString getDisplayName(const QString& filePath) const;
    
    /**
     * @brief Check if file exists
     * 
     * @param filePath Path to check
     * @return True if file exists
     */
    bool fileExists(const QString& filePath) const;

    QStringList recentNetworks_;
    QStringList recentSeeds_;
    
    QSettings settings_;
    
    static const int MAX_RECENT_FILES = 10;

signals:
    /**
     * @brief Signal emitted when a recent network file is selected
     * 
     * @param filePath Path to the selected network file
     */
    void recentNetworkSelected(const QString& filePath);
    
    /**
     * @brief Signal emitted when a recent seeds file is selected
     * 
     * @param filePath Path to the selected seeds file
     */
    void recentSeedsSelected(const QString& filePath);
};

#endif