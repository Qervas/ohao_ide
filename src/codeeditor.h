#pragma once
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include "dockwidgetbase.h"
#include "cpphighlighter.h"

class LineNumberArea;
class QDialog;
class QLineEdit;
class QCheckBox;
class QPushButton;

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
protected:
    void keyPressEvent(QKeyEvent *e) override;
};

class CodeEditor : public DockWidgetBase {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor();

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
    void setFont(const QFont &font);

    // Make these public for LineNumberArea
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    // Intelligent indentation methods (made public for CustomPlainTextEdit)
    void handleIndent(bool indent);
    bool handleSmartBackspace();
    void handleNewLine();
    bool isIntelligentIndentEnabled() const { return m_intelligentIndent; }
    void setIntelligentIndent(bool enabled) { m_intelligentIndent = enabled; }

    // Search and Replace methods
    void showFindDialog();
    void showReplaceDialog();
    void findNext();
    void findPrevious();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void toggleLineComment();
    bool isLineCommented(const QString &text) const;
    void find();
    void replace();
    void replaceAll();
    void updateSearchHighlight();

private:
    CustomPlainTextEdit *m_editor;
    LineNumberArea *m_lineNumberArea;
    CppHighlighter *m_highlighter;
    bool m_intelligentIndent;

    // Search and Replace members
    QDialog *m_findDialog;
    QDialog *m_replaceDialog;
    QLineEdit *m_findLineEdit;
    QLineEdit *m_replaceLineEdit;
    QCheckBox *m_caseSensitiveCheckBox;
    QCheckBox *m_wholeWordsCheckBox;
    QCheckBox *m_regexCheckBox;
    QPushButton *m_findNextButton;
    QPushButton *m_findPrevButton;
    QPushButton *m_replaceButton;
    QPushButton *m_replaceAllButton;
    QString m_lastSearchText;
    QTextDocument::FindFlags m_searchFlags;

    void setupUI();
    void setupEditor();
    void setupSearchDialogs();
    QString getIndentString() const;
    int getIndentLevel(const QString &text) const;
    bool findText(const QString &text, QTextDocument::FindFlags flags);
    void clearSearchHighlights();
    void updateTabWidth();
}; 