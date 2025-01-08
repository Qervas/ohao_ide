#include "mainwindow.h"
#include "preferencesdialog.h"
#include "sessionsettings.h"
#include <QApplication>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScreen>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  // Create components
  projectTree = new ProjectTree(this);
  editorTabs = new QTabWidget(this);
  contentView = new ContentView(this);
  terminal = new Terminal(this);
  welcomeView = new WelcomeView(this);
  dockManager = new DockManager(this);

  // Connect welcome view signals
  connect(welcomeView, &WelcomeView::openFolder, this, &MainWindow::openFolder);
  connect(welcomeView, &WelcomeView::openFile, this, &MainWindow::openFile);
  connect(welcomeView, &WelcomeView::openRecentProject, this,
          &MainWindow::setInitialDirectory);

  // Create menus first
  createMenus();

  // Setup UI
  setupUI();
  createStatusBar();
  createDockWidgets();
  loadSettings();

  // Show welcome view by default
  dockManager->setDockVisible(DockManager::DockWidgetType::ProjectTree, false);
  dockManager->setDockVisible(DockManager::DockWidgetType::Editor, false);
  dockManager->setDockVisible(DockManager::DockWidgetType::ContentView, false);
  dockManager->setDockVisible(DockManager::DockWidgetType::Terminal, false);

  setWindowTitle(tr("Modern C++ IDE"));

  // Ensure menubar is visible
  menuBar()->setVisible(true);

  // Show welcome view by default
  setCentralWidget(welcomeView);

  // Initialize default state
  dockManager->setDockVisible(DockManager::DockWidgetType::ProjectTree, false);
  dockManager->setDockVisible(DockManager::DockWidgetType::Editor, false);
  dockManager->setDockVisible(DockManager::DockWidgetType::ContentView, false);
  dockManager->setDockVisible(DockManager::DockWidgetType::Terminal, false);

  // Load session after everything is initialized
  QTimer::singleShot(0, this, &MainWindow::loadSessionState);
}

void MainWindow::setupUI() {
  setCentralWidget(welcomeView);

  // Configure editor tabs
  editorTabs->setTabsClosable(true);
  editorTabs->setMovable(true);
  editorTabs->setDocumentMode(true);
  connect(editorTabs, &QTabWidget::tabCloseRequested, this,
          &MainWindow::closeTab);

  // Connect project tree signals
  connect(projectTree, &ProjectTree::fileSelected, this,
          &MainWindow::handleFileSelected);
  connect(projectTree, &ProjectTree::directoryChanged, this,
          &MainWindow::handleDirectoryChanged);
  connect(projectTree, &ProjectTree::rootDirectoryChanged, this,
          &MainWindow::handleRootDirectoryChanged);

  // Connect dock manager signals
  connect(dockManager, &DockManager::layoutChanged, this,
          &MainWindow::handleLayoutChanged);
  connect(dockManager, &DockManager::dockVisibilityChanged, this,
          &MainWindow::handleDockVisibilityChanged);
  // Update welcome view with recent projects
  welcomeView->updateRecentProjects(recentProjects);
}

void MainWindow::createDockWidgets() {
  // Create dock widgets
  QDockWidget *projectDock = dockManager->addDockWidget(
      DockManager::DockWidgetType::ProjectTree, projectTree, tr("Project"));
  QDockWidget *editorDock = dockManager->addDockWidget(
      DockManager::DockWidgetType::Editor, editorTabs, tr("Editor"));
  QDockWidget *contentDock =
      dockManager->addDockWidget(DockManager::DockWidgetType::ContentView,
                                 contentView, tr("Content View"));
  QDockWidget *terminalDock = dockManager->addDockWidget(
      DockManager::DockWidgetType::Terminal, terminal, tr("Terminal"));

  // Set terminal dock properties
  terminalDock->setFeatures(QDockWidget::DockWidgetClosable |
                            QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable |
                            QDockWidget::DockWidgetVerticalTitleBar);
  terminal->setMinimumHeight(100);
  terminal->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

  // Hide all views by default
  projectDock->hide();
  editorDock->hide();
  contentDock->hide();
  terminalDock->hide();

  // Create default layout
  dockManager->resetLayout();
}

