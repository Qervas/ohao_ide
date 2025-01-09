#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QFontDatabase>
#include <QSettings>
#include <QShortcut>
#include <QKeySequence>

class LineNumberArea : public QWidget {
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);
    highlighter = new CppHighlighter(document());

    // Set modern font
    QSettings settings;
    QFont font(
        settings.value("editor/fontFamily", "Monospace").toString(),
        settings.value("editor/fontSize", 12).toInt()
    );
    setFont(font);

    // Set tab width to 4 spaces
    QFontMetrics metrics(font);
    setTabStopDistance(4 * metrics.horizontalAdvance(' '));

    // Initialize word wrap from settings
    bool wordWrap = settings.value("editor/wordWrap", true).toBool();
    setLineWrapMode(wordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);

    // Set modern colors
    QPalette p = palette();
    p.setColor(QPalette::Base, QColor("#1E1E1E"));
    p.setColor(QPalette::Text, QColor("#D4D4D4"));
    setPalette(p);

    // Connect signals
    connect(this, &CodeEditor::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Add comment toggle shortcut
    QShortcut *commentShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this);
    connect(commentShortcut, &QShortcut::activated, this, &CodeEditor::toggleLineComment);

    // Set scroll bar policy
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 13 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                    lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        
        QColor lineColor = QColor("#2D2D30");
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor("#1E1E1E"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#858585"));
            painter.drawText(0, top, lineNumberArea->width() - 3, fontMetrics().height(),
                           Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::toggleLineComment() {
    QTextCursor cursor = textCursor();
    QTextDocument *doc = document();
    
    // Get the range of selected text
    int startBlock = cursor.selectionStart();
    int endBlock = cursor.selectionEnd();
    
    cursor.beginEditBlock();
    
    // Convert positions to blocks
    QTextBlock startBlockObj = doc->findBlock(startBlock);
    QTextBlock endBlockObj = doc->findBlock(endBlock);
    
    // If selection ends at start of a block, don't include that block
    if (endBlock > 0 && endBlock == endBlockObj.position()) {
        endBlockObj = endBlockObj.previous();
    }
    
    // Check if all selected lines are commented
    bool allCommented = true;
    QTextBlock block = startBlockObj;
    while (block.isValid() && block.blockNumber() <= endBlockObj.blockNumber()) {
        if (!isLineCommented(block.text())) {
            allCommented = false;
            break;
        }
        block = block.next();
    }
    
    // Toggle comments for all selected lines
    block = startBlockObj;
    while (block.isValid() && block.blockNumber() <= endBlockObj.blockNumber()) {
        cursor.setPosition(block.position());
        QString text = block.text();
        
        if (allCommented) {
            // Remove comment
            if (isLineCommented(text)) {
                cursor.movePosition(QTextCursor::StartOfLine);
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                cursor.removeSelectedText();
                cursor.movePosition(QTextCursor::StartOfLine);
            }
        } else {
            // Add comment
            if (!isLineCommented(text)) {
                cursor.movePosition(QTextCursor::StartOfLine);
                cursor.insertText("//");
            }
        }
        block = block.next();
    }
    
    cursor.endEditBlock();
}

bool CodeEditor::isLineCommented(const QString &text) const {
    QString trimmed = text.trimmed();
    return trimmed.startsWith("//");
}

// Remove the toggleWordWrap() method 