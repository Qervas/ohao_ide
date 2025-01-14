#include "lspclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDir>

LSPClient::LSPClient(QObject *parent)
    : QObject(parent), m_server(nullptr), m_initialized(false), m_nextId(1) {
}

LSPClient::~LSPClient() {
    stopServer();
}

bool LSPClient::startServer(const QString &command) {
    if (m_server) {
        return false;
    }

    m_server = new QProcess(this);
    connect(m_server, &QProcess::readyReadStandardOutput, this, &LSPClient::handleServerOutput);
    connect(m_server, &QProcess::readyReadStandardError, this, &LSPClient::handleServerError);
    connect(m_server, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LSPClient::handleServerFinished);

    m_server->start(command);
    return m_server->waitForStarted();
}

void LSPClient::stopServer() {
    if (m_server) {
        m_server->terminate();
        if (!m_server->waitForFinished(3000)) {
            m_server->kill();
        }
        delete m_server;
        m_server = nullptr;
    }
    m_initialized = false;
}

bool LSPClient::isServerRunning() const {
    return m_server && m_server->state() == QProcess::Running;
}

void LSPClient::initialize(const QString &rootPath) {
    QJsonObject params;
    params["processId"] = QJsonValue::Null;
    params["rootUri"] = uriFromPath(rootPath);
    params["capabilities"] = QJsonObject({
        {"textDocument", QJsonObject({
            {"completion", QJsonObject({
                {"completionItem", QJsonObject({
                    {"snippetSupport", true}
                })}
            })},
            {"hover", QJsonObject({
                {"contentFormat", QJsonArray({"markdown", "plaintext"})}
            })},
            {"definition", QJsonObject()}
        })}
    });

    sendRequest("initialize", params);
}

void LSPClient::didOpen(const QString &uri, const QString &languageId, const QString &text) {
    QJsonObject params;
    params["textDocument"] = QJsonObject({
        {"uri", uri},
        {"languageId", languageId},
        {"version", 1},
        {"text", text}
    });

    sendNotification("textDocument/didOpen", params);
}

void LSPClient::didChange(const QString &uri, const QString &text) {
    QJsonObject params;
    params["textDocument"] = QJsonObject({
        {"uri", uri},
        {"version", 1}
    });
    params["contentChanges"] = QJsonArray({
        QJsonObject({{"text", text}})
    });

    sendNotification("textDocument/didChange", params);
}

void LSPClient::didClose(const QString &uri) {
    QJsonObject params;
    params["textDocument"] = QJsonObject({
        {"uri", uri}
    });

    sendNotification("textDocument/didClose", params);
}

void LSPClient::requestCompletion(const QString &uri, int line, int character) {
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    params["position"] = QJsonObject({
        {"line", line},
        {"character", character}
    });

    sendRequest("textDocument/completion", params);
}

void LSPClient::requestHover(const QString &uri, int line, int character) {
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    params["position"] = QJsonObject({
        {"line", line},
        {"character", character}
    });

    sendRequest("textDocument/hover", params);
}

void LSPClient::requestDefinition(const QString &uri, int line, int character) {
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    params["position"] = QJsonObject({
        {"line", line},
        {"character", character}
    });

    sendRequest("textDocument/definition", params);
}

void LSPClient::handleServerOutput() {
    m_buffer.append(m_server->readAllStandardOutput());

    while (!m_buffer.isEmpty()) {
        // Look for the Content-Length header
        int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) {
            // Header not complete yet
            return;
        }

        // Parse the Content-Length header
        QByteArray header = m_buffer.left(headerEnd);
        int contentLength = -1;
        QList<QByteArray> headers = header.split('\n');
        for (const QByteArray &h : headers) {
            if (h.startsWith("Content-Length: ")) {
                contentLength = h.mid(16).trimmed().toInt();
                break;
            }
        }

        if (contentLength == -1) {
            // Invalid header, clear buffer and return
            m_buffer.clear();
            return;
        }

        // Check if we have the complete message
        int messageStart = headerEnd + 4; // Skip \r\n\r\n
        int totalLength = messageStart + contentLength;
        if (m_buffer.size() < totalLength) {
            // Message not complete yet
            return;
        }

        // Extract and process the message
        QByteArray message = m_buffer.mid(messageStart, contentLength);
        processMessage(message);

        // Remove processed message from buffer
        m_buffer.remove(0, totalLength);
    }
}

void LSPClient::processMessage(const QByteArray &message) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message, &error);
    if (error.error != QJsonParseError::NoError) {
        emit serverError(tr("Failed to parse JSON message: %1").arg(error.errorString()));
        return;
    }

    if (!doc.isObject()) {
        emit serverError(tr("Invalid message format: not a JSON object"));
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("id")) {
        handleResponse(obj);
    } else if (obj.contains("method")) {
        handleNotification(obj);
    } else if (obj.contains("error")) {
        emit serverError(obj["error"].toObject()["message"].toString());
    }
}

void LSPClient::handleServerError() {
    QString error = m_server->readAllStandardError();
    emit serverError(error);
}

void LSPClient::handleServerFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        emit serverError(tr("Language server crashed or exited with error code %1").arg(exitCode));
    }
    m_initialized = false;
}

void LSPClient::sendRequest(const QString &method, const QJsonObject &params) {
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = m_nextId;
    request["method"] = method;
    request["params"] = params;

    m_pendingRequests[m_nextId] = method;
    m_nextId++;

    QJsonDocument doc(request);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    QString message = QString("Content-Length: %1\r\n\r\n%2")
                         .arg(data.length())
                         .arg(QString::fromUtf8(data));
    
    m_server->write(message.toUtf8());
}

void LSPClient::sendNotification(const QString &method, const QJsonObject &params) {
    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = method;
    notification["params"] = params;

    QJsonDocument doc(notification);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    QString message = QString("Content-Length: %1\r\n\r\n%2")
                         .arg(data.length())
                         .arg(QString::fromUtf8(data));
    
    m_server->write(message.toUtf8());
}

void LSPClient::handleResponse(const QJsonObject &response) {
    int id = response["id"].toInt();
    QString method = m_pendingRequests.take(id);

    if (method == "initialize") {
        m_initialized = true;
        emit initialized();
    } else if (method == "textDocument/completion") {
        QJsonArray completions = response["result"].toObject()["items"].toArray();
        emit completionReceived(completions);
    } else if (method == "textDocument/hover") {
        QString contents = response["result"].toObject()["contents"].toString();
        emit hoverReceived(contents);
    } else if (method == "textDocument/definition") {
        QJsonObject location = response["result"].toArray().first().toObject();
        QString uri = location["uri"].toString();
        QJsonObject range = location["range"].toObject();
        QJsonObject start = range["start"].toObject();
        emit definitionReceived(uri, start["line"].toInt(), start["character"].toInt());
    }
}

void LSPClient::handleNotification(const QJsonObject &notification) {
    QString method = notification["method"].toString();
    if (method == "textDocument/publishDiagnostics") {
        QJsonObject params = notification["params"].toObject();
        QString uri = params["uri"].toString();
        QJsonArray diagnostics = params["diagnostics"].toArray();
        emit diagnosticsReceived(uri, diagnostics);
    }
}

QString LSPClient::uriFromPath(const QString &path) const {
    return QUrl::fromLocalFile(QDir::cleanPath(path)).toString();
}

QString LSPClient::pathFromUri(const QString &uri) const {
    return QUrl(uri).toLocalFile();
} 