void MainWindow::createMenus() {
  QMenuBar *menuBar = this->menuBar();

  // File menu
  QMenu *fileMenu = menuBar->addMenu(tr("&File"));

  QAction *newAction = fileMenu->addAction(tr("&New File"));
  newAction->setShortcut(QKeySequence::New);
  connect(newAction, &QAction::triggered, this, &MainWindow::createNewFile);

  QAction *openAction = fileMenu->addAction(tr("&Open File..."));
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

  QAction *openFolderAction = fileMenu->addAction(tr("Open &Folder..."));
  openFolderAction->setShortcut(QKeySequence("Ctrl+K, Ctrl+O"));
  connect(openFolderAction, &QAction::triggered, this, &MainWindow::openFolder);

  fileMenu->addSeparator();

  QAction *saveAction = fileMenu->addAction(tr("&Save"));
  saveAction->setShortcut(QKeySequence::Save);
  connect(saveAction, &QAction::triggered, this, [this]() { saveFile(); });

  QAction *saveAsAction = fileMenu->addAction(tr("Save &As..."));
  saveAsAction->setShortcut(QKeySequence::SaveAs);
  connect(saveAsAction, &QAction::triggered, this, [this]() { saveFileAs(); });

  fileMenu->addSeparator();

  QAction *exitAction = fileMenu->addAction(tr("E&xit"));
  exitAction->setShortcut(QKeySequence::Quit);
  connect(exitAction, &QAction::triggered, this, &QWidget::close);

  // Edit menu
  QMenu *editMenu = menuBar->addMenu(tr("&Edit"));

  QAction *undoAction = editMenu->addAction(tr("&Undo"));
  undoAction->setShortcut(QKeySequence::Undo);
  connect(undoAction, &QAction::triggered, this, &MainWindow::undo);

  QAction *redoAction = editMenu->addAction(tr("&Redo"));
  redoAction->setShortcut(QKeySequence::Redo);
  connect(redoAction, &QAction::triggered, this, &MainWindow::redo);

  editMenu->addSeparator();

  QAction *cutAction = editMenu->addAction(tr("Cu&t"));
  cutAction->setShortcut(QKeySequence::Cut);
  connect(cutAction, &QAction::triggered, this, &MainWindow::cut);

  QAction *copyAction = editMenu->addAction(tr("&Copy"));
  copyAction->setShortcut(QKeySequence::Copy);
  connect(copyAction, &QAction::triggered, this, &MainWindow::copy);

  QAction *pasteAction = editMenu->addAction(tr("&Paste"));
  pasteAction->setShortcut(QKeySequence::Paste);
  connect(pasteAction, &QAction::triggered, this, &MainWindow::paste);

  // View menu
  QMenu *viewMenu = menuBar->addMenu(tr("&View"));
  QAction *webBrowserAction = viewMenu->addAction(tr("Web Browser"));
  webBrowserAction->setShortcut(QKeySequence("Ctrl+Shift+B"));
  connect(webBrowserAction, &QAction::triggered, this, [this]() {
    contentView->loadWebContent("https://www.google.com");
    dockManager->setDockVisible(DockManager::DockWidgetType::ContentView, true);
  });

  // Add dock widget toggles with new syntax
  QAction *projectTreeAction = viewMenu->addAction(tr("Project Tree"));
  projectTreeAction->setShortcut(QKeySequence("Ctrl+B"));
  connect(projectTreeAction, &QAction::triggered, this, [this]() {
    auto dock =
        dockManager->getDockWidget(DockManager::DockWidgetType::ProjectTree);
    if (dock)
      dock->setVisible(!dock->isVisible());
  });

  QAction *terminalAction = viewMenu->addAction(tr("Terminal"));
  terminalAction->setShortcut(QKeySequence("Ctrl+`"));
  connect(terminalAction, &QAction::triggered, this, [this]() {
    auto dock =
        dockManager->getDockWidget(DockManager::DockWidgetType::Terminal);
    if (dock)
      dock->setVisible(!dock->isVisible());
  });

  QAction *contentViewAction = viewMenu->addAction(tr("Content View"));
  contentViewAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
  connect(contentViewAction, &QAction::triggered, this, [this]() {
    auto dock =
        dockManager->getDockWidget(DockManager::DockWidgetType::ContentView);
    if (dock)
      dock->setVisible(!dock->isVisible());
  });

  viewMenu->addSeparator();

  // Layout management
  QAction *resetLayoutAction = viewMenu->addAction(tr("Reset Layout"));
  connect(resetLayoutAction, &QAction::triggered, this,
          &MainWindow::resetLayout);

  QAction *saveLayoutAction = viewMenu->addAction(tr("Save Layout"));
  connect(saveLayoutAction, &QAction::triggered, this, &MainWindow::saveLayout);

  QAction *loadLayoutAction = viewMenu->addAction(tr("Load Layout"));
  connect(loadLayoutAction, &QAction::triggered, this, &MainWindow::loadLayout);

  // Help menu
  QMenu *helpMenu = menuBar->addMenu(tr("&Help"));
  helpMenu->addAction(tr("&About"), this, &MainWindow::about);

  // Add Recent Projects submenu to File menu
  recentProjectsMenu = new QMenu(tr("Recent Projects"), this);
  fileMenu->insertMenu(fileMenu->actions().at(3),
                       recentProjectsMenu); // Insert after Open Folder
  updateRecentProjectsMenu();

  // Settings menu
  QMenu *settingsMenu = menuBar->addMenu(tr("&Settings"));
  QAction *preferencesAction = settingsMenu->addAction(tr("&Preferences"));
  preferencesAction->setShortcut(QKeySequence::Preferences);
  connect(preferencesAction, &QAction::triggered, this,
          &MainWindow::showPreferences);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);

  // Add dock widget toggles
  menu.addAction(tr("Show Project Tree"), [this]() {
    dockManager->setDockVisible(DockManager::DockWidgetType::ProjectTree, true);
  });
  menu.addAction(tr("Show Terminal"), [this]() {
    dockManager->setDockVisible(DockManager::DockWidgetType::Terminal, true);
  });
  menu.addAction(tr("Show Content View"), [this]() {
    dockManager->setDockVisible(DockManager::DockWidgetType::ContentView, true);
  });

  menu.addSeparator();

  // Layout management
  menu.addAction(tr("Reset Layout"), this, &MainWindow::resetLayout);
  menu.addAction(tr("Save Layout"), this, &MainWindow::saveLayout);
  menu.addAction(tr("Load Layout"), this, &MainWindow::loadLayout);

  menu.exec(event->globalPos());
}

