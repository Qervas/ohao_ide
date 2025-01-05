#pragma once
#include <QWidget>
#include <QWebEngineView>
#include <QStackedWidget>
#include <QPushButton>
#include <QTabWidget>
#include "filepreview.h"

class ContentView : public QWidget {
    Q_OBJECT
public:
    explicit ContentView(QWidget *parent = nullptr);
    void loadFile(const QString &filePath);
    void loadWebContent(const QString &url);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    enum class ViewMode {
        Empty,
        Preview,
        Web
    };

    QTabWidget *tabs;
    QString currentPath;
    QString currentTitle;

    void setupUI();
    bool isWebContent(const QString &path) const;
    void updateTheme();
    void closeTab(int index);
    QWidget* findTabByPath(const QString &path);
};
