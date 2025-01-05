#pragma once
#include <QTreeView>
#include <QFileSystemModel>
#include <QMenu>
#include <QStringList>

class ProjectTree : public QTreeView {
    Q_OBJECT

public:
    explicit ProjectTree(QWidget *parent = nullptr);
    void setRootPath(const QString &path);
    QString getRootPath() const { return currentRootPath; }
    void openFolder(const QString &path = QString());

signals:
    void fileSelected(const QString &filePath);
    void directoryChanged(const QString &path);
    void rootDirectoryChanged(const QString &path);

private slots:
    void onItemClicked(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);
    void createNewFile();
    void createNewFolder();
    void deleteItem();
    void renameItem();
    void openContainingFolder();
    void copyFilePath();
    void copyRelativePath();

private:
    QFileSystemModel *model;
    QMenu *contextMenu;
    QMenu *fileContextMenu;
    QMenu *folderContextMenu;
    QString currentRootPath;

    void setupFileSystemModel();
    void setupTreeView();
    void setupContextMenus();
    QStringList getDefaultFilters() const;
    QString getRelativePath(const QString &absolutePath) const;
    void createContextMenuActions(QMenu *menu, bool isFile);
}; 