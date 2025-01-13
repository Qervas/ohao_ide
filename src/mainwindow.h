#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QInputDialog>
#include <QMenu>
#include "projecttree.h"
#include "codeeditor.h"
#include "contentview.h"
#include "terminal.h"
#include "dockmanager.h"
#include "welcomeview.h"
#include "browserview.h"
#include "terminalwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void setInitialDirectory(const QString &path);
    void loadFile(const QString &filePath);

public slots:
    void handleFocusChange(QWidget *old, QWidget *now);
    void focusEditor();
    void focusProjectTree();
    void focusTerminal();
    void focusContentView();
    void handleCtrlW();
    void handleCtrlN();
    void showShortcutsHelp();

private slots:
    void createNewFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    bool saveFile(const QString &filePath);
    void closeTab(int index);
    void openFolder();
    void handleFileSelected(const QString &filePath);
    void handleDirectoryChanged(const QString &path);
    void handleRootDirectoryChanged(const QString &path);
    void handleLayoutChanged();
    void handleDockVisibilityChanged(DockManager::DockWidgetType type, bool visible);
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void about();
    void resetLayout();
    void saveLayout();
    void loadLayout();
    void updateRecentProjectsMenu();
    void showPreferences();
    void closeFolder();

protected:
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void setupUI();
    void createMenus();
    void createStatusBar();
    void createDockWidgets();
    void createActions();
    void loadSettings();
    void saveSettings();
    CodeEditor* currentEditor();
    QString currentFilePath();
    void updateWindowTitle();
    bool maybeSave();
    void updateViewMenu();
    void applyEditorSettings();
    void saveSessionState();
    void loadSessionState();


    ProjectTree *projectTree;
    QTabWidget *editorTabs;
    ContentView *contentView;
    WelcomeView *welcomeView;
    Terminal *terminal;
    DockManager *dockManager;
    QString projectPath;
    QStringList recentProjects;
    QMenu *recentProjectsMenu;

    // Actions for view menu
    QMap<DockManager::DockWidgetType, QAction*> viewActions;

    QWidget* currentFocusWidget;
    void setupGlobalShortcuts();
    void setupFocusTracking();
};