void MainWindow::handleLayoutChanged() {
  // Update the view menu checkboxes
  updateViewMenu();

  // Save the current layout
  dockManager->saveLayout();
}

void MainWindow::handleDockVisibilityChanged(DockManager::DockWidgetType type,
                                             bool visible) {
  // Update the corresponding view menu action
  if (viewActions.contains(type)) {
    viewActions[type]->setChecked(visible);
  }
}

void MainWindow::resetLayout() { dockManager->resetLayout(); }

void MainWindow::saveLayout() {
  QString name = QInputDialog::getText(
      this, tr("Save Layout"), tr("Layout name:"), QLineEdit::Normal, "custom");
  if (!name.isEmpty()) {
    dockManager->saveLayout(name);
  }
}

void MainWindow::loadLayout() {
  QSettings settings;
  QStringList layouts = settings.childGroups();
  layouts.removeAll("layout");

  if (layouts.isEmpty()) {
    QMessageBox::information(this, tr("Load Layout"),
                             tr("No saved layouts found."));
    return;
  }

  bool ok;
  QString name = QInputDialog::getItem(
      this, tr("Load Layout"), tr("Select layout:"), layouts, 0, false, &ok);
  if (ok && !name.isEmpty()) {
    dockManager->loadLayout(name);
  }
}

