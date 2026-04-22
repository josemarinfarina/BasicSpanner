#ifndef DARKTHEME_H
#define DARKTHEME_H

#include <QString>
#include <QColor>

namespace DarkTheme {

namespace Colors {
    constexpr const char* BG_PRIMARY     = "#0d1117";
    constexpr const char* BG_SECONDARY   = "#161b22";
    constexpr const char* BG_TERTIARY    = "#1a1d23";
    constexpr const char* BG_ELEVATED    = "#21262d";
    
    constexpr const char* BORDER         = "#2d3748";
    constexpr const char* BORDER_LIGHT   = "#3d4452";
    constexpr const char* BORDER_FOCUS   = "#00d4aa";
    
    constexpr const char* TEXT_PRIMARY   = "#e2e8f0";
    constexpr const char* TEXT_SECONDARY = "#a0aec0";
    constexpr const char* TEXT_MUTED     = "#718096";
    constexpr const char* TEXT_DARK      = "#0d1117";
    
    constexpr const char* ACCENT         = "#00d4aa";
    constexpr const char* ACCENT_HOVER   = "#00e6b8";
    constexpr const char* ACCENT_PRESSED = "#00b894";
    
    constexpr const char* DANGER         = "#ff6b6b";
    constexpr const char* DANGER_HOVER   = "#ff8787";
    constexpr const char* SUCCESS        = "#48bb78";
    constexpr const char* WARNING        = "#f6ad55";
    constexpr const char* INFO           = "#63b3ed";
    
    constexpr const char* CHART_PRIMARY  = "#00d4aa";
    constexpr const char* CHART_SECONDARY= "#ff6b6b";
    constexpr const char* CHART_GRID     = "#2d3748";
}

inline QColor bgPrimary()     { return QColor(Colors::BG_PRIMARY); }
inline QColor bgSecondary()   { return QColor(Colors::BG_SECONDARY); }
inline QColor bgTertiary()    { return QColor(Colors::BG_TERTIARY); }
inline QColor bgElevated()    { return QColor(Colors::BG_ELEVATED); }
inline QColor border()        { return QColor(Colors::BORDER); }
inline QColor borderLight()   { return QColor(Colors::BORDER_LIGHT); }
inline QColor textPrimary()   { return QColor(Colors::TEXT_PRIMARY); }
inline QColor textSecondary() { return QColor(Colors::TEXT_SECONDARY); }
inline QColor textMuted()     { return QColor(Colors::TEXT_MUTED); }
inline QColor accent()        { return QColor(Colors::ACCENT); }
inline QColor accentHover()   { return QColor(Colors::ACCENT_HOVER); }
inline QColor danger()        { return QColor(Colors::DANGER); }
inline QColor success()       { return QColor(Colors::SUCCESS); }

namespace Styles {

inline QString primaryButton() {
    return QString(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 %1, stop:1 %2);"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 4px 10px;"
        "    font-weight: 600;"
        "    font-size: 9pt;"
        "    color: %3;"
        "    min-height: 20px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 %4, stop:1 %1);"
        "}"
        "QPushButton:pressed {"
        "    background: %2;"
        "}"
    ).arg(Colors::ACCENT, Colors::ACCENT_PRESSED, Colors::TEXT_DARK, Colors::ACCENT_HOVER);
}

inline QString secondaryButton() {
    return QString(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 %1, stop:1 %2);"
        "    border: 1px solid %3;"
        "    border-radius: 4px;"
        "    padding: 4px 10px;"
        "    font-weight: 500;"
        "    font-size: 9pt;"
        "    color: %4;"
        "    min-height: 20px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 %3, stop:1 %1);"
        "    border-color: %5;"
        "}"
        "QPushButton:pressed {"
        "    background: %2;"
        "}"
    ).arg(Colors::BG_ELEVATED, Colors::BG_TERTIARY, Colors::BORDER, 
          Colors::TEXT_PRIMARY, Colors::BORDER_LIGHT);
}

inline QString dangerButton() {
    return QString(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 %1, stop:1 #ee5a5a);"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 4px 10px;"
        "    font-weight: 600;"
        "    font-size: 9pt;"
        "    color: %2;"
        "    min-height: 20px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 %3, stop:1 %1);"
        "}"
        "QPushButton:pressed {"
        "    background: #ee5a5a;"
        "}"
    ).arg(Colors::DANGER, Colors::TEXT_DARK, Colors::DANGER_HOVER);
}

