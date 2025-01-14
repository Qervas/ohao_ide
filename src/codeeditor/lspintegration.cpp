#include "lspintegration.h"
#include "../lsp/lspclient.h"
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QCompleter>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QToolTip>
#include <QDir>

LSPIntegration::LSPIntegration(QPlainTextEdit *editor)
    : QObject(editor), m_editor(editor), m_lspClient(new LSPClient(this)), m_serverInitialized(false) {
    setupLSPClient();
}

LSPIntegration::~LSPIntegration() {
    if (m_lspClient) {
        m_lspClient->stopServer();
    }
}

void LSPIntegration::setupLSPClient() {
    // Connect LSP client signals
    connect(m_lspClient, &LSPClient::initialized,
            [this]() { m_serverInitialized = true; });
    connect(m_lspClient, &LSPClient::completionReceived,
            this, &LSPIntegration::handleCompletionReceived);
    connect(m_lspClient, &LSPClient::hoverReceived,
            this, &LSPIntegration::handleHoverReceived);
    connect(m_lspClient, &LSPClient::definitionReceived,
            this, &LSPIntegration::handleDefinitionReceived);
    connect(m_lspClient, &LSPClient::diagnosticsReceived,
            this, &LSPIntegration::handleDiagnosticsReceived);
    connect(m_lspClient, &LSPClient::serverError,
            this, &LSPIntegration::handleServerError);

    // Start clangd server
    if (m_lspClient->startServer("clangd")) {
        m_lspClient->initialize(QDir::currentPath());
    }
}

void LSPIntegration::handleTextChanged() {
    if (!m_serverInitialized) return;

    QString uri = m_lspClient->uriFromPath(QDir::currentPath() + "/" + m_editor->windowTitle());
    m_lspClient->didChange(uri, m_editor->toPlainText());
}

void LSPIntegration::handleCursorPositionChanged() {
    if (!m_serverInitialized) return;

    QTextCursor cursor = m_editor->textCursor();
    int line = cursor.blockNumber();
    int character = cursor.positionInBlock();
    QString uri = m_lspClient->uriFromPath(QDir::currentPath() + "/" + m_editor->windowTitle());

    // Request hover information
    m_lspClient->requestHover(uri, line, character);

    // Request completion if typing
    QString currentWord = getCurrentWord();
    if (!currentWord.isEmpty()) {
        m_lspClient->requestCompletion(uri, line, character);
    }
}

void LSPIntegration::handleCompletionReceived(const QJsonArray &completions) {
    QStringList suggestions;
    for (const auto& val : completions) {
        if (val.isObject()) {
            QJsonObject item = val.toObject();
            suggestions << item["label"].toString();
        }
    }

    if (suggestions.isEmpty()) return;

    QCompleter *completer = new QCompleter(suggestions, this);
    completer->setWidget(m_editor);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    
    connect(completer, QOverload<const QString &>::of(&QCompleter::activated),
            [this](const QString &text) {
                QTextCursor cursor = m_editor->textCursor();
                QString currentWord = getCurrentWord();
                cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 
                                currentWord.length());
                cursor.insertText(text);
            });

    QRect cr = m_editor->cursorRect();
    cr.setWidth(completer->popup()->sizeHintForColumn(0) +
                completer->popup()->verticalScrollBar()->sizeHint().width());
    completer->complete(cr);
}

void LSPIntegration::handleHoverReceived(const QString &contents) {
    if (contents.isEmpty()) return;
    
    QRect rect = m_editor->cursorRect();
    QPoint pos = m_editor->viewport()->mapToGlobal(QPoint(rect.left(), rect.bottom()));
    QToolTip::showText(pos, contents);
}

void LSPIntegration::handleDefinitionReceived(const QString &uri, int line, int character) {
    emit gotoDefinitionRequested(uri, line, character);
}