void MainWindow::updateViewMenu() {
  for (auto it = viewActions.begin(); it != viewActions.end(); ++it) {
    it.value()->setChecked(dockManager->isDockVisible(it.key()));
  }
}

void MainWindow::createStatusBar() {
  // Create status bar widget to hold buttons
  QWidget *leftWidget = new QWidget(this);
  QHBoxLayout *leftLayout = new QHBoxLayout(leftWidget);
  leftLayout->setContentsMargins(0, 0, 0, 0);

  // Project tree button
  QToolButton *projectTreeButton = new QToolButton(this);
  projectTreeButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
  projectTreeButton->setToolTip(tr("Toggle Project Tree (Ctrl+B)"));
  projectTreeButton->setFixedSize(24, 24);
  connect(projectTreeButton, &QToolButton::clicked, this, [this]() {
    auto dock =
        dockManager->getDockWidget(DockManager::DockWidgetType::ProjectTree);
    if (dock)
      dock->setVisible(!dock->isVisible());
  });
  leftLayout->addWidget(projectTreeButton);

  // Add left widget to status bar
  statusBar()->addWidget(leftWidget);

  // Create right widget for other buttons
  QWidget *rightWidget = new QWidget(this);
  QHBoxLayout *rightLayout = new QHBoxLayout(rightWidget);
  rightLayout->setContentsMargins(0, 0, 0, 0);

  // Web browser button
  QToolButton *webButton = new QToolButton(this);
  webButton->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
  webButton->setToolTip(tr("Open Web Browser (Ctrl+Shift+B)"));
  webButton->setFixedSize(24, 24);
  connect(webButton, &QToolButton::clicked, this, [this]() {
    contentView->loadWebContent("https://www.google.com");
    dockManager->setDockVisible(DockManager::DockWidgetType::ContentView, true);
  });

  // Terminal button
  QToolButton *terminalButton = new QToolButton(this);
  terminalButton->setIcon(QIcon::fromTheme(
      "utilities-terminal", style()->standardIcon(QStyle::SP_CommandLink)));
  terminalButton->setToolTip(tr("Toggle Terminal (Ctrl+`)"));
  terminalButton->setFixedSize(24, 24);
  connect(terminalButton, &QToolButton::clicked, this, [this]() {
    auto terminalDock =
        dockManager->getDockWidget(DockManager::DockWidgetType::Terminal);
    if (terminalDock) {
      terminalDock->setVisible(!terminalDock->isVisible());
    }
  });

  rightLayout->addWidget(webButton);
  rightLayout->addWidget(terminalButton);

  statusBar()->addPermanentWidget(rightWidget);
}

void MainWindow::loadSettings() {
  QSettings settings;
  recentProjects = settings.value("recentProjects").toStringList();

  // Load window state and geometry if you want to restore last session
  if (settings.contains("windowGeometry")) {
    restoreGeometry(settings.value("windowGeometry").toByteArray());
  }
  if (settings.contains("windowState")) {
    restoreState(settings.value("windowState").toByteArray());
  }
}

void MainWindow::saveSettings() {
  QSettings settings;
  settings.setValue("recentProjects", recentProjects);
  settings.setValue("windowGeometry", saveGeometry());
  settings.setValue("windowState", saveState());
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (maybeSave()) {
    saveSettings();
    saveSessionState();
    event->accept();
  } else {
    event->ignore();
  }
}

