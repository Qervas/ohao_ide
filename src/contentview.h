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
  explicit ContentView(QWidget *parent = nullptr);
  void loadFile(const QString &filePath);
  void loadWebContent(const QString &url);
  QString getCurrentUrl() const;
  QString getCurrentFilePath() const;

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
