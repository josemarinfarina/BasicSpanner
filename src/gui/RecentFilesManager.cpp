/**
 * @file RecentFilesManager.cpp
 * @brief Implementation of RecentFilesManager class
 * 
 * This file contains the implementation of the RecentFilesManager class which
 * manages recently opened files for both seeds and networks.
 */

#include "RecentFilesManager.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QApplication>

RecentFilesManager::RecentFilesManager(QObject *parent)
    : QObject(parent)
    , settings_("BasicSpanner", "RecentFiles")
{
    loadRecentFiles();
}

RecentFilesManager::~RecentFilesManager()
{
    saveRecentFiles();
}

void RecentFilesManager::addRecentNetwork(const QString& filePath)
{
    if (filePath.isEmpty() || !fileExists(filePath)) {
        return;
    }
    
    recentNetworks_.removeAll(filePath);
    
    recentNetworks_.prepend(filePath);
    
    while (recentNetworks_.size() > MAX_RECENT_FILES) {
        recentNetworks_.removeLast();
    }
    
    saveRecentFiles();
}

void RecentFilesManager::addRecentSeeds(const QString& filePath)
{
    if (filePath.isEmpty() || !fileExists(filePath)) {
        return;
    }
    
    recentSeeds_.removeAll(filePath);
    
    recentSeeds_.prepend(filePath);
    
    while (recentSeeds_.size() > MAX_RECENT_FILES) {
        recentSeeds_.removeLast();
    }
    
    saveRecentFiles();
}

QStringList RecentFilesManager::getRecentNetworks() const
{
    return recentNetworks_;
}

QStringList RecentFilesManager::getRecentSeeds() const
{
    return recentSeeds_;
}

QMenu* RecentFilesManager::createRecentNetworksSubmenu(QMenu* parentMenu)
{
    QMenu* submenu = new QMenu("Open Recent Networks", parentMenu);
    updateSubmenuActions(submenu, recentNetworks_, &RecentFilesManager::onRecentNetworkSelected);
    return submenu;
}

QMenu* RecentFilesManager::createRecentSeedsSubmenu(QMenu* parentMenu)
{
    QMenu* submenu = new QMenu("Open Recent Seeds", parentMenu);
    updateSubmenuActions(submenu, recentSeeds_, &RecentFilesManager::onRecentSeedsSelected);
    return submenu;
}

void RecentFilesManager::clearRecentNetworks()
{
    recentNetworks_.clear();
    saveRecentFiles();
}

void RecentFilesManager::clearRecentSeeds()
{
    recentSeeds_.clear();
    saveRecentFiles();
}

void RecentFilesManager::clearAllRecentFiles()
{
    recentNetworks_.clear();
    recentSeeds_.clear();
    saveRecentFiles();
}

void RecentFilesManager::onRecentNetworkSelected(const QString& filePath)
{
    if (fileExists(filePath)) {
        emit recentNetworkSelected(filePath);
    } else {
        qDebug() << "Recent network file not found:" << filePath;
        recentNetworks_.removeAll(filePath);
        saveRecentFiles();
    }
}

void RecentFilesManager::onRecentSeedsSelected(const QString& filePath)
{
    if (fileExists(filePath)) {
        emit recentSeedsSelected(filePath);
    } else {
        qDebug() << "Recent seeds file not found:" << filePath;
        recentSeeds_.removeAll(filePath);
        saveRecentFiles();
    }
}

void RecentFilesManager::loadRecentFiles()
{
    recentNetworks_ = settings_.value("recentNetworks").toStringList();
    recentSeeds_ = settings_.value("recentSeeds").toStringList();
    
    QStringList validNetworks, validSeeds;
    
    for (const QString& network : recentNetworks_) {
        if (fileExists(network)) {
            validNetworks.append(network);
        }
    }
    
    for (const QString& seeds : recentSeeds_) {
        if (fileExists(seeds)) {
            validSeeds.append(seeds);
        }
    }
    
    recentNetworks_ = validNetworks;
    recentSeeds_ = validSeeds;
}

void RecentFilesManager::saveRecentFiles()
{
    settings_.setValue("recentNetworks", recentNetworks_);
    settings_.setValue("recentSeeds", recentSeeds_);
    settings_.sync();
}

void RecentFilesManager::updateSubmenuActions(QMenu* submenu, const QStringList& fileList, 
                                             void (RecentFilesManager::*slotFunction)(const QString&))
{
    submenu->clear();
    
    if (fileList.isEmpty()) {
        QAction* noFilesAction = submenu->addAction("No recent files");
        noFilesAction->setEnabled(false);
        return;
    }
    
    for (const QString& filePath : fileList) {
        QString displayName = getDisplayName(filePath);
        QAction* action = submenu->addAction(displayName);
        
        connect(action, &QAction::triggered, this, [this, filePath, slotFunction]() {
            (this->*slotFunction)(filePath);
        });
        
        action->setToolTip(filePath);
    }
    
    submenu->addSeparator();
    QAction* clearAction = submenu->addAction("Clear Recent Files");
    connect(clearAction, &QAction::triggered, this, [this, submenu]() {
        if (submenu->title().contains("Networks")) {
            clearRecentNetworks();
        } else if (submenu->title().contains("Seeds")) {
            clearRecentSeeds();
        }
        updateSubmenuActions(submenu, 
                           submenu->title().contains("Networks") ? recentNetworks_ : recentSeeds_,
                           submenu->title().contains("Networks") ? 
                           &RecentFilesManager::onRecentNetworkSelected : 
                           &RecentFilesManager::onRecentSeedsSelected);
    });
}

QString RecentFilesManager::getDisplayName(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    QString dirPath = fileInfo.absolutePath();
    
    QString currentDir = QDir::currentPath();
    if (dirPath.startsWith(currentDir)) {
        QString relativePath = dirPath.mid(currentDir.length());
        if (!relativePath.isEmpty()) {
            if (relativePath.startsWith('/')) {
                relativePath = relativePath.mid(1);
            }
            return relativePath + "/" + fileName;
        }
    }
    
    QString homeDir = QDir::homePath();
    if (dirPath.startsWith(homeDir)) {
        QString relativePath = dirPath.mid(homeDir.length());
        if (!relativePath.isEmpty()) {
            if (relativePath.startsWith('/')) {
                relativePath = relativePath.mid(1);
            }
            return "~/" + relativePath + "/" + fileName;
        }
    }
    
    QString parentDir = fileInfo.dir().dirName();
    if (parentDir.isEmpty() || parentDir == ".") {
        return fileName;
    } else {
        return parentDir + "/" + fileName;
    }
}

bool RecentFilesManager::fileExists(const QString& filePath) const
{
    return QFileInfo::exists(filePath);
} 