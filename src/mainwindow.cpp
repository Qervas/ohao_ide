#include "mainwindow.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDir>
#include <QMimeDatabase>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    createMenus();
    createActions();
    
    // Set initial directory to home
    QString homePath = QDir::homePath();
    projectTree->setRootPath(homePath);
    terminal->setWorkingDirectory(homePath);
    
    setWindowTitle("Modern C++ IDE");
}

void MainWindow::setupUI() {
    // Main splitter
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
    QList<int> mainSizes;
    mainSizes << 200 << 800;  // Project tree: 200px, Rest: 800px
    mainSplitter->setSizes(mainSizes);
    
    QList<int> rightSizes;
    rightSizes << 600 << 200;  // Editor: 600px, Preview/Terminal: 200px
    rightSplitter->setSizes(rightSizes);
    
    QList<int> verticalSizes;
    verticalSizes << 200 << 200;  // Preview: 200px, Terminal: 200px
    rightVerticalSplitter->setSizes(verticalSizes);
    
    setCentralWidget(mainSplitter);

    // Connect project tree signals
    connect(projectTree, &ProjectTree::directoryChanged,
            this, &MainWindow::onDirectoryChanged);
    connect(projectTree, &ProjectTree::fileSelected,
            this, &MainWindow::onFileSelected);
            
    // Set minimum sizes
    projectTree->setMinimumWidth(100);
    editorTabs->setMinimumWidth(300);
    rightVerticalSplitter->setMinimumWidth(200);
    filePreview->setMinimumHeight(100);
    terminal->setMinimumHeight(100);
    
    // Set stretch factors
    mainSplitter->setStretchFactor(0, 0);  // Project tree doesn't stretch
    mainSplitter->setStretchFactor(1, 1);  // Right side stretches
    rightSplitter->setStretchFactor(0, 1);  // Editor stretches
    rightSplitter->setStretchFactor(1, 0);  // Preview/Terminal doesn't stretch
    
    // Set initial window size
    resize(1200, 800);
}

void MainWindow::createMenus() {
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::createNewFile);
    fileMenu->addAction(tr("&Open"), QKeySequence::Open, this, &MainWindow::openFile);
    fileMenu->addAction(tr("Open &Folder..."), tr("Ctrl+K, Ctrl+O"), this, &MainWindow::openFolder);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::saveFile);
    fileMenu->addAction(tr("Save &As"), QKeySequence::SaveAs, this, &MainWindow::saveFileAs);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, this, &QWidget::close);

    // Edit menu
    QMenu *editMenu = menuBar->addMenu(tr("&Edit"));
    editMenu->addAction(tr("&Undo"), QKeySequence::Undo, this, &MainWindow::undo);
    editMenu->addAction(tr("&Redo"), QKeySequence::Redo, this, &MainWindow::redo);
    editMenu->addSeparator();
    editMenu->addAction(tr("Cu&t"), QKeySequence::Cut, this, &MainWindow::cut);
    editMenu->addAction(tr("&Copy"), QKeySequence::Copy, this, &MainWindow::copy);
    editMenu->addAction(tr("&Paste"), QKeySequence::Paste, this, &MainWindow::paste);
}

void MainWindow::createActions() {
    // Add actions for file operations
    connect(editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
}

// Add these slot implementations
void MainWindow::createNewFile() {
    CodeEditor *editor = new CodeEditor(this);
    editorTabs->addTab(editor, tr("Untitled"));
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open File"), "",
        tr("C++ Files (*.cpp *.h *.hpp);;All Files (*)"));

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot open file %1:\n%2.")
            .arg(fileName)
            .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    CodeEditor *editor = new CodeEditor(this);
    editor->setPlainText(in.readAll());
    
    // Get the file name without path for the tab
    QFileInfo fileInfo(fileName);
    editor->setProperty("filePath", fileName);  // Store path as property
    int index = editorTabs->addTab(editor, fileInfo.fileName());
    
    // Switch to the new tab
    editorTabs->setCurrentIndex(index);
    
    file.close();
    
    // Update window title
    setWindowTitle(QString("%1 - Modern C++ IDE").arg(fileInfo.fileName()));
}

void MainWindow::saveFile() {
    CodeEditor *editor = qobject_cast<CodeEditor*>(editorTabs->currentWidget());
    if (!editor)
        return;

    // Get the file path from widget property
    QString fileName = editor->property("filePath").toString();
    
    // If this is a new file, get a save location
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(this,
            tr("Save File"), "",
            tr("C++ Files (*.cpp *.h *.hpp);;All Files (*)"));

        if (fileName.isEmpty())
            return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot save file %1:\n%2.")
            .arg(fileName)
            .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << editor->toPlainText();
    file.close();

    // Update tab text and property
    QFileInfo fileInfo(fileName);
    editor->setProperty("filePath", fileName);
    editorTabs->setTabText(editorTabs->currentIndex(), fileInfo.fileName());

    // Update window title
    setWindowTitle(QString("%1 - Modern C++ IDE").arg(fileInfo.fileName()));
}

