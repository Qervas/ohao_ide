#include "sessionsettings.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>

SessionSettings& SessionSettings::instance() {
    static SessionSettings instance;
    return instance;
}

SessionSettings::SessionSettings(QObject *parent)
    : QObject(parent) {
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

void SessionSettings::saveSession(const QStringList& openedFiles,
                                const QStringList& openedDirs,
                                int currentTabIndex) {
    QJsonObject sessionObj;

    QJsonArray filesArray = QJsonArray::fromStringList(openedFiles);
    QJsonArray dirsArray = QJsonArray::fromStringList(openedDirs);

    sessionObj["openedFiles"] = filesArray;
    sessionObj["openedDirs"] = dirsArray;
    sessionObj["currentTabIndex"] = currentTabIndex;

    QJsonDocument doc(sessionObj);
    QFile file(getSessionFilePath());

    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void SessionSettings::loadSession(QStringList& openedFiles,
                                QStringList& openedDirs,
                                int& currentTabIndex) {
    QFile file(getSessionFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        return;
    }

    QJsonObject sessionObj = doc.object();

    QJsonArray filesArray = sessionObj["openedFiles"].toArray();
    QJsonArray dirsArray = sessionObj["openedDirs"].toArray();

    openedFiles.clear();
    openedDirs.clear();

    for (const QJsonValue& value : filesArray) {
        if (value.isString()) {
            openedFiles << value.toString();
        }
    }

    for (const QJsonValue& value : dirsArray) {
        if (value.isString()) {
            openedDirs << value.toString();
        }
    }

    currentTabIndex = sessionObj["currentTabIndex"].toInt(0);
}
