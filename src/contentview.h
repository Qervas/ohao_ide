#pragma once
#include "filepreview.h"
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QWebEngineView>
#include <QWidget>

class ContentView : public QWidget {
  Q_OBJECT
public:
  struct TabState {
    QString type;     // "web" or "file"
    QString url;      // For web content
    QString filePath; // For file preview
    QString title;    // Tab title
  };
  explicit ContentView(QWidget *parent = nullptr);
  void loadFile(const QString &filePath);
  void loadWebContent(const QString &url);
  QString getCurrentUrl() const;
  QString getCurrentFilePath() const;
  QList<TabState> getTabStates() const;
  void restoreTabStates(const QList<TabState> &states);

protected:
  void resizeEvent(QResizeEvent *event) override;

private slots:
  void handleNewTab(const QUrl &url);

private:
  enum class ViewMode { Empty, Preview, Web };

  QTabWidget *tabs;
  QString currentPath;
  QString currentTitle;

  void setupUI();
  bool isWebContent(const QString &path) const;
  void updateTheme();
  void closeTab(int index);
  QWidget *findTabByPath(const QString &path);
};
