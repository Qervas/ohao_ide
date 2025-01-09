#pragma once
#include <QTreeView>
#include <QFileSystemModel>
#include <QMenu>
#include <QStringList>
#include <QFileSystemWatcher>

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

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

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
    void handleDirectoryChange(const QString &path);
    void handleFileChange(const QString &path);
    void refreshCurrentDirectory();

private:
    QFileSystemModel *model;
    QMenu *contextMenu;
    QMenu *fileContextMenu;
    QMenu *folderContextMenu;
    QString currentRootPath;
    QFileSystemWatcher *fsWatcher;

    void setupFileSystemModel();
    void setupTreeView();
    void setupContextMenus();
    void setupFileWatcher();
    QStringList getDefaultFilters() const;
    QString getRelativePath(const QString &absolutePath) const;
    void createContextMenuActions(QMenu *menu, bool isFile);
    void watchDirectory(const QString &path);
};
