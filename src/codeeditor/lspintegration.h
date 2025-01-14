#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class LSPClient;
class QPlainTextEdit;
class QTextCursor;

class LSPIntegration : public QObject {
    Q_OBJECT

public:
    explicit LSPIntegration(QPlainTextEdit *editor);
    ~LSPIntegration();

    void setupLSPClient();
    void handleTextChanged();
    void handleCursorPositionChanged();
    void requestDefinition();
    void requestHover(const QTextCursor &cursor);
    QString getWordUnderCursor(const QTextCursor &cursor) const;
    QPair<int, int> getCursorPosition() const;

private slots:
    void handleCompletionReceived(const QJsonArray &completions);
    void handleHoverReceived(const QString &contents);
    void handleDefinitionReceived(const QString &uri, int line, int character);
    void handleDiagnosticsReceived(const QString &uri, const QJsonArray &diagnostics);
    void handleServerError(const QString &message);

signals:
    void gotoDefinitionRequested(const QString &uri, int line, int character);

private:
    QString getCurrentWord() const;
    bool isInsideString() const;
    bool isInsideComment() const;

    QPlainTextEdit *m_editor;
    LSPClient *m_lspClient;
    bool m_serverInitialized;
}; 