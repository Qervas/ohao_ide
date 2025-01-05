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
    bool saveFile();
    bool saveFileAs();
    bool saveFile(const QString &filePath);
    void closeTab(int index);
    void openFolder();
    void handleFileSelected(const QString &filePath);
    void handleDirectoryChanged(const QString &path);
    void handleRootDirectoryChanged(const QString &path);
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void about();

private:
    void setupUI();
    void createMenus();
    void createStatusBar();
    void loadSettings();
    void saveSettings();
    void closeEvent(QCloseEvent *event) override;
    CodeEditor* currentEditor();
    QString currentFilePath();
    void updateWindowTitle();
    bool maybeSave();
    void loadFile(const QString &filePath);

    ProjectTree *projectTree;
    QTabWidget *editorTabs;
    FilePreview *filePreview;
    Terminal *terminal;
    QString projectPath;
}; 