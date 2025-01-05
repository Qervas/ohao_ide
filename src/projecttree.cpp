#include "projecttree.h"
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <qshortcut.h>
#include <QMouseEvent>


ProjectTree::ProjectTree(QWidget *parent) : QTreeView(parent) {
    setupFileSystemModel();
    setupTreeView();
    setupContextMenus();
    model->setRootPath("");
    setRootIndex(model->index(""));
    // Add F2 shortcut for rename
    QShortcut *renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(renameShortcut, &QShortcut::activated, this, &ProjectTree::renameItem);

    // Enable double click on empty area
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
}

void ProjectTree::setupFileSystemModel() {
    model = new QFileSystemModel(this);

    // Set filters for showing all relevant files
    model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    // Set name filters but don't hide the filtered ones (just gray them out)
    model->setNameFilterDisables(false);
    model->setNameFilters(getDefaultFilters());
    model->setReadOnly(false);  // Make sure the model is editable

    setModel(model);
}

void ProjectTree::setupTreeView() {
    // Hide the size, type, and date columns
    hideColumn(1); // Size
    hideColumn(2); // Type
    hideColumn(3); // Date Modified

    // Set header properties
    header()->setStretchLastSection(true);
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header()->hide(); // Hide header like VSCode

    // Enable selection
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    // Enable drag and drop
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

    // Set context menu policy
    setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect signals
    connect(this, &QTreeView::clicked, this, &ProjectTree::onItemClicked);
    connect(this, &QTreeView::doubleClicked, this, &ProjectTree::onItemDoubleClicked);
    connect(this, &QTreeView::customContextMenuRequested, this, &ProjectTree::showContextMenu);
}

void ProjectTree::setupContextMenus() {
    // Main context menu (for empty areas)
    contextMenu = new QMenu(this);
    QAction *openFolderAction = contextMenu->addAction(tr("Open Folder..."));
    connect(openFolderAction, &QAction::triggered, [this]() { openFolder(); });

    // File context menu
    fileContextMenu = new QMenu(this);
    createContextMenuActions(fileContextMenu, true);

    // Folder context menu
    folderContextMenu = new QMenu(this);
    createContextMenuActions(folderContextMenu, false);
}

void ProjectTree::createContextMenuActions(QMenu *menu, bool isFile) {
    // Common actions
    QAction *copyPathAction = menu->addAction(tr("Copy Path"));
    QAction *copyRelativePathAction = menu->addAction(tr("Copy Relative Path"));
    menu->addSeparator();

    QAction *renameAction = menu->addAction(tr("Rename"));
    QAction *deleteAction = menu->addAction(tr("Delete"));
    menu->addSeparator();

    if (!isFile) {
        QAction *newFileAction = menu->addAction(tr("New File"));
        QAction *newFolderAction = menu->addAction(tr("New Folder"));
        menu->addSeparator();

        connect(newFileAction, &QAction::triggered, this, &ProjectTree::createNewFile);
        connect(newFolderAction, &QAction::triggered, this, &ProjectTree::createNewFolder);
    }

    QAction *openContainingFolderAction = menu->addAction(tr("Open Containing Folder"));

    // Connect actions
    connect(copyPathAction, &QAction::triggered, this, &ProjectTree::copyFilePath);
    connect(copyRelativePathAction, &QAction::triggered, this, &ProjectTree::copyRelativePath);
    connect(renameAction, &QAction::triggered, this, &ProjectTree::renameItem);
    connect(deleteAction, &QAction::triggered, this, &ProjectTree::deleteItem);
    connect(openContainingFolderAction, &QAction::triggered, this, &ProjectTree::openContainingFolder);
}

void ProjectTree::openFolder(const QString &path) {
    QString folderPath = path;
    if (folderPath.isEmpty()) {
        folderPath = QFileDialog::getExistingDirectory(this,
            tr("Open Folder"), QDir::homePath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    }

    if (!folderPath.isEmpty()) {
        setRootPath(folderPath);
    }
}

void ProjectTree::setRootPath(const QString &path) {
    if (path.isEmpty() || !QDir(path).exists()) {
        return;
    }

    currentRootPath = path;
    QModelIndex index = model->setRootPath(path);
    setRootIndex(index);

    // Expand the root item
    expand(index);

    emit rootDirectoryChanged(path);
    emit directoryChanged(path);
}

QString ProjectTree::getRelativePath(const QString &absolutePath) const {
    if (currentRootPath.isEmpty()) {
        return absolutePath;
    }
    return QDir(currentRootPath).relativeFilePath(absolutePath);
}

void ProjectTree::showContextMenu(const QPoint &pos) {
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) {
        contextMenu->exec(viewport()->mapToGlobal(pos));
        return;
    }

    QString path = model->filePath(index);
    QFileInfo fileInfo(path);

    if (fileInfo.isFile()) {
        fileContextMenu->exec(viewport()->mapToGlobal(pos));
    } else {
        folderContextMenu->exec(viewport()->mapToGlobal(pos));
    }
}

