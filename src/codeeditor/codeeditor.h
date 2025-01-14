#pragma once
#include "../highlighters/basehighlighter.h"
#include "codeeditor/bracketmatching.h"
#include "codeeditor/quotematching.h"
#include "customtextedit.h"
#include "dockwidgetbase.h"
#include "linenumberarea.h"
#include "lsp/lspclient.h"
#include <QMap>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

// Forward declarations
class QDialog;
class QLineEdit;
class QCheckBox;
class QPushButton;
class SearchDialog;

class CodeEditor : public DockWidgetBase {
  Q_OBJECT

public:
  explicit CodeEditor(QWidget *parent = nullptr);
  ~CodeEditor();

  // DockWidgetBase interface
  void setWorkingDirectory(const QString &path) override;
  bool canClose() override;
  void updateTheme() override;
  void focusWidget() override { m_editor->setFocus(); }
  bool hasUnsavedChanges() override {
    return m_editor->document()->isModified();
  }
  QString workingDirectory() const { return m_workingDirectory; }

  // Editor specific methods
  void setPlainText(const QString &text) { m_editor->setPlainText(text); }
  QString toPlainText() const { return m_editor->toPlainText(); }
  void undo() { m_editor->undo(); }
  void redo() { m_editor->redo(); }
  void cut() { m_editor->cut(); }
  void copy() { m_editor->copy(); }
  void paste() { m_editor->paste(); }
  QTextDocument *document() const { return m_editor->document(); }
  void setLineWrapMode(QPlainTextEdit::LineWrapMode mode) {
    m_editor->setLineWrapMode(mode);
  }
  void setFont(const QFont &font);

  // Line number area
  int lineNumberAreaWidth() const;
  void lineNumberAreaPaintEvent(QPaintEvent *event);

  // Intelligent indentation
  void handleIndent(bool indent);
  bool handleSmartBackspace();
  void handleNewLine();
  bool isIntelligentIndentEnabled() const { return m_intelligentIndent; }
  void setIntelligentIndent(bool enabled) { m_intelligentIndent = enabled; }

  // Search and Replace
  void showFindDialog();
  void showReplaceDialog();
  void findNext();
  void findPrevious();

  // Syntax highlighting
  void setSyntaxHighlighting(bool enabled);
  bool isSyntaxHighlightingEnabled() const;

  // Auto-pairing
  bool handleAutoPair(QKeyEvent *e);
  bool handleKeyPress(QKeyEvent *e);
  bool handleBracketPair(QChar ch, QTextCursor &cursor,
                         const QString &selectedText, const QString &afterChar);

  void requestDefinition();
  void requestHover(const QTextCursor &cursor);
  QString getWordUnderCursor(const QTextCursor &cursor) const;

  // Code folding
  void toggleFold(const QTextBlock &blockToFold);
  void foldAll();
  void unfoldAll();
  bool isFoldable(const QTextBlock &block) const;
  bool isFolded(const QTextBlock &block) const;
  void setFolded(const QTextBlock &block, bool folded);
  int findFoldingEndBlock(const QTextBlock &startBlock) const;

signals:
  void gotoDefinitionRequested(const QString &uri, int line, int character);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;

private slots:
  void updateLineNumberAreaWidth(int newBlockCount);
  void highlightCurrentLine();
  void updateLineNumberArea(const QRect &rect, int dy);
  void toggleLineComment();
  void updateBracketMatching();
  void cursorPositionChanged();
  void handleCompletionReceived(const QJsonArray &completions);
  void handleHoverReceived(const QString &contents);
  void handleDefinitionReceived(const QString &uri, int line, int character);
  void handleDiagnosticsReceived(const QString &uri,
                                 const QJsonArray &diagnostics);
  void handleServerError(const QString &message);
  void handleTextChanged();
  void handleCursorPositionChanged();
  void handleFoldShortcut();
  void handleUnfoldShortcut();

private:
  CustomPlainTextEdit *m_editor;
  LineNumberArea *m_lineNumberArea;
  BaseHighlighter *m_highlighter;
  bool m_intelligentIndent;
  QString m_workingDirectory;

  // Search and Replace members
  SearchDialog *m_findDialog;
  QDialog *m_replaceDialog;
  BracketMatcher m_bracketMatcher;
  QuoteMatcher m_quoteMatcher;

  // LSP
  LSPClient *m_lspClient;
  QTimer *m_changeTimer;
  bool m_serverInitialized;

  // Private methods
  void setupUI();
  void setupEditor();
  void setupSearchDialogs();
  void setupBracketMatching();
  void setupLSPClient();
  QString getIndentString() const;
  int getIndentLevel(const QString &text) const;
  void updateTabWidth();
  bool isLineCommented(const QString &text) const;
  void showCompletionPopup(const QJsonArray &completions);
  void showHoverPopup(const QString &contents);
  void highlightDiagnostics(const QJsonArray &diagnostics);
  void jumpToDefinition(const QString &uri, int line, int character);
  QString getCurrentWord() const;
  QPair<int, int> getCursorPosition() const;
  QRect cursorRect() const { return m_editor->cursorRect(); }
  QWidget *viewport() const { return m_editor->viewport(); }

  // Code folding members
  QMap<int, bool> m_foldedBlocks; // Maps block number to folded state
  QMap<int, int> m_foldingRanges; // Maps start block to end block
  QMap<int, bool>
      m_hoveredFoldMarkers; // Tracks which fold markers are being hovered
  void updateFoldingRanges();
  void paintFoldingMarkers(QPainter &painter, const QTextBlock &block,
                           const QRectF &rect);
  bool isBlockVisible(const QTextBlock &block) const;
  void updateVisibleBlocks();
  void updateFoldingMarkerArea(const QRect &rect, int dy);
  QTextBlock blockAtPosition(int y) const;
  bool isFoldMarkerUnderMouse(const QPoint &pos, const QTextBlock &block) const;

  QTextEdit::ExtraSelection createBracketSelection(int position,
                                                   const QColor &color);

  friend class LineNumberArea;
  friend class CustomPlainTextEdit;
};
