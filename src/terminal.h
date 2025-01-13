#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QSplitter>
#include "terminalwidget.h"
#include "dockwidgetbase.h"

class Terminal : public DockWidgetBase {
    Q_OBJECT

public:
    explicit Terminal(QWidget *parent = nullptr);
    
    // Override base class methods
    void setWorkingDirectory(const QString &path) override;
    bool canClose() override { return true; }
    void updateTheme() override;
    void focusWidget() override;

    // Terminal specific methods
    void createNewTerminalTab();
    void setIntelligentIndent(bool enabled);

private slots:
    void addNewTab();
    void closeTab(int index);
    void splitHorizontally();
    void splitVertically();
    void closeCurrentSplit();

private:
    QTabWidget *tabWidget;
    QList<QSplitter*> splitters;
    bool m_intelligentIndent;

    void setupUI();
    void createToolBar();
    TerminalWidget* createTerminal();
    QSplitter* getCurrentSplitter();
    TerminalWidget* getCurrentTerminal();
};