bool MainWindow::maybeSave() {
  CodeEditor *editor = currentEditor();
  if (!editor || !editor->document()->isModified())
    return true;

  const QMessageBox::StandardButton ret = QMessageBox::warning(
      this, tr("Application"),
      tr("The document has been modified.\n"
         "Do you want to save your changes?"),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
  switch (ret) {
  case QMessageBox::Save:
    return saveFile();
  case QMessageBox::Cancel:
    return false;
  default:
    break;
  }
  return true;
}

void MainWindow::setInitialDirectory(const QString &path) {
  if (path.isEmpty() || !QDir(path).exists()) {
    return;
  }

  projectTree->setRootPath(path);
  projectPath = path;

  // Update recent projects list
  recentProjects.removeAll(path);
  recentProjects.prepend(path);
  while (recentProjects.size() > 10) {
    recentProjects.removeLast();
  }
  updateRecentProjectsMenu();
  welcomeView->updateRecentProjects(recentProjects);

  // Switch from welcome view to editor view
  if (centralWidget() == welcomeView) {
    welcomeView->setParent(nullptr);
    setCentralWidget(editorTabs);
  }

  // Show project tree
  dockManager->setDockVisible(DockManager::DockWidgetType::ProjectTree, true);
}

void MainWindow::handleFileSelected(const QString &filePath) {
  QFileInfo fileInfo(filePath);

  // Hide welcome view and switch to editor tabs
  if (centralWidget() == welcomeView) {
    welcomeView->setParent(nullptr);
    setCentralWidget(editorTabs);
  }

  // Show project tree
  dockManager->setDockVisible(DockManager::DockWidgetType::ProjectTree, true);

  // Check if file is already open
  for (int i = 0; i < editorTabs->count(); ++i) {
    if (CodeEditor *editor =
            qobject_cast<CodeEditor *>(editorTabs->widget(i))) {
      if (editor->property("filePath").toString() == filePath) {
        editorTabs->setCurrentIndex(i);
        dockManager->setDockVisible(DockManager::DockWidgetType::Editor, true);
        return;
      }
    }
  }
  loadFile(filePath);
}

void MainWindow::handleDirectoryChanged(const QString &path) {
  terminal->setWorkingDirectory(path);
}

void MainWindow::handleRootDirectoryChanged(const QString &path) {
  projectPath = path;
  updateWindowTitle();
}

void MainWindow::updateWindowTitle() {
  QString title = tr("Modern C++ IDE");
  if (!projectPath.isEmpty()) {
    QFileInfo info(projectPath);
    title = tr("%1 - %2").arg(info.fileName(), title);
  }
  setWindowTitle(title);
}

CodeEditor *MainWindow::currentEditor() {
  return qobject_cast<CodeEditor *>(editorTabs->currentWidget());
}

QString MainWindow::currentFilePath() {
  if (CodeEditor *editor = currentEditor()) {
    return editor->property("filePath").toString();
  }
  return QString();
}

void MainWindow::createNewFile() {
  // Hide welcome view and switch to editor tabs
  if (centralWidget() == welcomeView) {
    welcomeView->setParent(nullptr);
    setCentralWidget(editorTabs);
  }

  // Show project tree
  dockManager->setDockVisible(DockManager::DockWidgetType::ProjectTree, true);

  CodeEditor *editor = new CodeEditor(this);
  int index = editorTabs->addTab(editor, tr("untitled"));
  editorTabs->setCurrentIndex(index);
  dockManager->setDockVisible(DockManager::DockWidgetType::Editor, true);
}

void MainWindow::openFile() {
  QString fileName = QFileDialog::getOpenFileName(this);
  if (!fileName.isEmpty())
    loadFile(fileName);
}

void MainWindow::openFolder() { projectTree->openFolder(); }

void MainWindow::loadFile(const QString &filePath) {
  QFileInfo fileInfo(filePath);

  // Hide welcome view if it's still showing
  if (centralWidget() == welcomeView) {
    welcomeView->setParent(nullptr);
    setCentralWidget(editorTabs);
  }

  // Show project tree
  dockManager->setDockVisible(DockManager::DockWidgetType::ProjectTree, true);

  // Check if file is already open
  for (int i = 0; i < editorTabs->count(); ++i) {
    if (CodeEditor *editor =
            qobject_cast<CodeEditor *>(editorTabs->widget(i))) {
      if (editor->property("filePath").toString() == filePath) {
        editorTabs->setCurrentIndex(i);
        return;
      }
    }
  }

  // Check if it's a previewable file
  QRegularExpression previewableFiles(
      "\\.(jpg|jpeg|png|gif|bmp|pdf|html|htm)$",
      QRegularExpression::CaseInsensitiveOption);
  if (previewableFiles.match(filePath).hasMatch()) {
    contentView->loadFile(filePath);
    dockManager->setDockVisible(DockManager::DockWidgetType::ContentView, true);
    return;
  }

  // Load as text file
  QFile file(filePath);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    QMessageBox::warning(
        this, tr("Application"),
        tr("Cannot read file %1:\n%2.")
            .arg(QDir::toNativeSeparators(filePath), file.errorString()));
    return;
  }

  QTextStream in(&file);
  CodeEditor *editor = new CodeEditor(this);
  editor->setPlainText(in.readAll());
  editor->setProperty("filePath", filePath);

  int index = editorTabs->addTab(editor, fileInfo.fileName());
  editorTabs->setCurrentIndex(index);

  statusBar()->showMessage(tr("File loaded"), 2000);
}

