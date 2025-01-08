#pragma once
#include "contentview.h"
#include <QObject>
#include <QStringList>

class SessionSettings : public QObject {
  Q_OBJECT

public:
  struct WindowState {
    QString url;      // For web browser
    QString filePath; // For file preview
    bool isVisible;
    QByteArray geometry;
    QList<ContentView::TabState> tabStates;
  };
  static SessionSettings &instance();

  void saveSession(const QStringList &openedFiles,
                   const QStringList &openedDirs, int currentTabIndex,
                   const QMap<QString, WindowState> &windowStates,
                   const QByteArray &mainWindowGeometry,
                   const QByteArray &mainWindowState);

  void loadSession(QStringList &openedFiles, QStringList &openedDirs,
                   int &currentTabIndex,
                   QMap<QString, WindowState> &windowStates,
                   QByteArray &mainWindowGeometry, QByteArray &mainWindowState);

private:
  explicit SessionSettings(QObject *parent = nullptr);
  // Delete copy constructor and assignment operator
  SessionSettings(const SessionSettings &) = delete;
  SessionSettings &operator=(const SessionSettings &) = delete;

  QString getSessionFilePath() const;
  void ensureConfigDirectory() const;
};
