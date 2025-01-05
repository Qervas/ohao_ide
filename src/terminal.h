#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QSplitter>
#include "terminalwidget.h"

class Terminal : public QWidget {
    Q_OBJECT

public:
    explicit Terminal(QWidget *parent = nullptr);
    void setWorkingDirectory(const QString &path);

private slots:
    void addNewTab();
    void closeTab(int index);
    void splitHorizontally();
    void splitVertically();
    void closeCurrentSplit();

private:
    QTabWidget *tabWidget;
    QList<QSplitter*> splitters;
    QString currentWorkingDirectory;

    void setupUI();
    void createToolBar();
    TerminalWidget* createTerminal();
    QSplitter* getCurrentSplitter();
    TerminalWidget* getCurrentTerminal();
};
