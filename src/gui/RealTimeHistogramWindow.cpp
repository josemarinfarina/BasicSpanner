/**
 * @file RealTimeHistogramWindow.cpp
 * @brief Implementation of the real-time histogram window
 */

#include "RealTimeHistogramWindow.h"
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QTextEdit>
#include <QProgressBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QCloseEvent>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <QRegularExpression>
#include <QFileDialog>
#include <QScrollArea>
#include <QSplitter>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <map>

#include <QPainter>
#include <QPaintEvent>
#include <QToolTip>
#include <iostream>

#include "DarkTheme.h"

class VerticalHistogramWidget : public QWidget {
public:
    struct BarData {
        QString label;
        int value;
        QString tooltip;
    };
    
    VerticalHistogramWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(280);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMouseTracking(true);
        barColor_ = DarkTheme::accent();
        barColorAlt_ = DarkTheme::danger();
        useAltColor_ = false;
        hoveredBar_ = -1;
        minBarWidth_ = 45;
    }
    
    void setData(const std::vector<int>& values, const QStringList& labels) {
        bars_.clear();
        for (int i = 0; i < std::min(static_cast<int>(values.size()), static_cast<int>(labels.size())); ++i) {
            BarData bar;
            bar.label = labels[i];
            bar.value = values[i];
            bar.tooltip = QString("%1\nCount: %2").arg(labels[i]).arg(values[i]);
            bars_.push_back(bar);
        }
        updateMinimumWidth();
        update();
    }
    
    void setTitle(const QString& title) { title_ = title; update(); }
    void setUseAltColor(bool alt) { useAltColor_ = alt; update(); }
    void clear() { bars_.clear(); title_.clear(); setMinimumWidth(200); update(); }
    
    void updateMinimumWidth() {
        if (bars_.empty()) {
            setMinimumWidth(200);
            return;
        }
        const int margin = 55;
        const int barSpacing = 6;
        int requiredWidth = 2 * margin + (int)bars_.size() * (minBarWidth_ + barSpacing);
        setMinimumWidth(requiredWidth);
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        
        QLinearGradient bgGrad(0, 0, 0, height());
        bgGrad.setColorAt(0, DarkTheme::bgTertiary());
        bgGrad.setColorAt(1, DarkTheme::bgPrimary().darker(110));
        p.fillRect(rect(), bgGrad);
        
        p.setPen(QPen(DarkTheme::border(), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 10, 10);
        
        if (bars_.empty()) {
            p.setPen(DarkTheme::borderLight());
            p.setFont(QFont("Segoe UI", 11));
            p.drawText(rect().adjusted(0, -20, 0, 0), Qt::AlignCenter, "Waiting for data...");
            p.setFont(QFont("Segoe UI", 9));
            p.setPen(DarkTheme::textMuted());
            p.drawText(rect().adjusted(0, 10, 0, 0), Qt::AlignCenter, "Run analysis to see histogram");
            return;
        }
        
        const int margin = 55;
        const int topMargin = 55;
        const int bottomMargin = 45;
        const int barSpacing = 6;
        
        int chartWidth = width() - 2 * margin;
        int chartHeight = height() - topMargin - bottomMargin;
        
        int barWidth = std::max(minBarWidth_, (chartWidth - barSpacing * ((int)bars_.size() + 1)) / std::max(1, (int)bars_.size()));
        
        int startX = margin + barSpacing;
        
        int maxVal = 1;
        int total = 0;
        for (const auto& bar : bars_) {
            if (bar.value > maxVal) maxVal = bar.value;
            total += bar.value;
        }
        
        p.setPen(DarkTheme::textPrimary());
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        QString displayTitle = title_.isEmpty() ? "Connector Distribution" : title_;
        p.drawText(QRect(0, 12, width(), 26), Qt::AlignCenter, displayTitle);
        
        QRect chartArea(margin - 5, topMargin - 5, chartWidth + 10, chartHeight + 10);
        p.setPen(Qt::NoPen);
        p.setBrush(DarkTheme::bgPrimary());
        p.drawRoundedRect(chartArea, 6, 6);
        
        p.setFont(QFont("Segoe UI", 9));
        for (int i = 0; i <= 4; ++i) {
            int y = topMargin + chartHeight - (chartHeight * i / 4);
            int val = maxVal * i / 4;
            
            p.setPen(QPen(DarkTheme::border(), 1, Qt::DotLine));
            p.drawLine(margin, y, width() - margin, y);
            
            p.setPen(DarkTheme::textMuted());
            p.drawText(QRect(5, y - 12, margin - 15, 24), Qt::AlignRight | Qt::AlignVCenter, QString::number(val));
        }
        
        p.setPen(QPen(DarkTheme::borderLight(), 2));
        int baseY = topMargin + chartHeight;
        p.drawLine(margin, baseY, width() - margin, baseY);
        
        int x = startX;
        for (int i = 0; i < (int)bars_.size(); ++i) {
            const auto& bar = bars_[i];
            int barHeight = maxVal > 0 ? (bar.value * chartHeight) / maxVal : 0;
            if (barHeight < 4 && bar.value > 0) barHeight = 4;
            int y = topMargin + chartHeight - barHeight;
            
            QRect barRect(x, y, barWidth, barHeight);
            
            p.setPen(Qt::NoPen);
            QColor glowColor = useAltColor_ ? QColor(255, 107, 107, 40) : QColor(0, 212, 170, 40);
            p.setBrush(glowColor);
            p.drawRoundedRect(barRect.adjusted(-2, 2, 2, 4), 5, 5);
            
            QLinearGradient grad(barRect.left(), 0, barRect.right(), 0);
            QColor baseColor = useAltColor_ ? barColorAlt_ : barColor_;
            
            if (i == hoveredBar_) {
                baseColor = baseColor.lighter(120);
            }
            
            grad.setColorAt(0, baseColor.lighter(110));
            grad.setColorAt(0.5, baseColor);
            grad.setColorAt(1, baseColor.darker(110));
            
            p.setBrush(grad);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(barRect, 5, 5);
            
            if (barHeight > 8) {
                QLinearGradient shine(barRect.topLeft(), barRect.bottomLeft());
                shine.setColorAt(0, QColor(255, 255, 255, 70));
                shine.setColorAt(0.3, QColor(255, 255, 255, 0));
                p.setBrush(shine);
                p.drawRoundedRect(barRect.adjusted(2, 2, -2, -barHeight/2), 4, 4);
            }
            
            if (bar.value > 0) {
                QString valText = QString::number(bar.value);
                p.setFont(QFont("Segoe UI", 9, QFont::Bold));
                
                if (barHeight >= 22) {
                    p.setPen(QColor(255, 255, 255, 230));
                    p.drawText(barRect.adjusted(0, 4, 0, 0), Qt::AlignHCenter | Qt::AlignTop, valText);
                } else {
                    p.setPen(DarkTheme::textPrimary());
                    int textY = y - 18;
                    if (textY < topMargin - 8) textY = topMargin - 8;
                    p.drawText(QRect(x, textY, barWidth, 16), Qt::AlignCenter, valText);
                }
            }
            
            p.setPen(DarkTheme::textSecondary());
            p.setFont(QFont("Segoe UI", 8));
            
            QFontMetrics fm(p.font());
            QString displayLabel = bar.label;
            int labelWidth = barWidth + barSpacing - 2;
            
            if (fm.horizontalAdvance(displayLabel) > labelWidth) {
                displayLabel = fm.elidedText(displayLabel, Qt::ElideRight, labelWidth);
            }
            
            int labelY = baseY + 6;
            p.drawText(QRect(x - barSpacing/2, labelY, barWidth + barSpacing, 32), 
                       Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, displayLabel);
            
            x += barWidth + barSpacing;
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        const int margin = 55;
        const int barSpacing = 6;
        
        if (bars_.empty()) return;
        
        int chartWidth = width() - 2 * margin;
        int barWidth = std::max(minBarWidth_, (chartWidth - barSpacing * ((int)bars_.size() + 1)) / std::max(1, (int)bars_.size()));
        int startX = margin + barSpacing;
        
        int mx = event->pos().x();
        int newHovered = -1;
        
        int x = startX;
        for (int i = 0; i < (int)bars_.size(); ++i) {
            if (mx >= x && mx < x + barWidth) {
                newHovered = i;
                break;
            }
            x += barWidth + barSpacing;
        }
        
        if (newHovered != hoveredBar_) {
            hoveredBar_ = newHovered;
            update();
            
            if (hoveredBar_ >= 0 && hoveredBar_ < (int)bars_.size()) {
                QToolTip::showText(event->globalPosition().toPoint(), bars_[hoveredBar_].tooltip, this);
            }
        }
    }
    
    void leaveEvent(QEvent*) override {
        hoveredBar_ = -1;
        update();
    }
    
private:
    std::vector<BarData> bars_;
    QString title_;
    QColor barColor_;
    QColor barColorAlt_;
    bool useAltColor_;
    int hoveredBar_;
    int minBarWidth_;
};

RealTimeHistogramWindow::RealTimeHistogramWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget_(nullptr)
    , mainLayout_(nullptr)
    , histogramGroup_(nullptr)
    , histogramLayout_(nullptr)
    , histogramWidget_(nullptr)
    , controlGroup_(nullptr)
    , controlLayout_(nullptr)
    , refreshButton_(nullptr)
    , downloadButton_(nullptr)
    , autoRefreshCheckBox_(nullptr)
    , binCountSpinBox_(nullptr)
    , showStatisticsCheckBox_(nullptr)
    , binCountLabel_(nullptr)
    , statisticsGroup_(nullptr)
    , statisticsLayout_(nullptr)
    , totalPermutationsLabel_(nullptr)
    , meanLabel_(nullptr)
    , medianLabel_(nullptr)
    , stdDevLabel_(nullptr)
    , minLabel_(nullptr)
    , maxLabel_(nullptr)
    , rangeLabel_(nullptr)
    , dataGroup_(nullptr)
    , dataLayout_(nullptr)
    , dataTable_(nullptr)
    , autoRefreshTimer_(nullptr)
    , fileWatcher_(nullptr)
    , updateThrottleTimer_(nullptr)
    , updatePending_(false)
    , autoRefreshEnabled_(true)
    , binCount_(15)
    , showStatistics_(true)
    , showConnectorNames_(false)
    , showAfterPruning_(true)
    , refreshInterval_(1000)
    , lastProcessedSize_(0)
    , skipExistingOnNextParse_(false)
{
    setWindowTitle("Real-Time Connector Histogram");
    setMinimumSize(500, 400);
    resize(700, 500);
    
    try {
        setupUI();
        
        autoRefreshTimer_ = new QTimer(this);
        connect(autoRefreshTimer_, &QTimer::timeout, this, &RealTimeHistogramWindow::onAutoRefreshTimer);
        
        fileWatcher_ = new QFileSystemWatcher(this);
        connect(fileWatcher_, &QFileSystemWatcher::fileChanged, this, &RealTimeHistogramWindow::onFileChanged);
        
        updateThrottleTimer_ = new QTimer(this);
        updateThrottleTimer_->setSingleShot(true);
        updateThrottleTimer_->setInterval(100);
        connect(updateThrottleTimer_, &QTimer::timeout, this, &RealTimeHistogramWindow::performUpdate);
        
        connectorData_.clear();
        preConnectorData_.clear();
        permutationData_.clear();
        prePermutationData_.clear();
        connectorNamesData_.clear();
        lastProcessedSize_ = 0;
        
        QFile resultsFile("real_time_permutations.txt");
        if (resultsFile.exists()) {
            resultsFile.remove();
        }
        
        startMonitoring();
    } catch (...) {
    }
}

RealTimeHistogramWindow::~RealTimeHistogramWindow()
{
    
    stopMonitoring();
    
    if (autoRefreshTimer_) {
        try {
            autoRefreshTimer_->disconnect();
            autoRefreshTimer_->stop();
        } catch (...) {}
    }
    
    if (fileWatcher_) {
        try {
            fileWatcher_->disconnect();
        } catch (...) {}
    }
    
    if (updateThrottleTimer_) {
        try {
            updateThrottleTimer_->stop();
            updateThrottleTimer_->disconnect();
        } catch (...) {}
    }
    
    
    connectorData_.clear();
    preConnectorData_.clear();
    permutationData_.clear();
    prePermutationData_.clear();
    connectorNamesData_.clear();
    
}

void RealTimeHistogramWindow::setupUI()
{
    try {
        setStyleSheet(DarkTheme::Styles::windowStylesheet());
        
        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
        setCentralWidget(scrollArea);
        
        centralWidget_ = new QWidget();
        scrollArea->setWidget(centralWidget_);
        
        mainLayout_ = new QVBoxLayout(centralWidget_);
        mainLayout_->setContentsMargins(8, 8, 8, 8);
        mainLayout_->setSpacing(6);
        
        controlGroup_ = new QGroupBox("Controls");
        controlGroup_->setStyleSheet(DarkTheme::Styles::groupBox());
        controlGroup_->setMaximumHeight(60);
        controlLayout_ = new QHBoxLayout(controlGroup_);
        controlLayout_->setSpacing(8);
        controlLayout_->setContentsMargins(6, 4, 6, 4);
        
        refreshButton_ = new QPushButton("Refresh");
        refreshButton_->setStyleSheet(DarkTheme::Styles::primaryButton());
        refreshButton_->setToolTip("Manually refresh the data");
        connect(refreshButton_, &QPushButton::clicked, this, &RealTimeHistogramWindow::onRefreshClicked);
        
        downloadButton_ = new QPushButton("Export");
        downloadButton_->setStyleSheet(DarkTheme::Styles::secondaryButton());
        downloadButton_->setToolTip("Download current histogram data as CSV file");
        connect(downloadButton_, &QPushButton::clicked, this, &RealTimeHistogramWindow::onDownloadClicked);
        
        resetButton_ = new QPushButton("Reset");
        resetButton_->setStyleSheet(DarkTheme::Styles::dangerButton());
        resetButton_->setToolTip("Clear current histogram and statistics");
        connect(resetButton_, &QPushButton::clicked, this, &RealTimeHistogramWindow::clearData);
        
        autoRefreshCheckBox_ = new QCheckBox("Auto");
        autoRefreshCheckBox_->setChecked(autoRefreshEnabled_);
        autoRefreshCheckBox_->setToolTip("Automatically refresh the data");
        connect(autoRefreshCheckBox_, &QCheckBox::toggled, this, &RealTimeHistogramWindow::onAutoRefreshChanged);
        
        binCountLabel_ = new QLabel("Bars:");
        binCountLabel_->setStyleSheet(QString("font-size: 9pt; color: %1;").arg(DarkTheme::Colors::TEXT_SECONDARY));
        binCountSpinBox_ = new QSpinBox;
        binCountSpinBox_->setRange(5, 100);
        binCountSpinBox_->setValue(binCount_);
        binCountSpinBox_->setFixedWidth(50);
        binCountSpinBox_->setToolTip("Number of histogram bars");
        connect(binCountSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RealTimeHistogramWindow::onBinCountChanged);
        
        showStatisticsCheckBox_ = new QCheckBox("Stats");
        showStatisticsCheckBox_->setChecked(showStatistics_);
        showStatisticsCheckBox_->setToolTip("Show statistical information");
        connect(showStatisticsCheckBox_, &QCheckBox::toggled, this, &RealTimeHistogramWindow::onShowStatisticsChanged);
        
        showConnectorNamesCheckBox_ = new QCheckBox("Names");
        showConnectorNamesCheckBox_->setChecked(showConnectorNames_);
        showConnectorNamesCheckBox_->setToolTip("Show connector names instead of distribution");
        connect(showConnectorNamesCheckBox_, &QCheckBox::toggled, this, &RealTimeHistogramWindow::onShowConnectorNamesChanged);

        showAfterPruningCheckBox_ = new QCheckBox("After pruning");
        showAfterPruningCheckBox_->setChecked(showAfterPruning_);
        showAfterPruningCheckBox_->setToolTip("Toggle between pre-pruning (heuristic) and post-pruning connector counts");
        connect(showAfterPruningCheckBox_, &QCheckBox::toggled, this, &RealTimeHistogramWindow::onShowAfterPruningChanged);

        controlLayout_->addWidget(refreshButton_);
        controlLayout_->addWidget(downloadButton_);
        controlLayout_->addWidget(resetButton_);
        controlLayout_->addSpacing(8);
        controlLayout_->addWidget(autoRefreshCheckBox_);
        controlLayout_->addSpacing(4);
        controlLayout_->addWidget(binCountLabel_);
        controlLayout_->addWidget(binCountSpinBox_);
        controlLayout_->addSpacing(4);
        controlLayout_->addWidget(showStatisticsCheckBox_);
        controlLayout_->addWidget(showConnectorNamesCheckBox_);
        controlLayout_->addWidget(showAfterPruningCheckBox_);
        controlLayout_->addStretch();
        
        mainLayout_->addWidget(controlGroup_);
        
        QSplitter* contentSplitter = new QSplitter(Qt::Vertical, this);
        contentSplitter->setStyleSheet(QString(
            "QSplitter::handle:vertical {"
            "  background: %1;"
            "  height: 6px;"
            "  border-radius: 2px;"
            "}"
            "QSplitter::handle:vertical:hover {"
            "  background: %2;"
            "}"
        ).arg(DarkTheme::Colors::BORDER).arg(DarkTheme::Colors::ACCENT));
        
        histogramGroup_ = new QGroupBox("Real-Time Histogram");
        histogramGroup_->setStyleSheet(DarkTheme::Styles::accentPanel());
        histogramLayout_ = new QVBoxLayout(histogramGroup_);
        histogramLayout_->setContentsMargins(8, 20, 8, 12);
        
        QScrollArea* histogramScrollArea = new QScrollArea;
        histogramScrollArea->setWidgetResizable(true);
        histogramScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        histogramScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        histogramScrollArea->setFrameShape(QFrame::NoFrame);
        histogramScrollArea->setStyleSheet(QString(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollBar:horizontal {"
            "  height: 10px;"
            "  background: %1;"
            "  border-radius: 5px;"
            "  margin: 2px;"
            "}"
            "QScrollBar::handle:horizontal {"
            "  background: %2;"
            "  border-radius: 4px;"
            "  min-width: 30px;"
            "}"
            "QScrollBar::handle:horizontal:hover {"
            "  background: %3;"
            "}"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
            "  width: 0px;"
            "}"
        ).arg(DarkTheme::Colors::BG_TERTIARY)
         .arg(DarkTheme::Colors::BORDER_LIGHT)
         .arg(DarkTheme::Colors::ACCENT));
        
        histogramWidget_ = new VerticalHistogramWidget(this);
        histogramWidget_->setMinimumHeight(250);
        histogramScrollArea->setWidget(histogramWidget_);
        histogramLayout_->addWidget(histogramScrollArea);
        
        contentSplitter->addWidget(histogramGroup_);
        
        statisticsGroup_ = new QGroupBox("Statistics");
        statisticsGroup_->setStyleSheet(DarkTheme::Styles::groupBox());
        statisticsGroup_->setMaximumHeight(85);
        statisticsLayout_ = new QVBoxLayout(statisticsGroup_);
        statisticsLayout_->setContentsMargins(12, 12, 12, 12);
        statisticsLayout_->setSpacing(8);
        
        totalPermutationsLabel_ = new QLabel("n: 0");
        totalPermutationsLabel_->setStyleSheet(DarkTheme::Styles::statLabelAccent() + " font-size: 11pt; padding: 6px 12px;");
        meanLabel_ = new QLabel("μ: 0.0");
        meanLabel_->setStyleSheet(DarkTheme::Styles::statLabel() + " font-size: 10pt; padding: 6px 12px;");
        medianLabel_ = new QLabel("Med: 0.0");
        medianLabel_->setStyleSheet(DarkTheme::Styles::statLabel() + " font-size: 10pt; padding: 6px 12px;");
        stdDevLabel_ = new QLabel("σ: 0.0");
        stdDevLabel_->setStyleSheet(DarkTheme::Styles::statLabel() + " font-size: 10pt; padding: 6px 12px;");
        minLabel_ = new QLabel("Min: 0");
        minLabel_->setStyleSheet(DarkTheme::Styles::statLabelSuccess() + " font-size: 10pt; padding: 6px 12px;");
        maxLabel_ = new QLabel("Max: 0");
        maxLabel_->setStyleSheet(DarkTheme::Styles::statLabelDanger() + " font-size: 10pt; padding: 6px 12px;");
        rangeLabel_ = new QLabel("R: 0");
        rangeLabel_->setStyleSheet(DarkTheme::Styles::statLabel() + " font-size: 10pt; padding: 6px 12px;");
        
        QHBoxLayout* statsRow = new QHBoxLayout;
        statsRow->setSpacing(20);
        statsRow->addWidget(totalPermutationsLabel_);
        statsRow->addWidget(meanLabel_);
        statsRow->addWidget(medianLabel_);
        statsRow->addWidget(stdDevLabel_);
        statsRow->addWidget(minLabel_);
        statsRow->addWidget(maxLabel_);
        statsRow->addWidget(rangeLabel_);
        statsRow->addStretch();
        
        statisticsLayout_->addLayout(statsRow);
        
        dataGroup_ = new QGroupBox("Recent Results (drag handle above to resize)");
        dataGroup_->setStyleSheet(DarkTheme::Styles::groupBox());
        dataLayout_ = new QVBoxLayout(dataGroup_);
        dataLayout_->setContentsMargins(6, 6, 6, 6);
        
        dataTable_ = new QTableWidget;
        dataTable_->setColumnCount(3);
        dataTable_->setHorizontalHeaderLabels({"#", "Connectors", "Time"});
        dataTable_->horizontalHeader()->setStretchLastSection(true);
        dataTable_->setMinimumHeight(60);
        dataTable_->setAlternatingRowColors(true);
        dataTable_->setStyleSheet(DarkTheme::Styles::dataTable());
        dataTable_->verticalHeader()->setVisible(false);
        dataTable_->verticalHeader()->setDefaultSectionSize(24);
        dataTable_->horizontalHeader()->setDefaultSectionSize(80);
        
        dataLayout_->addWidget(dataTable_);
        contentSplitter->addWidget(dataGroup_);
        
        contentSplitter->setSizes({300, 150});
        contentSplitter->setChildrenCollapsible(false);
        
        mainLayout_->addWidget(contentSplitter, 1);
        mainLayout_->addWidget(statisticsGroup_);
        
    } catch (...) {
    }
}

void RealTimeHistogramWindow::setupChart()
{
}

void RealTimeHistogramWindow::addConnectorData(int permutation, int totalPermutations, int connectorsFound)
{
	try {
		connectorData_.push_back(connectorsFound);
		preConnectorData_.push_back(connectorsFound);
		permutationData_.push_back({permutation, connectorsFound});
		prePermutationData_.push_back({permutation, connectorsFound});
		
		if (dataTable_) {
			int row = dataTable_->rowCount();
			dataTable_->insertRow(row);
			dataTable_->setItem(row, 0, new QTableWidgetItem(QString::number(permutation)));
			dataTable_->setItem(row, 1, new QTableWidgetItem(QString::number(connectorsFound)));
			dataTable_->setItem(row, 2, new QTableWidgetItem(QDateTime::currentDateTime().toString("hh:mm:ss")));
			if (dataTable_->rowCount() > 50) {
				dataTable_->removeRow(0);
			}
		}
		
		performUpdate();
		scheduleUpdate();
		
	} catch (...) {
	}
}

void RealTimeHistogramWindow::addConnectorDataWithNames(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames)
{
	if (connectorsFound < 0) return;
	
	connectorData_.push_back(connectorsFound);
	preConnectorData_.push_back(connectorsFound);
	permutationData_.push_back({permutation, connectorsFound});
	prePermutationData_.push_back({permutation, connectorsFound});
	connectorNamesData_.push_back({permutation, connectorNames});
	
	if (dataTable_) {
		int row = dataTable_->rowCount();
		dataTable_->insertRow(row);
		dataTable_->setItem(row, 0, new QTableWidgetItem(QString::number(permutation)));
		
		if (showConnectorNames_ && !connectorNames.empty()) {
			QString namesText = QString::fromStdString(connectorNames[0]);
			for (size_t i = 1; i < connectorNames.size() && i < 3; ++i) {
				namesText += ", " + QString::fromStdString(connectorNames[i]);
			}
			if (connectorNames.size() > 3) {
				namesText += "...";
			}
			dataTable_->setItem(row, 1, new QTableWidgetItem(namesText));
		} else {
			dataTable_->setItem(row, 1, new QTableWidgetItem(QString::number(connectorsFound)));
		}
		
		dataTable_->setItem(row, 2, new QTableWidgetItem(QDateTime::currentDateTime().toString("hh:mm:ss")));
		
		if (dataTable_->rowCount() > 50) {
			dataTable_->removeRow(0);
		}
	}
	
	performUpdate();
	scheduleUpdate();
	
}

void RealTimeHistogramWindow::addConnectorDataWithSeeds(int permutation, int totalPermutations, int connectorsFound, const std::vector<std::string>& connectorNames, const std::vector<std::string>& seedNames, int preConnectorsFound) {
	int effectivePreCount = (preConnectorsFound > 0) ? preConnectorsFound : connectorsFound;

	connectorData_.push_back(connectorsFound);
	preConnectorData_.push_back(effectivePreCount);
	permutationData_.push_back({permutation, connectorsFound});
	prePermutationData_.push_back({permutation, effectivePreCount});
	connectorNamesData_.push_back({permutation, connectorNames});
	seedNamesData_.push_back({permutation, seedNames});
	
	int batchIndex = 0;
	if (permutationsPerBatch_ > 0) {
		batchIndex = (permutation - 1) / permutationsPerBatch_ + 1;
		batchIndexData_.push_back(batchIndex);
		if (!seedNames.empty() && batchSeeds_.find(batchIndex) == batchSeeds_.end()) {
			batchSeeds_[batchIndex] = seedNames;
		}
	} else {
		batchIndexData_.push_back(0);
	}
	
	if (dataTable_) {
		int row = dataTable_->rowCount();
		dataTable_->insertRow(row);
		dataTable_->setItem(row, 0, new QTableWidgetItem(QString::number(permutation)));
		
		if (showConnectorNames_ && !connectorNames.empty()) {
			QString namesText = QString::fromStdString(connectorNames[0]);
			for (size_t j = 1; j < connectorNames.size() && j < 3; ++j) {
				namesText += ", " + QString::fromStdString(connectorNames[j]);
			}
			if (connectorNames.size() > 3) namesText += "...";
			dataTable_->setItem(row, 1, new QTableWidgetItem(namesText));
		} else {
			dataTable_->setItem(row, 1, new QTableWidgetItem(QString::number(connectorsFound)));
		}
		
		dataTable_->setItem(row, 2, new QTableWidgetItem(QDateTime::currentDateTime().toString("hh:mm:ss")));
		if (dataTable_->rowCount() > 50) {
			dataTable_->removeRow(0);
		}
	}
	
	performUpdate();
	scheduleUpdate();
}

void RealTimeHistogramWindow::startMonitoring()
{
    if (fileWatcher_) {
        fileWatcher_->addPath("real_time_permutations.txt");
    }
    
    if (autoRefreshEnabled_ && autoRefreshTimer_) {
        autoRefreshTimer_->start(refreshInterval_);
    }
    
    parseResultsFile();
}

void RealTimeHistogramWindow::stopMonitoring()
{
    
    if (autoRefreshTimer_) {
        autoRefreshTimer_->stop();
    }
    
    if (fileWatcher_) {
        try {
            fileWatcher_->removePath("real_time_permutations.txt");
        } catch (...) {
        }
    }
    
}

void RealTimeHistogramWindow::clearData() {
    
    connectorData_.clear();
    preConnectorData_.clear();
    permutationData_.clear();
    prePermutationData_.clear();
    connectorNamesData_.clear();
    seedNamesData_.clear();
    batchIndexData_.clear();
    batchSeeds_.clear();
    permutationsPerBatch_ = 0;
    
    lastProcessedSize_ = 0;
    skipExistingOnNextParse_ = true;
    
    clearVisualHistogram();
    
    if (dataTable_) {
        dataTable_->setRowCount(0);
    }
    
    updateStatistics();
}

const std::vector<int>& RealTimeHistogramWindow::activeConnectorData() const
{
    if (!showAfterPruning_ && !preConnectorData_.empty()) {
        return preConnectorData_;
    }
    return connectorData_;
}

const std::vector<std::pair<int, int>>& RealTimeHistogramWindow::activePermutationData() const
{
    if (!showAfterPruning_ && !prePermutationData_.empty()) {
        return prePermutationData_;
    }
    return permutationData_;
}

void RealTimeHistogramWindow::updateHistogram()
{
    if (activeConnectorData().empty()) {
        return;
    }
    
    updateVisualHistogram();
    
    updateStatistics();
}

void RealTimeHistogramWindow::updateDisplayWithCounts(QString& displayText)
{
    const auto& data = activeConnectorData();
    if (data.empty()) {
        displayText += "No data available\n";
        return;
    }

    std::vector<int> bins = calculateHistogramBins(data, binCount_);

    int minVal = *std::min_element(data.begin(), data.end());
    int maxVal = *std::max_element(data.begin(), data.end());
    double binWidth = static_cast<double>(maxVal - minVal) / binCount_;

    QString title = showAfterPruning_ ? "Post-pruning" : "Pre-pruning";
    displayText += QString("%1 Connector Count Distribution (Total: %2)\n\n").arg(title).arg(data.size());

    for (int i = 0; i < bins.size(); ++i) {
        int binStart = minVal + static_cast<int>(i * binWidth);
        int binEnd = minVal + static_cast<int>((i + 1) * binWidth);
        QString bar = QString(bins[i], '*');
        displayText += QString("%1-%2: %3 (%4)\n").arg(binStart).arg(binEnd).arg(bar).arg(bins[i]);
    }
}

void RealTimeHistogramWindow::updateDisplayWithNames(QString& displayText)
{
    if (connectorNamesData_.empty()) {
        displayText += "No connector names data available\n";
        return;
    }
    
    displayText += "Connector Names Distribution\n\n";
    
    std::map<std::string, int> nameCounts;
    for (const auto& permutationData : connectorNamesData_) {
        for (const auto& name : permutationData.second) {
            nameCounts[name]++;
        }
    }
    
    std::vector<std::pair<std::string, int>> sortedNames(nameCounts.begin(), nameCounts.end());
    std::sort(sortedNames.begin(), sortedNames.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& [name, count] : sortedNames) {
        QString bar = QString(count, '*');
        displayText += QString("%1: %2 (%3)\n").arg(QString::fromStdString(name)).arg(bar).arg(count);
    }
}

void RealTimeHistogramWindow::onFileChanged(const QString& path)
{
    if (path == "real_time_permutations.txt" && !path.isEmpty()) {
        parseResultsFile();
    }
}

void RealTimeHistogramWindow::parseResultsFile()
{
    QFile file("real_time_permutations.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    
    qint64 currentSize = file.size();
    if (skipExistingOnNextParse_) {
        lastProcessedSize_ = currentSize;
        skipExistingOnNextParse_ = false;
        file.close();
        return;
    }
    if (currentSize == lastProcessedSize_) {
        file.close();
        return;
    }
    
    QTextStream in(&file);
    QString line;
    std::vector<int> newData;
    std::vector<std::pair<int, int>> newPermData;
    std::vector<std::pair<int, std::vector<std::string>>> newConnectorNamesData;
    std::vector<std::pair<int, std::vector<std::string>>> newSeedNamesData;
    std::vector<int> newBatchIndexData;
    std::map<int, std::vector<std::string>> newBatchSeeds;
    int currentBatchIndex = 0;
    
    QRegularExpression permRegex(R"(Permutation\s+(\d+)/(\d+):\s+(\d+)\s+connectors(?:\s+\[(.*?)\])?)");
    QRegularExpression seedsRegex(R"(\[Seeds:\s*(.*?)\])");
    
    try {
        while (!in.atEnd()) {
            line = in.readLine();
            if (line.isNull() || line.isEmpty()) continue;
            
            if (line.startsWith("--- BATCH ")) {
                QRegularExpression batchRe(R"(--- BATCH\s+(\d+)\s+---)");
                auto bmatch = batchRe.match(line);
                if (bmatch.hasMatch()) {
                    currentBatchIndex = bmatch.captured(1).toInt();
                }
                continue;
            }
            
            QRegularExpressionMatch match = permRegex.match(line);
            if (match.hasMatch()) {
                int permutationNum = match.captured(1).toInt();
                int denom = match.captured(2).toInt();
                int connectors = match.captured(3).toInt();
                QString namesPart = match.captured(4);
                
                if (denom > 0 && permutationsPerBatch_ == 0) {
                    permutationsPerBatch_ = denom;
                }
                
                newData.push_back(connectors);
                newPermData.push_back({permutationNum, connectors});
                newBatchIndexData.push_back(currentBatchIndex);
                
                std::vector<std::string> connectorNames;
                if (!namesPart.isEmpty()) {
                    if (namesPart.contains("[Seeds:")) {
                        namesPart = namesPart.left(namesPart.indexOf("[Seeds:"));
                    }
                    if (namesPart.startsWith("Connectors:")) {
                        namesPart = namesPart.mid(QString("Connectors:").length());
                    }
                    namesPart = namesPart.trimmed();
                    QStringList namesList = namesPart.split(", ", Qt::SkipEmptyParts);
                    for (const QString& name : namesList) {
                        QString clean = name.trimmed();
                        if (!clean.isEmpty()) connectorNames.push_back(clean.toStdString());
                    }
                }
                newConnectorNamesData.push_back({permutationNum, connectorNames});
                
                std::vector<std::string> seedNames;
                auto sm = seedsRegex.match(line);
                if (sm.hasMatch()) {
                    QString seedsPart = sm.captured(1).trimmed();
                    QStringList seedsList = seedsPart.split(", ", Qt::SkipEmptyParts);
                    for (const QString& s : seedsList) {
                        QString clean = s.trimmed();
                        if (!clean.isEmpty()) seedNames.push_back(clean.toStdString());
                    }
                    if (currentBatchIndex > 0 && !seedNames.empty() && newBatchSeeds.find(currentBatchIndex) == newBatchSeeds.end()) {
                        newBatchSeeds[currentBatchIndex] = seedNames;
                    }
                }
                newSeedNamesData.push_back({permutationNum, seedNames});
            }
        }
    } catch (...) {
        file.close();
        return;
    }
    
    lastProcessedSize_ = currentSize;
    file.close();
    
    if (newData.size() > connectorData_.size()) {
        connectorData_ = std::move(newData);
        permutationData_ = std::move(newPermData);
        connectorNamesData_ = std::move(newConnectorNamesData);
        seedNamesData_ = std::move(newSeedNamesData);
        batchIndexData_ = std::move(newBatchIndexData);
        batchSeeds_ = std::move(newBatchSeeds);
        preConnectorData_ = connectorData_;
        prePermutationData_ = permutationData_;
        
        updateHistogram();
        updateStatistics();
        
        if (dataTable_) {
            dataTable_->setRowCount(0);
            int startIdx = std::max(0, static_cast<int>(permutationData_.size()) - 50);
            for (int i = startIdx; i < permutationData_.size(); ++i) {
                int row = dataTable_->rowCount();
                dataTable_->insertRow(row);
                dataTable_->setItem(row, 0, new QTableWidgetItem(QString::number(permutationData_[i].first)));
                
                if (showConnectorNames_ && i < connectorNamesData_.size() && !connectorNamesData_[i].second.empty()) {
                    const auto& names = connectorNamesData_[i].second;
                    QString namesText = QString::fromStdString(names[0]);
                    for (size_t j = 1; j < names.size() && j < 3; ++j) {
                        namesText += ", " + QString::fromStdString(names[j]);
                    }
                    if (names.size() > 3) namesText += "...";
                    dataTable_->setItem(row, 1, new QTableWidgetItem(namesText));
                } else {
                    dataTable_->setItem(row, 1, new QTableWidgetItem(QString::number(permutationData_[i].second)));
                }
                
                dataTable_->setItem(row, 2, new QTableWidgetItem("From file"));
            }
        }
    }
}

std::vector<int> RealTimeHistogramWindow::calculateHistogramBins(const std::vector<int>& data, int numBins)
{
    if (data.empty() || numBins <= 0) return std::vector<int>(numBins, 0);
    
    std::vector<int> bins(numBins, 0);
    int minVal = *std::min_element(data.begin(), data.end());
    int maxVal = *std::max_element(data.begin(), data.end());
    
    if (minVal == maxVal) {
        bins[0] = data.size();
        return bins;
    }
    
    double binWidth = static_cast<double>(maxVal - minVal) / numBins;
    
    for (int value : data) {
        int binIndex = static_cast<int>((value - minVal) / binWidth);
        if (binIndex >= numBins) binIndex = numBins - 1;
        bins[binIndex]++;
    }
    
    return bins;
}

void RealTimeHistogramWindow::updateStatistics()
{
    if (!totalPermutationsLabel_ || !meanLabel_ || !medianLabel_ || !stdDevLabel_ || !minLabel_ || !maxLabel_ || !rangeLabel_) {
        return;
    }
    
    const auto& data = activeConnectorData();
    if (data.empty()) {
        totalPermutationsLabel_->setText("Total Permutations: 0");
        meanLabel_->setText("Mean: 0.0");
        medianLabel_->setText("Median: 0.0");
        stdDevLabel_->setText("Std Dev: 0.0");
        minLabel_->setText("Min: 0");
        maxLabel_->setText("Max: 0");
        rangeLabel_->setText("Range: 0");
        return;
    }
    
    try {
        int total = static_cast<int>(data.size());
        double sum = std::accumulate(data.begin(), data.end(), 0.0);
        double mean = sum / total;

        std::vector<int> sortedData = data;
        std::sort(sortedData.begin(), sortedData.end());
        double median;
        if (total % 2 == 0) {
            median = (sortedData[total/2 - 1] + sortedData[total/2]) / 2.0;
        } else {
            median = sortedData[total/2];
        }

        double variance = 0.0;
        for (int value : data) {
            variance += std::pow(value - mean, 2);
        }
        variance /= total;
        double stdDev = std::sqrt(variance);

        int minVal = *std::min_element(data.begin(), data.end());
        int maxVal = *std::max_element(data.begin(), data.end());
        int range = maxVal - minVal;
        
        totalPermutationsLabel_->setText(QString("Total Permutations: %1").arg(total));
        meanLabel_->setText(QString("Mean: %1").arg(mean, 0, 'f', 2));
        medianLabel_->setText(QString("Median: %1").arg(median, 0, 'f', 2));
        stdDevLabel_->setText(QString("Std Dev: %1").arg(stdDev, 0, 'f', 2));
        minLabel_->setText(QString("Min: %1").arg(minVal));
        maxLabel_->setText(QString("Max: %1").arg(maxVal));
        rangeLabel_->setText(QString("Range: %1").arg(range));
    } catch (...) {
        return;
    }
}

void RealTimeHistogramWindow::onAutoRefreshTimer()
{
    parseResultsFile();
}

void RealTimeHistogramWindow::onRefreshClicked()
{
    parseResultsFile();
}

void RealTimeHistogramWindow::onAutoRefreshChanged(bool checked)
{
    autoRefreshEnabled_ = checked;
    if (autoRefreshTimer_) {
        if (checked) {
            autoRefreshTimer_->start(refreshInterval_);
        } else {
            autoRefreshTimer_->stop();
        }
    }
}

void RealTimeHistogramWindow::onBinCountChanged(int value)
{
    binCount_ = value;
    createVisualHistogram();
    updateVisualHistogram();
}

void RealTimeHistogramWindow::onShowStatisticsChanged(bool checked)
{
    showStatistics_ = checked;
    statisticsGroup_->setVisible(checked);
}

void RealTimeHistogramWindow::onShowConnectorNamesChanged(bool checked)
{
    
    showConnectorNames_ = checked;
    
    if (dataTable_) {
        dataTable_->setRowCount(0);
        int startIdx = std::max(0, static_cast<int>(permutationData_.size()) - 50);
        for (int i = startIdx; i < permutationData_.size(); ++i) {
            int row = dataTable_->rowCount();
            dataTable_->insertRow(row);
            dataTable_->setItem(row, 0, new QTableWidgetItem(QString::number(permutationData_[i].first)));
            
            if (checked && i < connectorNamesData_.size() && !connectorNamesData_[i].second.empty()) {
                const auto& names = connectorNamesData_[i].second;
                QString namesText = QString::fromStdString(names[0]);
                for (size_t j = 1; j < names.size() && j < 3; ++j) {
                    namesText += ", " + QString::fromStdString(names[j]);
                }
                if (names.size() > 3) {
                    namesText += "...";
                }
                dataTable_->setItem(row, 1, new QTableWidgetItem(namesText));
            } else {
                dataTable_->setItem(row, 1, new QTableWidgetItem(QString::number(permutationData_[i].second)));
            }
            
            dataTable_->setItem(row, 2, new QTableWidgetItem("Updated"));
        }
    }
    
    if (histogramWidget_) {
        histogramWidget_->setUseAltColor(checked);
    }
    updateVisualHistogram();
    
}

void RealTimeHistogramWindow::onShowAfterPruningChanged(bool checked)
{
    showAfterPruning_ = checked;

    if (dataTable_) {
        const auto& permData = activePermutationData();
        dataTable_->setRowCount(0);
        int startIdx = std::max(0, static_cast<int>(permData.size()) - 50);
        for (int i = startIdx; i < static_cast<int>(permData.size()); ++i) {
            int row = dataTable_->rowCount();
            dataTable_->insertRow(row);
            dataTable_->setItem(row, 0, new QTableWidgetItem(QString::number(permData[i].first)));

            if (showConnectorNames_ && i < static_cast<int>(connectorNamesData_.size()) && !connectorNamesData_[i].second.empty()) {
                const auto& names = connectorNamesData_[i].second;
                QString namesText = QString::fromStdString(names[0]);
                for (size_t j = 1; j < names.size() && j < 3; ++j) {
                    namesText += ", " + QString::fromStdString(names[j]);
                }
                if (names.size() > 3) {
                    namesText += "...";
                }
                dataTable_->setItem(row, 1, new QTableWidgetItem(namesText));
            } else {
                dataTable_->setItem(row, 1, new QTableWidgetItem(QString::number(permData[i].second)));
            }
            dataTable_->setItem(row, 2, new QTableWidgetItem("Updated"));
        }
    }

    updateVisualHistogram();
    updateStatistics();
}

void RealTimeHistogramWindow::closeEvent(QCloseEvent* event)
{
    
    stopMonitoring();
    
    if (autoRefreshTimer_) {
        autoRefreshTimer_->disconnect();
        autoRefreshTimer_->stop();
    }
    
    if (fileWatcher_) {
        fileWatcher_->disconnect();
    }
    
    emit windowClosed();
    
    event->accept();
    
} 

void RealTimeHistogramWindow::updateHistogramWithCounts()
{
}

void RealTimeHistogramWindow::updateHistogramWithNames()
{
} 

void RealTimeHistogramWindow::createVisualHistogram()
{
}

void RealTimeHistogramWindow::updateVisualHistogram()
{
    if (!histogramWidget_) return;
    
    if (showConnectorNames_) {
        updateVisualHistogramWithNames();
    } else {
        updateVisualHistogramWithCounts();
    }
}

void RealTimeHistogramWindow::updateVisualHistogramWithCounts()
{
    const auto& data = activeConnectorData();
    if (data.empty() || !histogramWidget_) {
        return;
    }

    std::vector<int> bins = calculateHistogramBins(data, binCount_);

    int minVal = *std::min_element(data.begin(), data.end());
    int maxVal = *std::max_element(data.begin(), data.end());
    double binWidth = (maxVal == minVal) ? 1.0 : static_cast<double>(maxVal - minVal) / binCount_;

    QStringList labels;
    for (int i = 0; i < binCount_; ++i) {
        int binStart = minVal + static_cast<int>(i * binWidth);
        int binEnd = minVal + static_cast<int>((i + 1) * binWidth);
        if (i == binCount_ - 1) binEnd = maxVal;
        labels << QString("%1-%2").arg(binStart).arg(binEnd);
    }

    QString stageLabel = showAfterPruning_ ? "Post-pruning" : "Pre-pruning";
    histogramWidget_->setTitle(QString("%1 Connector Distribution (n=%2)").arg(stageLabel).arg(data.size()));
    histogramWidget_->setUseAltColor(false);
    histogramWidget_->setData(bins, labels);
}

void RealTimeHistogramWindow::updateVisualHistogramWithNames()
{
    if (connectorNamesData_.empty() || !histogramWidget_) {
        return;
    }
    
    std::map<std::string, int> nameFrequency;
    for (const auto& data : connectorNamesData_) {
        for (const std::string& name : data.second) {
            nameFrequency[name]++;
        }
    }
    
    if (nameFrequency.empty()) {
        return;
    }
    
    std::vector<std::pair<std::string, int>> sortedNames;
    for (const auto& pair : nameFrequency) {
        sortedNames.push_back(pair);
    }
    std::sort(sortedNames.begin(), sortedNames.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    int numBars = std::min(binCount_, static_cast<int>(sortedNames.size()));
    
    std::vector<int> values;
    QStringList labels;
    for (int i = 0; i < numBars; ++i) {
        values.push_back(sortedNames[i].second);
        QString displayName = QString::fromStdString(sortedNames[i].first);
        if (displayName.length() > 10) {
            displayName = displayName.left(8) + "..";
        }
        labels << displayName;
    }
    
    histogramWidget_->setTitle(QString("Top %1 Connectors").arg(numBars));
    histogramWidget_->setUseAltColor(true);
    histogramWidget_->setData(values, labels);
}

void RealTimeHistogramWindow::clearVisualHistogram()
{
    if (histogramWidget_) {
        histogramWidget_->clear();
    }
} 

void RealTimeHistogramWindow::scheduleUpdate()
{
    if (!updatePending_) {
        updatePending_ = true;
        if (updateThrottleTimer_) {
            updateThrottleTimer_->start();
        }
    }
}

void RealTimeHistogramWindow::performUpdate()
{
    updatePending_ = false;
    updateVisualHistogram();
    updateStatistics();
}

QString RealTimeHistogramWindow::getCurrentDataForExport() const
{
    QString exportData;
    QTextStream stream(&exportData);
    
    stream << "Real-Time Connector Histogram Data Export\n";
    stream << "Generated: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n\n";
    
    if (!connectorData_.empty()) {
        int total = static_cast<int>(connectorData_.size());
        double sum = std::accumulate(connectorData_.begin(), connectorData_.end(), 0.0);
        double mean = sum / total;
        
        std::vector<int> sortedData = connectorData_;
        std::sort(sortedData.begin(), sortedData.end());
        double median = (total % 2 == 0) ? 
            (sortedData[total/2 - 1] + sortedData[total/2]) / 2.0 : 
            sortedData[total/2];
        
        double variance = 0.0;
        for (int value : connectorData_) {
            variance += std::pow(value - mean, 2);
        }
        double stdDev = std::sqrt(variance / total);
        
        int minVal = *std::min_element(connectorData_.begin(), connectorData_.end());
        int maxVal = *std::max_element(connectorData_.begin(), connectorData_.end());
        int range = maxVal - minVal;
        
        stream << "STATISTICS SUMMARY:\n";
        stream << "Total Permutations: " << total << "\n";
        stream << "Mean: " << QString::number(mean, 'f', 2) << "\n";
        stream << "Median: " << QString::number(median, 'f', 2) << "\n";
        stream << "Standard Deviation: " << QString::number(stdDev, 'f', 2) << "\n";
        stream << "Minimum: " << minVal << "\n";
        stream << "Maximum: " << maxVal << "\n";
        stream << "Range: " << range << "\n\n";
    }
    
    stream << "INDIVIDUAL PERMUTATION DATA:\n";
    stream << "Permutation,Connectors_Found";
    
    if (permutationsPerBatch_ > 0) {
        stream << ",Batch_Index";
    }
    
    if (!connectorNamesData_.empty()) {
        stream << ",Connector_Names";
    }
    
    if (!seedNamesData_.empty()) {
        stream << ",Seed_Names";
    }
    stream << "\n";
    
    for (size_t i = 0; i < permutationData_.size(); ++i) {
        stream << permutationData_[i].first << "," << permutationData_[i].second;
        
        if (permutationsPerBatch_ > 0 && i < batchIndexData_.size()) {
            stream << "," << batchIndexData_[i];
        }
        
        if (i < connectorNamesData_.size() && !connectorNamesData_[i].second.empty()) {
            QString names = QString::fromStdString(connectorNamesData_[i].second[0]);
            for (size_t j = 1; j < connectorNamesData_[i].second.size(); ++j) {
                names += ";" + QString::fromStdString(connectorNamesData_[i].second[j]);
            }
            stream << ",\"" << names << "\"";
        } else {
            stream << ",";
        }
        
        if (i < seedNamesData_.size() && !seedNamesData_[i].second.empty()) {
            QString seeds = QString::fromStdString(seedNamesData_[i].second[0]);
            for (size_t j = 1; j < seedNamesData_[i].second.size(); ++j) {
                seeds += ";" + QString::fromStdString(seedNamesData_[i].second[j]);
            }
            stream << ",\"" << seeds << "\"";
        } else {
            stream << ",";
        }
        stream << "\n";
    }
    
    if (!connectorNamesData_.empty()) {
        stream << "\nCONNECTOR FREQUENCY ANALYSIS:\n";
        stream << "Connector_Name,Frequency,Percentage\n";
        
        std::map<std::string, int> nameFrequency;
        int totalConnectors = 0;
        
        for (const auto& permutationData : connectorNamesData_) {
            for (const auto& name : permutationData.second) {
                nameFrequency[name]++;
                totalConnectors++;
            }
        }
        
        std::vector<std::pair<std::string, int>> sortedNames;
        for (const auto& pair : nameFrequency) {
            sortedNames.push_back(pair);
        }
        std::sort(sortedNames.begin(), sortedNames.end(), 
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (const auto& pair : sortedNames) {
            double percentage = (totalConnectors > 0) ? (pair.second * 100.0) / totalConnectors : 0.0;
            stream << "\"" << QString::fromStdString(pair.first) << "\","
                   << pair.second << ","
                   << QString::number(percentage, 'f', 2) << "%\n";
        }
    }
    
    if (permutationsPerBatch_ > 0 && !batchSeeds_.empty()) {
        stream << "\nBATCH SEEDS:\n";
        stream << "Batch_Index,Seed_Names\n";
        for (const auto& kv : batchSeeds_) {
            const int bidx = kv.first;
            const auto& seeds = kv.second;
            QString seedsStr;
            if (!seeds.empty()) {
                seedsStr = QString::fromStdString(seeds[0]);
                for (size_t j = 1; j < seeds.size(); ++j) {
                    seedsStr += ";" + QString::fromStdString(seeds[j]);
                }
            }
            stream << bidx << ",\"" << seedsStr << "\"\n";
        }
    }
    
    return exportData;
}

void RealTimeHistogramWindow::onDownloadClicked()
{
    if (connectorData_.empty()) {
        QMessageBox::information(this, "No Data", "No histogram data available for download.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Histogram Data",
        QString("histogram_data_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV Files (*.csv);;Text Files (*.txt);;All Files (*)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QString exportData = getCurrentDataForExport();
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << exportData;
        file.close();
        
        QMessageBox::information(this, "Download Complete", 
            QString("Histogram data successfully saved to:\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "Error", 
            QString("Failed to save file:\n%1").arg(file.errorString()));
    }
} 