bool MainWindow::saveFile() {
  QString currentPath = currentFilePath();
  if (currentPath.isEmpty()) {
    return saveFileAs();
  }
  return saveFile(currentPath);
}

bool MainWindow::saveFileAs() {
  QString fileName = QFileDialog::getSaveFileName(this);
  if (fileName.isEmpty())
    return false;
  return saveFile(fileName);
}

bool MainWindow::saveFile(const QString &filePath) {
  CodeEditor *editor = currentEditor();
  if (!editor)
    return false;

  QFile file(filePath);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(
        this, tr("Application"),
        tr("Cannot write file %1:\n%2.")
            .arg(QDir::toNativeSeparators(filePath), file.errorString()));
    return false;
  }

  QTextStream out(&file);
  out << editor->toPlainText();
  editor->setProperty("filePath", filePath);

  QFileInfo info(filePath);
  editorTabs->setTabText(editorTabs->currentIndex(), info.fileName());

  statusBar()->showMessage(tr("File saved"), 2000);
  return true;
}

void MainWindow::closeTab(int index) {
  if (CodeEditor *editor =
          qobject_cast<CodeEditor *>(editorTabs->widget(index))) {
    if (editor->document()->isModified()) {
      QMessageBox::StandardButton ret;
      ret = QMessageBox::warning(this, tr("Application"),
                                 tr("The document has been modified.\n"
                                    "Do you want to save your changes?"),
                                 QMessageBox::Save | QMessageBox::Discard |
                                     QMessageBox::Cancel);
      if (ret == QMessageBox::Save)
        saveFile();
      else if (ret == QMessageBox::Cancel)
        return;
    }
    editorTabs->removeTab(index);
    delete editor;
  }
}

void MainWindow::undo() {
  if (CodeEditor *editor = currentEditor())
    editor->undo();
}

void MainWindow::redo() {
  if (CodeEditor *editor = currentEditor())
    editor->redo();
}

void MainWindow::cut() {
  if (CodeEditor *editor = currentEditor())
    editor->cut();
}

void MainWindow::copy() {
  if (CodeEditor *editor = currentEditor())
    editor->copy();
}

void MainWindow::paste() {
  if (CodeEditor *editor = currentEditor())
    editor->paste();
}

void MainWindow::about() {
  QMessageBox::about(this, tr("About Modern C++ IDE"),
                     tr("A modern C++ IDE built with Qt 6.\n\n"
                        "Features:\n"
                        "- Project tree with file management\n"
                        "- Multi-tab code editor with syntax highlighting\n"
                        "- File preview for images and PDFs\n"
                        "- Integrated terminal\n"
                        "- Modern dark theme"));
}

void MainWindow::updateRecentProjectsMenu() {
  recentProjectsMenu->clear();

  for (const QString &project : recentProjects) {
    QAction *action = recentProjectsMenu->addAction(QDir(project).dirName());
    action->setData(project);
    action->setToolTip(project);
    connect(action, &QAction::triggered, this,
            [this, project]() { setInitialDirectory(project); });
  }

  if (!recentProjects.isEmpty()) {
    recentProjectsMenu->addSeparator();
    QAction *clearAction =
        recentProjectsMenu->addAction(tr("Clear Recent Projects"));
    connect(clearAction, &QAction::triggered, this, [this]() {
      recentProjects.clear();
      updateRecentProjectsMenu();
      saveSettings();
    });
  }
}

