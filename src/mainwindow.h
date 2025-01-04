#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include "projecttree.h"
#include "codeeditor.h"
#include "filepreview.h"
#include "terminal.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void setInitialDirectory(const QString &path);

private slots:
    void createNewFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void closeTab(int index);
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void onDirectoryChanged(const QString &path);
    void onFileSelected(const QString &path);
    void openFolder();

private:
    void setupUI();
    void createMenus();
    void createActions();

    ProjectTree *projectTree;
    QTabWidget *editorTabs;
    FilePreview *filePreview;
    Terminal *terminal;

    void openFileInEditor(const QString &path);
    bool isPreviewableFile(const QString &path);
}; 