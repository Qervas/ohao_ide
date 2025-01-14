#pragma once
#include "codeeditor/codeeditor.h"
#include "views/content/contentview.h"
#include "views/dockmanager.h"
#include "views/dockwidgetbase.h"
#include "views/project/projecttree.h"
#include "views/terminal/terminalwidget.h"
#include "views/welcome/welcomeview.h"
#include <QInputDialog>
#include <QMainWindow>
#include <QMenu>
#include <QTabWidget>

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
  void handleDockVisibilityChanged(DockManager::DockWidgetType type,
                                   bool visible);
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
  CodeEditor *currentEditor();
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
  QMap<DockManager::DockWidgetType, QAction *> viewActions;

  QWidget *currentFocusWidget;
  void setupGlobalShortcuts();
  void setupFocusTracking();
};
