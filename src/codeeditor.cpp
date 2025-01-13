#include "codeeditor.h"
#include <QTextBlock>
#include <QPainter>
#include <QShortcut>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QMessageBox>

class LineNumberArea : public QWidget {
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}
    QSize sizeHint() const override { return QSize(codeEditor->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

CodeEditor::CodeEditor(QWidget *parent) : DockWidgetBase(parent) {
    m_editor = new CustomPlainTextEdit(this);
    m_lineNumberArea = new LineNumberArea(this);
    m_highlighter = new CppHighlighter(m_editor->document());

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_editor);

    // Set up editor properties
    QFont font("Monospace");
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    font.setPointSize(10);
    m_editor->setFont(font);

    // Set tab width
    QFontMetrics metrics(font);
    m_editor->setTabStopDistance(4 * metrics.horizontalAdvance(' '));

    // Line wrapping
    bool wordWrap = true; // This should come from settings
    m_editor->setLineWrapMode(wordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);

    // Set up line number area
    connect(m_editor, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(m_editor, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Set up shortcuts
    QShortcut *commentShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this);
    connect(commentShortcut, &QShortcut::activated, this, &CodeEditor::toggleLineComment);

    // Set scrollbar policy
    m_editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, m_editor->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + m_editor->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    m_editor->setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(m_editor->viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    DockWidgetBase::resizeEvent(e);
    QRect cr = m_editor->contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                   lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!m_editor->isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        
        QColor lineColor = QColor(45, 45, 45);  // Dark gray color
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = m_editor->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    m_editor->setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = m_editor->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset()).top());
    int bottom = top + qRound(m_editor->blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, m_lineNumberArea->width(), m_editor->fontMetrics().height(),
                           Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(m_editor->blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::toggleLineComment() {
    QTextCursor cursor = m_editor->textCursor();
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    int startBlock = cursor.blockNumber();
    cursor.setPosition(end);
    int endBlock = cursor.blockNumber();

    cursor.setPosition(start);
    cursor.beginEditBlock();

    bool allCommented = true;
    for (int i = startBlock; i <= endBlock; ++i) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        QString line = cursor.block().text();
        if (!isLineCommented(line)) {
            allCommented = false;
            break;
        }
        cursor.movePosition(QTextCursor::NextBlock);
    }

    cursor.setPosition(start);
    for (int i = startBlock; i <= endBlock; ++i) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        QString line = cursor.block().text();
        
        if (allCommented) {
            // Remove comment
            if (isLineCommented(line)) {
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                cursor.removeSelectedText();
                if (line.length() > 2 && line[2] == ' ') {
                    cursor.deleteChar();
                }
            }
        } else {
            // Add comment
            if (!isLineCommented(line)) {
                cursor.insertText("// ");
            }
        }
        cursor.movePosition(QTextCursor::NextBlock);
    }

    cursor.endEditBlock();
}

bool CodeEditor::isLineCommented(const QString &text) const {
    QString trimmed = text.trimmed();
    return trimmed.startsWith("//");
}

void CodeEditor::setWorkingDirectory(const QString &path) {
    DockWidgetBase::setWorkingDirectory(path);
    // No specific implementation needed for editor
}

bool CodeEditor::canClose() {
    return !hasUnsavedChanges() || 
           QMessageBox::question(this, tr("Unsaved Changes"),
                               tr("This document has unsaved changes. Do you want to close it anyway?"),
                               QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

void CodeEditor::updateTheme() {
    // This will be implemented when we add theme support
    // For now, we can set some default colors
    QPalette p = m_editor->palette();
    p.setColor(QPalette::Base, QColor("#1E1E1E"));
    p.setColor(QPalette::Text, QColor("#D4D4D4"));
    m_editor->setPalette(p);
}

// Remove the toggleWordWrap() method 