void LSPIntegration::handleDiagnosticsReceived(const QString &uri, const QJsonArray &diagnostics) {
    QList<QTextEdit::ExtraSelection> selections;
    
    // Keep existing selections that are not diagnostic highlights
    for (const auto &selection : m_editor->extraSelections()) {
        if (selection.format.background().color().alpha() == 255) {
            selections.append(selection);
        }
    }

    for (const auto& val : diagnostics) {
        if (!val.isObject()) continue;
        
        QJsonObject diagnostic = val.toObject();
        QJsonObject range = diagnostic["range"].toObject();
        QJsonObject start = range["start"].toObject();
        QJsonObject end = range["end"].toObject();
        QString severity = diagnostic["severity"].toString();
        QString message = diagnostic["message"].toString();

        // Convert LSP positions to text positions
        QTextCursor cursor(m_editor->document());
        
        // Move to start position
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, start["line"].toInt());
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, start["character"].toInt());
        int startPos = cursor.position();

        // Move to end position
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, end["line"].toInt());
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, end["character"].toInt());
        int endPos = cursor.position();

        // Create selection
        QTextEdit::ExtraSelection selection;
        selection.cursor = QTextCursor(m_editor->document());
        selection.cursor.setPosition(startPos);
        selection.cursor.setPosition(endPos, QTextCursor::KeepAnchor);

        // Set format based on severity
        QColor underlineColor;
        if (severity == "1") {  // Error
            underlineColor = QColor("#FF0000");  // Red
        } else if (severity == "2") {  // Warning
            underlineColor = QColor("#FFA500");  // Orange
        } else if (severity == "3") {  // Information
            underlineColor = QColor("#2196F3");  // Blue
        } else if (severity == "4") {  // Hint
            underlineColor = QColor("#4CAF50");  // Green
        } else {
            underlineColor = QColor("#FF0000");  // Default to red
        }

        // Create squiggly underline effect
        QTextCharFormat format;
        format.setUnderlineColor(underlineColor);
        format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        format.setToolTip(message);  // Show diagnostic message on hover

        selection.format = format;
        selections.append(selection);
    }

    m_editor->setExtraSelections(selections);
}

void LSPIntegration::handleServerError(const QString &message) {
    qDebug() << "LSP Server Error:" << message;
}

QString LSPIntegration::getCurrentWord() const {
    QTextCursor cursor = m_editor->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    return cursor.selectedText();
}

QPair<int, int> LSPIntegration::getCursorPosition() const {
    QTextCursor cursor = m_editor->textCursor();
    return qMakePair(cursor.blockNumber(), cursor.positionInBlock());
}

void LSPIntegration::requestDefinition() {
    if (!m_serverInitialized) return;

    QTextCursor cursor = m_editor->textCursor();
    int line = cursor.blockNumber();
    int character = cursor.positionInBlock();
    QString uri = m_lspClient->uriFromPath(QDir::currentPath() + "/" + m_editor->windowTitle());

    m_lspClient->requestDefinition(uri, line, character);
}

void LSPIntegration::requestHover(const QTextCursor &cursor) {
    if (!m_serverInitialized) return;

    int line = cursor.blockNumber();
    int character = cursor.positionInBlock();
    QString uri = m_lspClient->uriFromPath(QDir::currentPath() + "/" + m_editor->windowTitle());

    m_lspClient->requestHover(uri, line, character);
}

QString LSPIntegration::getWordUnderCursor(const QTextCursor &cursor) const {
    QTextCursor wordCursor = cursor;
    wordCursor.select(QTextCursor::WordUnderCursor);
    return wordCursor.selectedText();
}

bool LSPIntegration::isInsideString() const {
    QTextCursor cursor = m_editor->textCursor();
    int position = cursor.position();
    QString text = m_editor->toPlainText();
    
    // Scan from the start of the line to the cursor position
    int lineStart = text.lastIndexOf('\n', position - 1) + 1;
    if (lineStart == 0) lineStart = 0;
    
    bool inString = false;
    QChar stringChar;
    
    for (int i = lineStart; i < position; i++) {
        if (i >= text.length()) break;
        
        QChar currentChar = text[i];
        
        // Handle escape sequences
        if (currentChar == '\\' && i + 1 < text.length()) {
            i++; // Skip next character
            continue;
        }
        
        // Toggle string state if we encounter a quote
        if ((currentChar == '"' || currentChar == '\'' || currentChar == '`')) {
            if (!inString) {
                inString = true;
                stringChar = currentChar;
            } else if (currentChar == stringChar) {
                inString = false;
            }
        }
    }
    
    return inString;
}

bool LSPIntegration::isInsideComment() const {
    QTextCursor cursor = m_editor->textCursor();
    int position = cursor.position();
    QString text = m_editor->toPlainText();
    
    // Find start of the line
    int lineStart = text.lastIndexOf('\n', position - 1) + 1;
    if (lineStart == 0) lineStart = 0;
    
    // Check for // comments
    for (int i = lineStart; i < position; i++) {
        if (i + 1 >= text.length()) break;
        
        if (text[i] == '/' && text[i + 1] == '/') {
            return true;
        }
    }
    
    return false;
} 