void MainWindow::saveFileAs() {
    CodeEditor *editor = qobject_cast<CodeEditor*>(editorTabs->currentWidget());
    if (!editor)
        return;

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save File As"), "",
        tr("C++ Files (*.cpp *.h *.hpp);;All Files (*)"));

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot save file %1:\n%2.")
            .arg(fileName)
            .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << editor->toPlainText();
    file.close();

    // Update tab text and property
    QFileInfo fileInfo(fileName);
    editor->setProperty("filePath", fileName);
    editorTabs->setTabText(editorTabs->currentIndex(), fileInfo.fileName());

    // Update window title
    setWindowTitle(QString("%1 - Modern C++ IDE").arg(fileInfo.fileName()));
}

void MainWindow::closeTab(int index) {
    CodeEditor *editor = qobject_cast<CodeEditor*>(editorTabs->widget(index));
    if (!editor)
        return;

    // Check if the file has unsaved changes
    if (editor->document()->isModified()) {
        QMessageBox::StandardButton ret = QMessageBox::warning(this,
            tr("Unsaved Changes"),
            tr("The document has been modified.\nDo you want to save your changes?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save)
            saveFile();
        else if (ret == QMessageBox::Cancel)
            return;
    }

    editorTabs->removeTab(index);
    delete editor;

    // Update window title if no tabs are left
    if (editorTabs->count() == 0)
        setWindowTitle("Modern C++ IDE");
}

void MainWindow::undo() {
    if (auto editor = qobject_cast<CodeEditor*>(editorTabs->currentWidget()))
        editor->undo();
}

void MainWindow::redo() {
    if (auto editor = qobject_cast<CodeEditor*>(editorTabs->currentWidget()))
        editor->redo();
}

void MainWindow::cut() {
    if (auto editor = qobject_cast<CodeEditor*>(editorTabs->currentWidget()))
        editor->cut();
}

void MainWindow::copy() {
    if (auto editor = qobject_cast<CodeEditor*>(editorTabs->currentWidget()))
        editor->copy();
}

void MainWindow::paste() {
    if (auto editor = qobject_cast<CodeEditor*>(editorTabs->currentWidget()))
        editor->paste();
}

void MainWindow::onDirectoryChanged(const QString &path) {
    // Update terminal working directory when project directory changes
    terminal->setWorkingDirectory(path);
}

void MainWindow::onFileSelected(const QString &path) {
    // Update terminal working directory to the file's directory
    QFileInfo fileInfo(path);
    terminal->setWorkingDirectory(fileInfo.absolutePath());
    
    // Check file type and handle accordingly
    if (isPreviewableFile(path)) {
        filePreview->previewFile(path);
    } else {
        openFileInEditor(path);
    }
}

void MainWindow::openFileInEditor(const QString &path) {
    // Existing file opening logic
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot open file %1:\n%2.")
            .arg(path)
            .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    CodeEditor *editor = new CodeEditor(this);
    editor->setPlainText(in.readAll());
    
    QFileInfo fileInfo(path);
    editor->setProperty("filePath", path);
    int index = editorTabs->addTab(editor, fileInfo.fileName());
    
    editorTabs->setCurrentIndex(index);
    file.close();
    
    setWindowTitle(QString("%1 - Modern C++ IDE").arg(fileInfo.fileName()));
}

void MainWindow::openFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Folder"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        projectTree->setRootPath(dir);
        terminal->setWorkingDirectory(dir);
        setWindowTitle(QString("%1 - Modern C++ IDE").arg(QDir(dir).dirName()));
    }
}

void MainWindow::setInitialDirectory(const QString &path) {
    projectTree->setRootPath(path);
    terminal->setWorkingDirectory(path);
    
    // Update window title if it's not the home directory
    if (path != QDir::homePath()) {
        setWindowTitle(QString("%1 - Modern C++ IDE").arg(QDir(path).dirName()));
    }
}

bool MainWindow::isPreviewableFile(const QString &path) {
    QMimeDatabase db;
    QString mimeType = db.mimeTypeForFile(path).name();
    
    // Image files
    if (mimeType.startsWith("image/")) {
        return true;
    }
    
    // PDF files
    if (mimeType == "application/pdf") {
        return true;
    }
    
    return false;
} 