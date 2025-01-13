#pragma once
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include "dockwidgetbase.h"
#include "cpphighlighter.h"

class LineNumberArea;

// Custom editor class to access protected members
class CustomPlainTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CustomPlainTextEdit(QWidget *parent = nullptr) : QPlainTextEdit(parent) {}
    QTextBlock firstVisibleBlock() const { return QPlainTextEdit::firstVisibleBlock(); }
    QRectF blockBoundingGeometry(const QTextBlock &block) const {
        return QPlainTextEdit::blockBoundingGeometry(block);
    }
    QRectF blockBoundingRect(const QTextBlock &block) const {
        return QPlainTextEdit::blockBoundingRect(block);
    }
    QPointF contentOffset() const { return QPlainTextEdit::contentOffset(); }
    void setViewportMargins(int left, int top, int right, int bottom) {
        QPlainTextEdit::setViewportMargins(left, top, right, bottom);
    }
};

class CodeEditor : public DockWidgetBase {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    // Override base class methods
    void setWorkingDirectory(const QString &path) override;
    bool canClose() override;
    void updateTheme() override;
    void focusWidget() override { m_editor->setFocus(); }
    bool hasUnsavedChanges() override { return m_editor->document()->isModified(); }

    // Editor specific methods
    void setPlainText(const QString &text) { m_editor->setPlainText(text); }
    QString toPlainText() const { return m_editor->toPlainText(); }
    void undo() { m_editor->undo(); }
    void redo() { m_editor->redo(); }
    void cut() { m_editor->cut(); }
    void copy() { m_editor->copy(); }
    void paste() { m_editor->paste(); }
    QTextDocument* document() const { return m_editor->document(); }
    void setLineWrapMode(QPlainTextEdit::LineWrapMode mode) { m_editor->setLineWrapMode(mode); }

    // Make these public for LineNumberArea
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void toggleLineComment();
    bool isLineCommented(const QString &text) const;

private:
    CustomPlainTextEdit *m_editor;
    LineNumberArea *m_lineNumberArea;
    CppHighlighter *m_highlighter;
    void setupUI();
    void setupEditor();
}; 