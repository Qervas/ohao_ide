#include "customtextedit.h"
#include "codeeditor.h"
#include <QKeyEvent>
#include <QTimer>

CustomPlainTextEdit::CustomPlainTextEdit(QWidget *parent)
    : QPlainTextEdit(parent), m_hoverTimer(nullptr) {
}

CustomPlainTextEdit::~CustomPlainTextEdit() {
    if (m_hoverTimer) {
        m_hoverTimer->stop();
        delete m_hoverTimer;
    }
}

void CustomPlainTextEdit::keyPressEvent(QKeyEvent *e) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(parent());
    if (!editor) {
        QPlainTextEdit::keyPressEvent(e);
        return;
    }

    // Handle Home key
    if (e->key() == Qt::Key_Home && !(e->modifiers() & Qt::ControlModifier)) {
        QTextCursor cursor = textCursor();
        int originalPosition = cursor.position();
        cursor.movePosition(QTextCursor::StartOfLine);
        int startOfLine = cursor.position();
        
        // Get the text of the current line
        QString lineText = cursor.block().text();
        
        // Find the first non-whitespace character
        int firstNonSpace = 0;
        while (firstNonSpace < lineText.length() && lineText[firstNonSpace].isSpace()) {
            firstNonSpace++;
        }
        
        // If we're already at the first non-space character, move to start of line
        // Or if we're at the start of line, move to first non-space
        if (originalPosition == startOfLine + firstNonSpace) {
            cursor.setPosition(startOfLine);
        } else {
            cursor.setPosition(startOfLine + firstNonSpace);
        }
        
        // Handle shift selection
        if (e->modifiers() & Qt::ShiftModifier) {
            cursor.setPosition(cursor.position(), QTextCursor::KeepAnchor);
        }
        
        setTextCursor(cursor);
        e->accept();
        return;
    }

    // Handle auto-pairing first, before any other key handling
    QString text = e->text();
    if (text.length() == 1) {
        QChar ch = text[0];
        if (ch == '(' || ch == '[' || ch == '{' || ch == '"' || ch == '\'' ||
            ch == '`' || ch == ')' || ch == ']' || ch == '}') {
            if (editor->handleAutoPair(e)) {
                e->accept();
                return;
            }
        }
    }

    // Handle other special keys
    if ((e->key() == Qt::Key_Tab || e->key() == Qt::Key_Backtab) && editor->isIntelligentIndentEnabled()) {
        bool isShiftTab = (e->key() == Qt::Key_Backtab) || (e->key() == Qt::Key_Tab && (e->modifiers() & Qt::ShiftModifier));
        editor->handleIndent(!isShiftTab);
        e->accept();
        return;
    } else if (e->key() == Qt::Key_Return && editor->isIntelligentIndentEnabled()) {
        editor->handleNewLine();
        e->accept();
        return;
    } else if (e->key() == Qt::Key_Backspace && editor->isIntelligentIndentEnabled()) {
        if (editor->handleSmartBackspace()) {
            e->accept();
            return;
        }
    }

    QPlainTextEdit::keyPressEvent(e);
}

void CustomPlainTextEdit::mousePressEvent(QMouseEvent *e) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(parent());
    if (!editor) {
        QPlainTextEdit::mousePressEvent(e);
        return;
    }

    // Handle Ctrl+Click for goto definition
    if (e->button() == Qt::LeftButton && e->modifiers() & Qt::ControlModifier) {
        QTextCursor cursor = cursorForPosition(e->pos());
        setTextCursor(cursor);
        editor->requestDefinition();
        e->accept();
        return;
    }

    QPlainTextEdit::mousePressEvent(e);
}

void CustomPlainTextEdit::mouseMoveEvent(QMouseEvent *e) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(parent());
    if (!editor) {
        QPlainTextEdit::mouseMoveEvent(e);
        return;
    }

    // Show hand cursor when Ctrl is pressed and hovering over text
    if (e->modifiers() & Qt::ControlModifier) {
        QTextCursor cursor = cursorForPosition(e->pos());
        QString word = editor->getWordUnderCursor(cursor);
        if (!word.isEmpty()) {
            viewport()->setCursor(Qt::PointingHandCursor);
        } else {
            viewport()->setCursor(Qt::IBeamCursor);
        }
    } else {
        viewport()->setCursor(Qt::IBeamCursor);
    }

    // Request hover information
    if (m_hoverTimer) {
        m_hoverTimer->stop();
    } else {
        m_hoverTimer = new QTimer(this);
        m_hoverTimer->setSingleShot(true);
        connect(m_hoverTimer, &QTimer::timeout, [this, editor]() {
            QPoint pos = mapFromGlobal(QCursor::pos());
            QTextCursor cursor = cursorForPosition(pos);
            editor->requestHover(cursor);
        });
    }
    m_hoverTimer->start(500); // 500ms delay before showing hover

    QPlainTextEdit::mouseMoveEvent(e);
} 