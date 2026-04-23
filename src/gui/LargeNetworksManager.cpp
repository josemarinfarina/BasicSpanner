#include "LargeNetworksManager.h"
#include "AppController.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QAbstractItemView>
#include <QDebug>

LargeNetworksManager::LargeNetworksManager(AppController* controller, QWidget* parent)
    : QWidget(parent)
    , appController_(controller)
    , exampleNetworksList_(nullptr)
    , loadExampleButton_(nullptr)
    , exampleInfoLabel_(nullptr)
    , exampleDescriptionText_(nullptr)
    , networkTypeCombo_(nullptr)
    , networkSizeSpinBox_(nullptr)
    , seedValueSpinBox_(nullptr)
    , generateNetworkButton_(nullptr)
{
    setupUI();
    populateExampleList();
}

void LargeNetworksManager::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    QWidget* leftPanel = new QWidget;
    leftPanel->setMaximumWidth(400);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    
    QGroupBox* examplesGroup = new QGroupBox("Available Example Networks");
    QVBoxLayout* examplesLayout = new QVBoxLayout(examplesGroup);
    
    exampleNetworksList_ = new QListWidget;
    exampleNetworksList_->setSelectionMode(QAbstractItemView::SingleSelection);
    
    loadExampleButton_ = new QPushButton("Load Selected Network");
    loadExampleButton_->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    loadExampleButton_->setEnabled(false);
    
    examplesLayout->addWidget(exampleNetworksList_);
    examplesLayout->addWidget(loadExampleButton_);
    
    QGroupBox* generatorGroup = new QGroupBox("Generate New Network");
    QFormLayout* generatorLayout = new QFormLayout(generatorGroup);
    
    networkTypeCombo_ = new QComboBox;
    networkTypeCombo_->addItem("Random Network", "random");
    networkTypeCombo_->addItem("Scale-Free Network", "scale_free");
    networkTypeCombo_->addItem("Small-World Network", "small_world");
    networkTypeCombo_->addItem("Hierarchical Network", "hierarchical");
    
    networkSizeSpinBox_ = new QSpinBox;
    networkSizeSpinBox_->setMinimum(50);
    networkSizeSpinBox_->setMaximum(10000);
    networkSizeSpinBox_->setValue(500);
    
    seedValueSpinBox_ = new QSpinBox;
    seedValueSpinBox_->setMinimum(1);
    seedValueSpinBox_->setMaximum(9999);
    seedValueSpinBox_->setValue(42);
    
    generateNetworkButton_ = new QPushButton("Generate Network");
    generateNetworkButton_->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 8px; }");
    
    generatorLayout->addRow("Network Type:", networkTypeCombo_);
    generatorLayout->addRow("Size:", networkSizeSpinBox_);
    generatorLayout->addRow("Seed:", seedValueSpinBox_);
    generatorLayout->addRow("", generateNetworkButton_);
    
    leftLayout->addWidget(examplesGroup);
    leftLayout->addWidget(generatorGroup);
    leftLayout->addStretch();
    
    QWidget* rightPanel = new QWidget;
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    QGroupBox* infoGroup = new QGroupBox("Network Information");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    exampleInfoLabel_ = new QLabel("Select a network to view information");
    exampleInfoLabel_->setAlignment(Qt::AlignTop);
    exampleInfoLabel_->setWordWrap(true);
    exampleInfoLabel_->setStyleSheet("font-weight: bold; padding: 10px;");
    
    exampleDescriptionText_ = new QTextEdit;
    exampleDescriptionText_->setReadOnly(true);
    exampleDescriptionText_->setPlaceholderText("Network description will appear here...");
    
    QString networkTypesInfo = R"(
<h3>Available Network Types:</h3>

<h4>Random Network (Erdos-Renyi)</h4>
<p>Each pair of nodes connects with a fixed probability. Useful as a baseline model.</p>

<h4>Scale-Free Network (Barabasi-Albert)</h4>
<p>Follows the "preferential attachment" rule - nodes with more connections attract more connections. Common in social and web networks.</p>

<h4>Small-World Network (Watts-Strogatz)</h4>
<p>Combines high clustering with short paths. Models social and neural networks.</p>

<h4>Hierarchical Network</h4>
<p>Modular structure with hierarchical levels. Useful for modelling multi-level organizations.</p>

<h3>Tips:</h3>
<ul>
<li>Larger networks may take longer to process</li>
<li>Different seeds generate different topologies</li>
<li>Experiment with different types for specific use cases</li>
</ul>
    )";
    
    QTextEdit* helpText = new QTextEdit;
    helpText->setHtml(networkTypesInfo);
    helpText->setReadOnly(true);
    helpText->setMaximumHeight(300);
    
    infoLayout->addWidget(exampleInfoLabel_);
    infoLayout->addWidget(exampleDescriptionText_);
    infoLayout->addWidget(helpText);
    
    rightLayout->addWidget(infoGroup);
    
    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(rightPanel);
    
    connect(exampleNetworksList_, &QListWidget::itemSelectionChanged, 
            this, &LargeNetworksManager::onExampleNetworkSelected);
    connect(loadExampleButton_, &QPushButton::clicked, 
            this, &LargeNetworksManager::loadSelectedExample);
    connect(generateNetworkButton_, &QPushButton::clicked, 
            this, &LargeNetworksManager::generateAndLoadNetwork);
}