void MainWindow::showPreferences() {
  PreferencesDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    // Save settings
    QSettings settings;
    settings.setValue("editor/fontSize", dialog.getFontSize());
    settings.setValue("editor/fontFamily", dialog.getFontFamily());

    // Apply settings to all open editors
    applyEditorSettings();
  }
}

void MainWindow::applyEditorSettings() {
  QSettings settings;
  QFont font(settings.value("editor/fontFamily", "Monospace").toString(),
             settings.value("editor/fontSize", 11).toInt());

  // Apply to all open editor tabs
  for (int i = 0; i < editorTabs->count(); ++i) {
    if (CodeEditor *editor =
            qobject_cast<CodeEditor *>(editorTabs->widget(i))) {
      editor->setFont(font);
    }
  }
}

void MainWindow::saveSessionState() {
  QStringList openedFiles;
  QStringList openedDirs;
  QMap<QString, SessionSettings::WindowState> windowStates;

  // Collect opened files
  for (int i = 0; i < editorTabs->count(); ++i) {
    if (CodeEditor *editor =
            qobject_cast<CodeEditor *>(editorTabs->widget(i))) {
      QString filePath = editor->property("filePath").toString();
      if (!filePath.isEmpty()) {
        openedFiles << filePath;
      }
    }
  }

  // Add current project directory
  if (!projectPath.isEmpty()) {
    openedDirs << projectPath;
  }

  // Save window states
  if (auto dock = dockManager->getDockWidget(
          DockManager::DockWidgetType::ContentView)) {
    SessionSettings::WindowState state;
    state.isVisible = dock->isVisible();
    state.geometry = dock->saveGeometry();
    if (contentView) {
      state.tabStates = contentView->getTabStates();
    }
    windowStates["contentView"] = state;
  }

  // Save main window state
  SessionSettings::instance().saveSession(
      openedFiles, openedDirs, editorTabs->currentIndex(), windowStates,
      saveGeometry(), saveState());
}

void MainWindow::loadSessionState() {
  QStringList openedFiles;
  QStringList openedDirs;
  int currentTabIndex;
  QMap<QString, SessionSettings::WindowState> windowStates;
  QByteArray mainWindowGeometry;
  QByteArray mainWindowState;

  SessionSettings::instance().loadSession(openedFiles, openedDirs,
                                          currentTabIndex, windowStates,
                                          mainWindowGeometry, mainWindowState);

  // Load directories first
  for (const QString &dir : openedDirs) {
    if (QDir(dir).exists()) {
      setInitialDirectory(dir);
    }
  }

  // Load files
  for (const QString &file : openedFiles) {
    if (QFileInfo(file).exists()) {
      loadFile(file);
    }
  }

  // Restore window states
  if (!windowStates.isEmpty()) {
    const auto &contentViewState = windowStates.value("contentView");
    if (contentView && contentViewState.isVisible) {
      // Restore tabs
      contentView->restoreTabStates(contentViewState.tabStates);

      // Restore dock state
      if (auto dock = dockManager->getDockWidget(
              DockManager::DockWidgetType::ContentView)) {
        dock->setVisible(true);
        if (!contentViewState.geometry.isEmpty()) {
          dock->restoreGeometry(contentViewState.geometry);
        }
      }
    }
  }

  // Set current tab only if valid
  if (currentTabIndex >= 0 && currentTabIndex < editorTabs->count()) {
    editorTabs->setCurrentIndex(currentTabIndex);
  }

  // Restore window geometry and state last
  if (!mainWindowGeometry.isEmpty()) {
    restoreGeometry(mainWindowGeometry);
  }
  if (!mainWindowState.isEmpty()) {
    restoreState(mainWindowState);
  }
}
