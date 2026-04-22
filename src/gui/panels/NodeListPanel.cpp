/**
 * @file NodeListPanel.cpp
 * @brief Implementation of the node list panel
 */

#include "NodeListPanel.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QEvent>
#include <algorithm>

NodeListPanel::NodeListPanel(QWidget *parent)
    : QWidget(parent)
    , searchBox_(nullptr)
    , nodeTable_(nullptr)
    , countLabel_(nullptr)
{
    setupUI();
}

bool NodeListPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == nodeTable_->viewport()) {
        if (event->type() == QEvent::Leave) {
            emit nodeUnhovered();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void NodeListPanel::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    searchBox_ = new QLineEdit;
    searchBox_->setPlaceholderText("Search nodes...");
    searchBox_->setClearButtonEnabled(true);
    layout->addWidget(searchBox_);

    countLabel_ = new QLabel("0 nodes");
    countLabel_->setStyleSheet("color: #94a3b8; font-size: 11px;");
    layout->addWidget(countLabel_);

    nodeTable_ = new QTableWidget;
    nodeTable_->setColumnCount(7);
    nodeTable_->setHorizontalHeaderLabels({"Node", "Type", "Deg(G)", "Deg(L)", "Betw(G)", "Betw(L)", "Freq %"});
    nodeTable_->horizontalHeader()->setStretchLastSection(false);
    nodeTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int col = 1; col < 7; ++col) {
        nodeTable_->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    }
    nodeTable_->verticalHeader()->setVisible(false);
    nodeTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    nodeTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    nodeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    nodeTable_->setMouseTracking(true);
    nodeTable_->setAlternatingRowColors(true);
    nodeTable_->setSortingEnabled(true);
    
    nodeTable_->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e293b;
            alternate-background-color: #0f172a;
            color: #e2e8f0;
            gridline-color: #334155;
            border: 1px solid #334155;
            border-radius: 6px;
            font-size: 11px;
        }
        QTableWidget::item {
            padding: 4px 6px;
            border: none;
        }
        QTableWidget::item:hover {
            background-color: #3b82f6;
            color: white;
        }
        QTableWidget::item:selected {
            background-color: #2563eb;
            color: white;
        }
        QHeaderView::section {
            background-color: #0f172a;
            color: #00d4aa;
            padding: 6px 4px;
            border: none;
            border-bottom: 2px solid #00d4aa;
            font-weight: bold;
            font-size: 10px;
        }
        QScrollBar:vertical {
            background: #1e293b;
            width: 10px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical {
            background: #475569;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #64748b;
        }
    )");

    layout->addWidget(nodeTable_, 1);

    connect(searchBox_, &QLineEdit::textChanged, this, &NodeListPanel::onSearchTextChanged);
    connect(nodeTable_, &QTableWidget::cellEntered, this, &NodeListPanel::onCellEntered);
    connect(nodeTable_, &QTableWidget::itemClicked, this, &NodeListPanel::onItemClicked);
    
    nodeTable_->viewport()->installEventFilter(this);
}

void NodeListPanel::setNodeData(const std::vector<NodeData>& nodeData)
{
    allNodeData_ = nodeData;
    filterNodes(searchBox_->text());
}

void NodeListPanel::setNodes(const std::vector<std::string>& nodeNames,
                              const std::map<std::string, QString>& nodeTypes)
{
    allNodeData_.clear();
    allNodeData_.reserve(nodeNames.size());
    
    for (const auto& name : nodeNames) {
        NodeData data;
        data.name = name;
        data.degreeGeneral = nodeDegrees_.count(name) ? nodeDegrees_[name] : 0;
        data.frequency = nodeFrequencies_.count(name) ? nodeFrequencies_[name] : 0.0;
        
        auto it = nodeTypes.find(name);
        if (it != nodeTypes.end()) {
            data.type = it->second;
        }
        
        allNodeData_.push_back(data);
    }
    
    filterNodes(searchBox_->text());
}

void NodeListPanel::updateDegrees(const std::map<std::string, int>& degrees)
{
    nodeDegrees_ = degrees;
    
    for (auto& node : allNodeData_) {
        auto it = degrees.find(node.name);
        if (it != degrees.end()) {
            node.degreeGeneral = it->second;
        }
    }
    
    filterNodes(searchBox_->text());
}

void NodeListPanel::updateFrequencies(const std::map<std::string, double>& frequencies)
{
    nodeFrequencies_ = frequencies;
    
    for (auto& node : allNodeData_) {
        auto it = frequencies.find(node.name);
        if (it != frequencies.end()) {
            node.frequency = it->second;
        }
    }
    
    filterNodes(searchBox_->text());
}

void NodeListPanel::clearNodes()
{
    allNodeData_.clear();
    nodeDegrees_.clear();
    nodeFrequencies_.clear();
    nodeTable_->setRowCount(0);
    countLabel_->setText("0 nodes");
}