void LargeNetworksManager::populateExampleList()
{
    if (!exampleNetworksList_) {
        qDebug() << "[ERROR] exampleNetworksList_ is null!";
        return;
    }
    
    exampleNetworksList_->clear();
    
    QDir examplesDir("examples");
    if (!examplesDir.exists()) {
        if (exampleInfoLabel_) {
            exampleInfoLabel_->setText("'examples' directory not found");
        }
        return;
    }
    
    QStringList filters;
    filters << "*.txt";
    QFileInfoList fileList = examplesDir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo& fileInfo : fileList) {
        QString fileName = fileInfo.fileName();
        
        if (fileName.startsWith("sample_seeds") || fileName.contains("results")) {
            continue;
        }
        
        QString displayName = fileName;
        displayName.remove(".txt");
        
        QString description;
        if (fileName.contains("random")) {
            description = "Random Network";
        } else if (fileName.contains("scale_free")) {
            description = "Scale-Free Network";
        } else if (fileName.contains("small_world")) {
            description = "Small-World Network";
        } else if (fileName.contains("hierarchical")) {
            description = "Hierarchical Network";
        } else {
            description = "Example Network";
        }
        
        QRegularExpression nodeCountRegex("(\\d+)_nodes");
        QRegularExpressionMatch nodeCountMatch = nodeCountRegex.match(fileName);
        if (nodeCountMatch.hasMatch()) {
            QString nodeCount = nodeCountMatch.captured(1);
            description += QString(" (%1 nodes)").arg(nodeCount);
        }
        
        QRegularExpression seedRegex("seed_(\\d+)");
        QRegularExpressionMatch seedMatch = seedRegex.match(fileName);
        if (seedMatch.hasMatch()) {
            QString seed = seedMatch.captured(1);
            description += QString(" [seed %1]").arg(seed);
        }
        
        QListWidgetItem* item = new QListWidgetItem(description);
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        item->setToolTip(fileInfo.absoluteFilePath());
        exampleNetworksList_->addItem(item);
    }
}

void LargeNetworksManager::onExampleNetworkSelected()
{
    QListWidgetItem* current = exampleNetworksList_->currentItem();
    if (!current) {
        loadExampleButton_->setEnabled(false);
        exampleInfoLabel_->setText("Select a network to view information");
        exampleDescriptionText_->clear();
        return;
    }
    
    loadExampleButton_->setEnabled(true);
    QString filePath = current->data(Qt::UserRole).toString();
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        exampleInfoLabel_->setText("Error reading file");
        return;
    }
    
    QTextStream in(&file);
    QString description;
    int nodeCount = 0;
    int edgeCount = 0;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("#")) {
            if (line.contains("Generado con semilla:") || line.contains("description:")) {
                description += line.mid(1).trimmed() + "\n";
            }
        } else if (!line.isEmpty()) {
            QStringList parts = line.split(" ");
            if (parts.size() >= 2) {
                nodeCount = parts[0].toInt();
                edgeCount = parts[1].toInt();
                break;
            }
        }
    }
    
    QString info = QString("<b>File:</b> %1<br>").arg(QFileInfo(filePath).fileName());
    info += QString("<b>Nodes:</b> %1<br>").arg(nodeCount);
    info += QString("<b>Edges:</b> %1<br>").arg(edgeCount);
    if (edgeCount > 0 && nodeCount > 1) {
        double density = (2.0 * edgeCount) / (nodeCount * (nodeCount - 1));
        info += QString("<b>Density:</b> %1<br>").arg(density, 0, 'f', 4);
    }
    
    exampleInfoLabel_->setText(info);
    exampleDescriptionText_->setPlainText(description);
}

void LargeNetworksManager::loadSelectedExample()
{
    QListWidgetItem* currentItem = exampleNetworksList_->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "Warning", "Please select a network from the list.");
        return;
    }
    
    QString filename = currentItem->data(Qt::UserRole).toString();
    
    if (QFile::exists(filename)) {
        emit exampleNetworkRequested(filename);
    } else {
        QMessageBox::warning(this, "Error", 
            QString("Could not find file: %1").arg(filename));
    }
}

void LargeNetworksManager::generateAndLoadNetwork()
{
    QString networkType = networkTypeCombo_->currentData().toString();
    int size = networkSizeSpinBox_->value();
    int seed = seedValueSpinBox_->value();
    
    emit networkGenerationRequested(networkType, size, seed);
} 