inline QString groupBox() {
    return QString(
        "QGroupBox {"
        "    font-weight: bold;"
        "    font-size: 9pt;"
        "    border: 1px solid %1;"
        "    border-radius: 6px;"
        "    margin-top: 8px;"
        "    padding: 6px;"
        "    padding-top: 12px;"
        "    background-color: %2;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 6px;"
        "    color: %3;"
        "}"
    ).arg(Colors::BORDER, Colors::BG_SECONDARY, Colors::TEXT_SECONDARY);
}

inline QString accentPanel() {
    return QString(
        "QGroupBox {"
        "    font-weight: bold;"
        "    font-size: 9pt;"
        "    border: 1px solid %1;"
        "    border-radius: 6px;"
        "    margin-top: 8px;"
        "    padding-top: 6px;"
        "    background-color: %2;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 6px;"
        "    color: %3;"
        "}"
    ).arg(Colors::BORDER, Colors::BG_SECONDARY, Colors::TEXT_SECONDARY);
}

inline QString statLabel() {
    return QString(
        "QLabel {"
        "    background: %1;"
        "    border: 1px solid %2;"
        "    border-radius: 3px;"
        "    padding: 4px 8px;"
        "    font-size: 9pt;"
        "    color: %3;"
        "}"
    ).arg(Colors::BG_TERTIARY, Colors::BORDER, Colors::TEXT_PRIMARY);
}

inline QString statLabelAccent() {
    return statLabel() + QString("QLabel { font-weight: bold; color: %1; }").arg(Colors::ACCENT);
}

inline QString statLabelSuccess() {
    return statLabel() + QString("QLabel { color: %1; }").arg(Colors::SUCCESS);
}

inline QString statLabelDanger() {
    return statLabel() + QString("QLabel { color: %1; }").arg(Colors::DANGER);
}

inline QString dataTable() {
    return QString(
        "QTableWidget {"
        "    border: 1px solid %1;"
        "    border-radius: 4px;"
        "    gridline-color: %1;"
        "    background-color: %2;"
        "    alternate-background-color: %3;"
        "    color: %4;"
        "    font-size: 9pt;"
        "}"
        "QTableWidget::item {"
        "    padding: 2px 4px;"
        "    border: none;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: %5;"
        "    color: %6;"
        "}"
        "QHeaderView::section {"
        "    background: %7;"
        "    border: none;"
        "    border-bottom: 2px solid %5;"
        "    padding: 4px 6px;"
        "    font-weight: bold;"
        "    font-size: 9pt;"
        "    color: %4;"
        "}"
    ).arg(Colors::BORDER, Colors::BG_PRIMARY, Colors::BG_SECONDARY,
          Colors::TEXT_PRIMARY, Colors::ACCENT, Colors::TEXT_DARK, Colors::BG_TERTIARY);
}

inline QString inputField() {
    return QString(
        "background: %1;"
        "border: 1px solid %2;"
        "border-radius: 6px;"
        "padding: 6px 10px;"
        "color: %3;"
    ).arg(Colors::BG_TERTIARY, Colors::BORDER, Colors::TEXT_PRIMARY);
}

inline QString windowStylesheet() {
    return QString(
        "QMainWindow { background-color: %1; }"
        "QWidget { background-color: %1; color: %2; font-size: 9pt; }"
        "QCheckBox { color: %3; spacing: 4px; font-size: 9pt; }"
        "QCheckBox::indicator { width: 14px; height: 14px; border-radius: 3px; "
        "    border: 1px solid %4; background: %5; }"
        "QCheckBox::indicator:checked { background: %6; border-color: %6; }"
        "QCheckBox::indicator:hover { border-color: %6; }"
        "QSpinBox { background: %5; border: 1px solid %4; border-radius: 3px; "
        "    padding: 2px 4px; color: %2; font-size: 9pt; min-height: 18px; }"
        "QSpinBox:focus { border-color: %6; }"
        "QLabel { font-size: 9pt; }"
    ).arg(Colors::BG_PRIMARY, Colors::TEXT_PRIMARY, Colors::TEXT_SECONDARY,
          Colors::BORDER, Colors::BG_TERTIARY, Colors::ACCENT);
}

}
}

#endif
