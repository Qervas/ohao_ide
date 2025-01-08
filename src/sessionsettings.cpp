#include "sessionsettings.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

SessionSettings &SessionSettings::instance() {
  static SessionSettings instance;
  return instance;
}

SessionSettings::SessionSettings(QObject *parent) : QObject(parent) {
  ensureConfigDirectory();
}

QString SessionSettings::getSessionFilePath() const {
  QString configPath = QDir::currentPath() + "/.ohao-ide";
  return configPath + "/session.json";
}

void SessionSettings::ensureConfigDirectory() const {
  QString configPath = QDir::currentPath() + "/.ohao-ide";
  QDir dir(configPath);
  if (!dir.exists()) {
    dir.mkpath(".");
  }
}

void SessionSettings::saveSession(
    const QStringList &openedFiles, const QStringList &openedDirs,
    int currentTabIndex, const QMap<QString, WindowState> &windowStates,
    const QByteArray &mainWindowGeometry, const QByteArray &mainWindowState) {
  QJsonObject sessionObj;

  sessionObj["openedFiles"] = QJsonArray::fromStringList(openedFiles);
  sessionObj["openedDirs"] = QJsonArray::fromStringList(openedDirs);
  sessionObj["currentTabIndex"] = currentTabIndex;
  sessionObj["mainWindowGeometry"] = QString(mainWindowGeometry.toBase64());
  sessionObj["mainWindowState"] = QString(mainWindowState.toBase64());

  // Save window states
  QJsonObject windowStatesObj;
  for (auto it = windowStates.constBegin(); it != windowStates.constEnd();
       ++it) {
    QJsonObject stateObj;
    stateObj["url"] = it.value().url;
    stateObj["filePath"] = it.value().filePath;
    stateObj["isVisible"] = it.value().isVisible;
    stateObj["geometry"] = QString(it.value().geometry.toBase64());

    // Save tab states
    QJsonArray tabStatesArray;
    for (const auto &tabState : it.value().tabStates) {
      QJsonObject tabObj;
      tabObj["type"] = tabState.type;
      tabObj["url"] = tabState.url;
      tabObj["filePath"] = tabState.filePath;
      tabObj["title"] = tabState.title;
      tabStatesArray.append(tabObj);
    }
    stateObj["tabStates"] = tabStatesArray;

    windowStatesObj[it.key()] = stateObj;
  }
  sessionObj["windowStates"] = windowStatesObj;

  QJsonDocument doc(sessionObj);
  QFile file(getSessionFilePath());
  if (file.open(QIODevice::WriteOnly)) {
    file.write(doc.toJson());
  }
}

void SessionSettings::loadSession(QStringList &openedFiles,
                                  QStringList &openedDirs, int &currentTabIndex,
                                  QMap<QString, WindowState> &windowStates,
                                  QByteArray &mainWindowGeometry,
                                  QByteArray &mainWindowState) {
  QFile file(getSessionFilePath());
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }

  QByteArray data = file.readAll();
  file.close();

  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(data, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject()) {
    qWarning() << "Failed to parse session file:" << error.errorString();
    return;
  }

  QJsonObject sessionObj = doc.object();

  QJsonArray filesArray = sessionObj["openedFiles"].toArray();
  QJsonArray dirsArray = sessionObj["openedDirs"].toArray();

  openedFiles.clear();
  openedDirs.clear();

  for (const auto &value : filesArray) {
    if (value.isString()) {
      openedFiles << value.toString();
    }
  }

  for (const auto &value : dirsArray) {
    if (value.isString()) {
      openedDirs << value.toString();
    }
  }
  currentTabIndex = sessionObj["currentTabIndex"].toInt(0);

  mainWindowGeometry = QByteArray::fromBase64(
      sessionObj["mainWindowGeometry"].toString().toLatin1());
  mainWindowState = QByteArray::fromBase64(
      sessionObj["mainWindowState"].toString().toLatin1());

  // Load window states
  QJsonObject windowStatesObj = sessionObj["windowStates"].toObject();
  for (auto it = windowStatesObj.constBegin(); it != windowStatesObj.constEnd();
       ++it) {
    QJsonObject stateObj = it.value().toObject();
    WindowState state;
    state.url = stateObj["url"].toString();
    state.filePath = stateObj["filePath"].toString();
    state.isVisible = stateObj["isVisible"].toBool();
    state.geometry =
        QByteArray::fromBase64(stateObj["geometry"].toString().toLatin1());

    // Load tab states
    QJsonArray tabStatesArray = stateObj["tabStates"].toArray();
    for (const auto &tabValue : tabStatesArray) {
      QJsonObject tabObj = tabValue.toObject();
      ContentView::TabState tabState;
      tabState.type = tabObj["type"].toString();
      tabState.url = tabObj["url"].toString();
      tabState.filePath = tabObj["filePath"].toString();
      tabState.title = tabObj["title"].toString();
      state.tabStates.append(tabState);
    }

    windowStates[it.key()] = state;
  }
}
