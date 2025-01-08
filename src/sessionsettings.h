#pragma once
#include <QObject>
#include <QStringList>

class SessionSettings : public QObject {
    Q_OBJECT

public:
    static SessionSettings& instance();

    void saveSession(const QStringList& openedFiles,
                    const QStringList& openedDirs,
                    int currentTabIndex);

    void loadSession(QStringList& openedFiles,
                    QStringList& openedDirs,
                    int& currentTabIndex);

private:
    explicit SessionSettings(QObject *parent = nullptr);
    // Delete copy constructor and assignment operator
    SessionSettings(const SessionSettings&) = delete;
    SessionSettings& operator=(const SessionSettings&) = delete;

    QString getSessionFilePath() const;
    void ensureConfigDirectory() const;
};