void ProjectTree::onItemClicked(const QModelIndex &index) {
    QString path = model->filePath(index);
    QFileInfo fileInfo(path);

    if (fileInfo.isDir()) {
        emit directoryChanged(path);
    }
}

void ProjectTree::onItemDoubleClicked(const QModelIndex &index) {
    QString path = model->filePath(index);
    QFileInfo fileInfo(path);

    if (fileInfo.isFile()) {
        emit fileSelected(path);
    }
}

void ProjectTree::mouseDoubleClickEvent(QMouseEvent *event) {
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        // Double clicked on empty area
        createNewFile();
        return;
    }
    QTreeView::mouseDoubleClickEvent(event);
}

void ProjectTree::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete) {
        deleteItem();
        event->accept();
    } else {
        QTreeView::keyPressEvent(event);
    }
}

void ProjectTree::createNewFile() {
    QModelIndex index = currentIndex();
    QString parentPath = index.isValid() ? model->filePath(index) : currentRootPath;
    QFileInfo fileInfo(parentPath);
    if (fileInfo.isFile()) {
        parentPath = fileInfo.dir().absolutePath();
    }

    bool ok;
    QString fileName = QInputDialog::getText(this, tr("New File"),
        tr("File name:"), QLineEdit::Normal, "newfile.txt", &ok);

    if (ok && !fileName.isEmpty()) {
        QString filePath = QDir(parentPath).filePath(fileName);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            QModelIndex newIndex = model->index(filePath);
            setCurrentIndex(newIndex);
            edit(newIndex);
        }
    }
}

void ProjectTree::createNewFolder() {
    QModelIndex index = currentIndex();
    QString parentPath = index.isValid() ? model->filePath(index) : currentRootPath;
    QFileInfo fileInfo(parentPath);
    if (fileInfo.isFile()) {
        parentPath = fileInfo.dir().absolutePath();
    }

    bool ok;
    QString folderName = QInputDialog::getText(this, tr("New Folder"),
        tr("Folder name:"), QLineEdit::Normal, "New Folder", &ok);

    if (ok && !folderName.isEmpty()) {
        QDir dir(parentPath);
        if (dir.mkdir(folderName)) {
            QModelIndex newIndex = model->index(dir.filePath(folderName));
            setCurrentIndex(newIndex);
            edit(newIndex);
        }
    }
}

void ProjectTree::deleteItem() {
    QModelIndex index = currentIndex();
    if (!index.isValid()) return;

    QString path = model->filePath(index);
    QFileInfo fileInfo(path);

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Delete"),
        tr("Are you sure you want to delete '%1'?").arg(fileInfo.fileName()),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (fileInfo.isDir()) {
            QDir dir(path);
            dir.removeRecursively();
        } else {
            QFile::remove(path);
        }
    }
}

void ProjectTree::renameItem() {
    QModelIndex index = currentIndex();
    if (!index.isValid()) return;
    edit(index);
}

void ProjectTree::openContainingFolder() {
    QModelIndex index = currentIndex();
    if (!index.isValid()) return;

    QString path = model->filePath(index);
    QFileInfo fileInfo(path);
    QString folderPath = fileInfo.isFile() ? fileInfo.dir().absolutePath() : path;

    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

void ProjectTree::copyFilePath() {
    QModelIndex index = currentIndex();
    if (!index.isValid()) return;

    QString path = model->filePath(index);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(path);
}

void ProjectTree::copyRelativePath() {
    QModelIndex index = currentIndex();
    if (!index.isValid()) return;

    QString path = model->filePath(index);
    QString relativePath = getRelativePath(path);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(relativePath);
}

QStringList ProjectTree::getDefaultFilters() const {
    return QStringList{
        // C/C++ files
        "*.c", "*.cpp", "*.cxx", "*.cc",
        "*.h", "*.hpp", "*.hxx", "*.hh",
        "*.inl", "*.inc",

        // Build files
        "CMakeLists.txt", "*.cmake",
        "Makefile", "*.mk",
        "*.pro", "*.pri",

        // Web development
        "*.html", "*.css", "*.js", "*.ts",
        "*.jsx", "*.tsx", "*.vue",

        // Python
        "*.py", "*.pyw", "*.pyx",

        // Documentation
        "*.md", "*.txt", "*.rst",
        "*.pdf", "*.doc", "*.docx",

        // Data files
        "*.json", "*.xml", "*.yaml", "*.yml",
        "*.csv", "*.ini", "*.conf",

        // Images
        "*.png", "*.jpg", "*.jpeg", "*.gif",
        "*.bmp", "*.svg", "*.ico",

        // Shell scripts
        "*.sh", "*.bash", "*.zsh",
        "*.bat", "*.cmd", "*.ps1"
    };
}
