#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application information for QSettings
    QApplication::setOrganizationName("ohao");
    QApplication::setApplicationName("ohao_IDE");
    
    MainWindow window;
    window.show();
    
    // If command line argument is provided, open that folder
    if (argc > 1) {
        QString path = QString::fromLocal8Bit(argv[1]);
        QFileInfo fileInfo(path);
        if (fileInfo.exists()) {
            if (fileInfo.isDir()) {
                window.setInitialDirectory(path);
            } else if (fileInfo.isFile()) {
                window.loadFile(path);
            }
        }
    }
    
    return app.exec();
} 