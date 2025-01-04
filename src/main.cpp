#include <QApplication>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;

    // Get initial directory
    QString initialPath;
    if (argc > 1) {
        // Use the first argument as path
        initialPath = QString::fromLocal8Bit(argv[1]);
        
        // Handle relative paths
        if (initialPath == ".") {
            initialPath = QDir::currentPath();
        } else if (initialPath == "..") {
            initialPath = QDir::currentPath();
            initialPath = QFileInfo(initialPath).dir().absolutePath();
        } else {
            QFileInfo fileInfo(initialPath);
            if (fileInfo.exists()) {
                initialPath = fileInfo.absoluteFilePath();
            } else {
                initialPath = QDir::homePath();
            }
        }
    } else {
        // No arguments, use home directory
        initialPath = QDir::homePath();
    }

    // Set the initial directory
    window.setInitialDirectory(initialPath);
    window.show();

    return app.exec();
} 