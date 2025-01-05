#include "mainwindow.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QScreen>
#include <QSplitter>
#include <QFileInfo>
#include <QDir>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUI();
    createMenus();
    createStatusBar();
    loadSettings();

    setWindowTitle(tr("Modern C++ IDE"));
}

void MainWindow::setupUI()
{
    // Create main splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left side - Project Tree
    projectTree = new ProjectTree(this);
    mainSplitter->addWidget(projectTree);
    
    // Center and right side splitter
    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal);
    
    // Center - Editor tabs
    editorTabs = new QTabWidget;
    editorTabs->setTabsClosable(true);
    editorTabs->setMovable(true);
    rightSplitter->addWidget(editorTabs);
    
    // Right side vertical splitter
    QSplitter *rightVerticalSplitter = new QSplitter(Qt::Vertical);
    
    // Preview area
    filePreview = new FilePreview(this);
    rightVerticalSplitter->addWidget(filePreview);
    
    // Terminal
    terminal = new Terminal(this);
    rightVerticalSplitter->addWidget(terminal);
    
    rightSplitter->addWidget(rightVerticalSplitter);
    mainSplitter->addWidget(rightSplitter);
    
    // Set initial splitter sizes
    mainSplitter->setStretchFactor(0, 1);  // Project tree
    mainSplitter->setStretchFactor(1, 4);  // Right side
    rightSplitter->setStretchFactor(0, 3); // Editor
    rightSplitter->setStretchFactor(1, 1); // Preview/Terminal
    
    setCentralWidget(mainSplitter);

    // Connect signals
    connect(editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(projectTree, &ProjectTree::fileSelected, this, &MainWindow::handleFileSelected);
    connect(projectTree, &ProjectTree::directoryChanged, this, &MainWindow::handleDirectoryChanged);
    connect(projectTree, &ProjectTree::rootDirectoryChanged, this, &MainWindow::handleRootDirectoryChanged);
}

void MainWindow::createMenus()
{
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
    connect(saveAction, &QAction::triggered, this, [this]() {
        saveFile();
    });

    QAction *saveAsAction = fileMenu->addAction(tr("Save &As..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, [this]() {
        saveFileAs();
    });

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

    // Help menu
    QMenu *helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::about);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    // Restore window geometry
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = screen()->availableGeometry();
        resize(availableGeometry.width() * 3 / 4, availableGeometry.height() * 3 / 4);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
    
    // Restore last opened project
    QString lastProject = settings.value("lastProject").toString();
    if (!lastProject.isEmpty() && QDir(lastProject).exists()) {
        setInitialDirectory(lastProject);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("lastProject", projectPath);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::maybeSave()
{
    CodeEditor *editor = currentEditor();
    if (!editor || !editor->document()->isModified())
        return true;

    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("Application"),
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

void MainWindow::setInitialDirectory(const QString &path)
{
    if (path.isEmpty() || !QDir(path).exists())
        return;

    projectPath = path;
    projectTree->setRootPath(path);
    terminal->setWorkingDirectory(path);
    updateWindowTitle();
}

void MainWindow::handleFileSelected(const QString &filePath)
{
    loadFile(filePath);
}

void MainWindow::handleDirectoryChanged(const QString &path)
{
    terminal->setWorkingDirectory(path);
}

void MainWindow::handleRootDirectoryChanged(const QString &path)
{
    projectPath = path;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    QString title = tr("Modern C++ IDE");
    if (!projectPath.isEmpty()) {
        QFileInfo info(projectPath);
        title = tr("%1 - %2").arg(info.fileName(), title);
    }
    setWindowTitle(title);
}

CodeEditor* MainWindow::currentEditor()
{
    return qobject_cast<CodeEditor*>(editorTabs->currentWidget());
}

QString MainWindow::currentFilePath()
{
    if (CodeEditor *editor = currentEditor()) {
        return editor->property("filePath").toString();
    }
    return QString();
}

void MainWindow::createNewFile()
{
    CodeEditor *editor = new CodeEditor(this);
    int index = editorTabs->addTab(editor, tr("untitled"));
    editorTabs->setCurrentIndex(index);
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
        loadFile(fileName);
}

void MainWindow::openFolder()
{
    projectTree->openFolder();
}

void MainWindow::loadFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    
    // Check if file is already open
    for (int i = 0; i < editorTabs->count(); ++i) {
        if (CodeEditor *editor = qobject_cast<CodeEditor*>(editorTabs->widget(i))) {
            if (editor->property("filePath").toString() == filePath) {
                editorTabs->setCurrentIndex(i);
                return;
            }
        }
    }
    
    // Check if it's a previewable file
    QRegularExpression previewableFiles("^(jpg|jpeg|png|gif|bmp|pdf)$");
    if (previewableFiles.match(fileInfo.suffix().toLower()).hasMatch()) {
        filePreview->loadFile(filePath);
        return;
    }
    
    // Load as text file
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
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

bool MainWindow::saveFile()
{
    QString currentPath = currentFilePath();
    if (currentPath.isEmpty()) {
        return saveFileAs();
    }
    return saveFile(currentPath);
}

bool MainWindow::saveFileAs()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return false;
    return saveFile(fileName);
}

bool MainWindow::saveFile(const QString &filePath)
{
    CodeEditor *editor = currentEditor();
    if (!editor)
        return false;

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                           tr("Cannot write file %1:\n%2.")
                           .arg(QDir::toNativeSeparators(filePath),
                               file.errorString()));
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

void MainWindow::closeTab(int index)
{
    if (CodeEditor *editor = qobject_cast<CodeEditor*>(editorTabs->widget(index))) {
        if (editor->document()->isModified()) {
            QMessageBox::StandardButton ret;
            ret = QMessageBox::warning(this, tr("Application"),
                tr("The document has been modified.\n"
                   "Do you want to save your changes?"),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            if (ret == QMessageBox::Save)
                saveFile();
            else if (ret == QMessageBox::Cancel)
                return;
        }
        editorTabs->removeTab(index);
        delete editor;
    }
}

void MainWindow::undo()
{
    if (CodeEditor *editor = currentEditor())
        editor->undo();
}

void MainWindow::redo()
{
    if (CodeEditor *editor = currentEditor())
        editor->redo();
}

void MainWindow::cut()
{
    if (CodeEditor *editor = currentEditor())
        editor->cut();
}

void MainWindow::copy()
{
    if (CodeEditor *editor = currentEditor())
        editor->copy();
}

void MainWindow::paste()
{
    if (CodeEditor *editor = currentEditor())
        editor->paste();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Modern C++ IDE"),
        tr("A modern C++ IDE built with Qt 6.\n\n"
           "Features:\n"
           "- Project tree with file management\n"
           "- Multi-tab code editor with syntax highlighting\n"
           "- File preview for images and PDFs\n"
           "- Integrated terminal\n"
           "- Modern dark theme"));
} 