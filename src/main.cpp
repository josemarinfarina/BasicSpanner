/**
 * @file main.cpp
 * @brief Main entry point for the BasicSpanner application.
 *
 * Initializes the Qt application, applies the stylesheet and launches the
 * main window.
 */

#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QFile>
#include <QMetaType>
#include <iostream>
#include <memory>
#include <typeinfo>

#include "gui/MainWindow.h"
#include "core/BasicNetworkFinder.h"

/**
 * @brief Application entry point.
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line argument strings.
 * @return Exit status code.
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<std::shared_ptr<BatchAnalysisResult>>(
        "std::shared_ptr<BatchAnalysisResult>");

    app.setApplicationName("BasicSpanner");
    app.setApplicationVersion("1.0.0");
    app.setApplicationDisplayName("BasicSpanner");
    app.setOrganizationName("BasicSpanner");

    app.setStyle(QStyleFactory::create("Fusion"));

    QFile styleFile("styles/style.qss");
    if (!styleFile.exists()) {
        styleFile.setFileName("../styles/style.qss");
    }

    if (styleFile.open(QFile::ReadOnly)) {
        const QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
    }
    styleFile.close();

    try {
        std::unique_ptr<MainWindow> window = std::make_unique<MainWindow>();
        window->show();
        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "Fatal exception: " << typeid(e).name()
                  << " - " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal unknown exception" << std::endl;
        return 2;
    }
}
