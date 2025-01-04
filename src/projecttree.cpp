#include "projecttree.h"
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>

ProjectTree::ProjectTree(QWidget *parent) : QTreeView(parent) {
    setupFileSystemModel();
    setupTreeView();
}

void ProjectTree::setupFileSystemModel() {
    model = new QFileSystemModel(this);
    
    // Set filters for showing all relevant files
    model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    
    // Set name filters but don't hide the filtered ones (just gray them out)
    model->setNameFilterDisables(false);
    model->setNameFilters(getDefaultFilters());
    
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
    
    // Enable selection
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Connect signals
    connect(this, &QTreeView::clicked, this, &ProjectTree::onItemClicked);
    connect(this, &QTreeView::doubleClicked, this, &ProjectTree::onItemDoubleClicked);
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

void ProjectTree::setRootPath(const QString &path) {
    if (path.isEmpty() || !QDir(path).exists()) {
        return;
    }

    currentPath = path;
    QModelIndex index = model->setRootPath(path);
    setRootIndex(index);
    
    // Expand the root item
    expand(index);
    
    emit directoryChanged(path);
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
    } else if (fileInfo.isDir()) {
        // Toggle expand/collapse on directory double-click
        isExpanded(index) ? collapse(index) : expand(index);
    }
} 