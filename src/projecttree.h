#pragma once
#include <QTreeView>
#include <QFileSystemModel>
#include <QStringList>

class ProjectTree : public QTreeView {
    Q_OBJECT

public:
    explicit ProjectTree(QWidget *parent = nullptr);
    void setRootPath(const QString &path);
    QString currentPath;

signals:
    void fileSelected(const QString &filePath);
    void directoryChanged(const QString &path);

private:
    QFileSystemModel *model;
    QStringList getDefaultFilters() const;
    void setupFileSystemModel();
    void setupTreeView();

private slots:
    void onItemClicked(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);
}; 