void NodeListPanel::updateNodeTypes(const std::map<std::string, QString>& nodeTypes)
{
    for (auto& node : allNodeData_) {
        auto it = nodeTypes.find(node.name);
        if (it != nodeTypes.end()) {
            node.type = it->second;
        } else {
            node.type = "Regular";
        }
    }
    
    filterNodes(searchBox_->text());
}

void NodeListPanel::onSearchTextChanged(const QString& text)
{
    filterNodes(text);
}

void NodeListPanel::filterNodes(const QString& filter)
{
    nodeTable_->setSortingEnabled(false);
    nodeTable_->setRowCount(0);
    
    QString lowerFilter = filter.toLower();
    int visibleCount = 0;
    
    for (const auto& nodeData : allNodeData_) {
        QString qName = QString::fromStdString(nodeData.name);
        
        if (filter.isEmpty() || qName.toLower().contains(lowerFilter)) {
            int row = nodeTable_->rowCount();
            nodeTable_->insertRow(row);
            
            QTableWidgetItem* nameItem = new QTableWidgetItem(qName);
            nameItem->setData(Qt::UserRole, qName);
            nodeTable_->setItem(row, 0, nameItem);
            
            QColor typeColor(100, 100, 100);
            if (nodeData.type == "Seed") {
                typeColor = QColor(255, 107, 107);
            } else if (nodeData.type == "Connector") {
                typeColor = QColor(59, 130, 246);
            } else if (nodeData.type == "Extra Connector") {
                typeColor = QColor(251, 191, 36);
            }
            QTableWidgetItem* typeItem = new QTableWidgetItem(nodeData.type);
            typeItem->setForeground(typeColor);
            typeItem->setTextAlignment(Qt::AlignCenter);
            nodeTable_->setItem(row, 1, typeItem);
            
            QTableWidgetItem* degGItem = new QTableWidgetItem();
            degGItem->setData(Qt::DisplayRole, nodeData.degreeGeneral);
            degGItem->setTextAlignment(Qt::AlignCenter);
            nodeTable_->setItem(row, 2, degGItem);
            
            QTableWidgetItem* degLItem = new QTableWidgetItem();
            degLItem->setData(Qt::DisplayRole, nodeData.degreeLocal);
            degLItem->setTextAlignment(Qt::AlignCenter);
            nodeTable_->setItem(row, 3, degLItem);
            
            QTableWidgetItem* betwGItem = new QTableWidgetItem();
            betwGItem->setData(Qt::DisplayRole, QString::number(nodeData.betweennessGeneral, 'f', 4));
            betwGItem->setTextAlignment(Qt::AlignCenter);
            nodeTable_->setItem(row, 4, betwGItem);
            
            QTableWidgetItem* betwLItem = new QTableWidgetItem();
            betwLItem->setData(Qt::DisplayRole, QString::number(nodeData.betweennessLocal, 'f', 4));
            betwLItem->setTextAlignment(Qt::AlignCenter);
            nodeTable_->setItem(row, 5, betwLItem);
            
            QTableWidgetItem* freqItem = new QTableWidgetItem();
            if (nodeData.frequency > 0) {
                freqItem->setData(Qt::DisplayRole, QString::number(nodeData.frequency, 'f', 1));
                if (nodeData.frequency >= 80) {
                    freqItem->setForeground(QColor(34, 197, 94));
                } else if (nodeData.frequency >= 50) {
                    freqItem->setForeground(QColor(251, 191, 36));
                } else {
                    freqItem->setForeground(QColor(148, 163, 184));
                }
            } else {
                freqItem->setData(Qt::DisplayRole, "-");
                freqItem->setForeground(QColor(71, 85, 105));
            }
            freqItem->setTextAlignment(Qt::AlignCenter);
            nodeTable_->setItem(row, 6, freqItem);
            
            visibleCount++;
        }
    }
    
    nodeTable_->setSortingEnabled(true);
    
    countLabel_->setText(QString("%1 nodes").arg(visibleCount));
    if (!filter.isEmpty() && visibleCount < static_cast<int>(allNodeData_.size())) {
        countLabel_->setText(QString("%1 / %2 nodes").arg(visibleCount).arg(allNodeData_.size()));
    }
}

void NodeListPanel::onCellEntered(int row, int column)
{
    Q_UNUSED(column);
    
    QTableWidgetItem* item = nodeTable_->item(row, 0);
    if (item) {
        QString nodeName = item->data(Qt::UserRole).toString();
        emit nodeHovered(nodeName.toStdString());
    }
}

void NodeListPanel::onItemClicked(QTableWidgetItem* item)
{
    if (!item) return;
    
    int row = item->row();
    QTableWidgetItem* nameItem = nodeTable_->item(row, 0);
    if (nameItem) {
        QString nodeName = nameItem->data(Qt::UserRole).toString();
        emit nodeSelected(nodeName.toStdString());
    }
}
