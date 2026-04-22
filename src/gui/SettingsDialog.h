#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QColorDialog>
#include <QLabel>
#include "GraphVisualization.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(GraphVisualization* graphViz, QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void onNodeColorChanged();
    void onSeedColorChanged();
    void onConnectorColorChanged();
    void onExtraConnectorColorChanged();
    void onNodeSizeChanged(double value);
    void onNodeBorderColorChanged();
    void onNodeBorderWidthChanged(double value);
    void onShowLabelsToggled(bool show);
    void onFontSizeChanged(int size);
    void onEdgeColorChanged();
    void onEdgeWidthChanged(double value);
    void resetToDefaults();
    void applySettings();
    void accept() override;

private:
    void setupUI();
    void setupAppearanceTab();
    void setupGeneralTab();
    void loadCurrentSettings();
    void updateColorButtonStyle(QPushButton* button, const QColor& color);

    GraphVisualization* graphVisualization_;
    
    QTabWidget* tabWidget_;
    QWidget* appearanceTab_;
    QWidget* generalTab_;
    
    QPushButton* nodeColorButton_;
    QPushButton* seedColorButton_;
    QPushButton* connectorColorButton_;
    QPushButton* extraConnectorColorButton_;
    QDoubleSpinBox* nodeSizeSpinBox_;
    QPushButton* nodeBorderColorButton_;
    QDoubleSpinBox* nodeBorderWidthSpinBox_;
    QCheckBox* showLabelsCheckBox_;
    QSpinBox* fontSizeSpinBox_;
    
    QPushButton* edgeColorButton_;
    QDoubleSpinBox* edgeWidthSpinBox_;
    
    QPushButton* resetButton_;
    QPushButton* applyButton_;
    QPushButton* okButton_;
    QPushButton* cancelButton_;
    
    QColor currentNodeColor_;
    QColor currentSeedColor_;
    QColor currentConnectorColor_;
    QColor currentExtraConnectorColor_;
    QColor currentNodeBorderColor_;
    QColor currentEdgeColor_;
};

#endif