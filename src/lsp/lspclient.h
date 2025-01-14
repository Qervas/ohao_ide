#pragma once
#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QMap>

class LSPClient : public QObject {
    Q_OBJECT

public:
    explicit LSPClient(QObject *parent = nullptr);
    ~LSPClient();

    bool startServer(const QString &command);
    void stopServer();
    bool isServerRunning() const;

    // LSP methods
    void initialize(const QString &rootPath);
    void didOpen(const QString &uri, const QString &languageId, const QString &text);
    void didChange(const QString &uri, const QString &text);
    void didClose(const QString &uri);
    void requestCompletion(const QString &uri, int line, int character);
    void requestHover(const QString &uri, int line, int character);
    void requestDefinition(const QString &uri, int line, int character);
    QString uriFromPath(const QString &path) const;

signals:
    void initialized();
    void completionReceived(const QJsonArray &completions);
    void hoverReceived(const QString &contents);
    void definitionReceived(const QString &uri, int line, int character);
    void diagnosticsReceived(const QString &uri, const QJsonArray &diagnostics);
    void serverError(const QString &message);

private slots:
    void handleServerOutput();
    void handleServerError();
    void handleServerFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *m_server;
    bool m_initialized;
    int m_nextId;
    QMap<int, QString> m_pendingRequests;
    QByteArray m_buffer; // Buffer for incomplete messages

    void sendRequest(const QString &method, const QJsonObject &params);
    void sendNotification(const QString &method, const QJsonObject &params);
    void handleResponse(const QJsonObject &response);
    void handleNotification(const QJsonObject &notification);
    void processMessage(const QByteArray &message);
    QString pathFromUri(const QString &uri) const;
}; 