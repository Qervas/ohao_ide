#pragma once
#include "../highlighters/basehighlighter.h"
#include "dockwidgetbase.h"
#include "lsp/lspclient.h"
#include "linenumberarea.h"
#include "customtextedit.h"
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

// Structure to represent a bracket pair
struct BracketPair {
  int openPos;     // Position of opening bracket
  int closePos;    // Position of closing bracket
  int level;       // Nesting level
  QChar openChar;  // Opening bracket character
  QChar closeChar; // Closing bracket character
  bool isInvalid;  // Whether this is an unmatched bracket

  BracketPair(int open = -1, int close = -1, int lvl = 0, QChar oc = '{',
              QChar cc = '}')
      : openPos(open), closePos(close), level(lvl), openChar(oc), closeChar(cc),
        isInvalid(false) {}
};

// AVL Tree node for bracket matching
class BracketNode {
public:
  BracketPair pair;
  int height;
  BracketNode *left;
  BracketNode *right;

  BracketNode(const BracketPair &p)
      : pair(p), height(1), left(nullptr), right(nullptr) {}
};

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
  bool hasUnsavedChanges() override { return m_editor->document()->isModified(); }
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
  void setLineWrapMode(QPlainTextEdit::LineWrapMode mode) { m_editor->setLineWrapMode(mode); }
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
  void find();
  void replace();
  void replaceAll();
  void updateSearchHighlight();
  void updateBracketMatching();
  void cursorPositionChanged();
  void handleCompletionReceived(const QJsonArray &completions);
  void handleHoverReceived(const QString &contents);
  void handleDefinitionReceived(const QString &uri, int line, int character);
  void handleDiagnosticsReceived(const QString &uri, const QJsonArray &diagnostics);
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

  // Bracket handling
  QMap<QChar, QChar> bracketPairs;
  QVector<QColor> bracketColors;
  BracketNode *bracketRoot;

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
  bool findText(const QString &text, QTextDocument::FindFlags flags);
  void clearSearchHighlights();
  void updateTabWidth();
  bool shouldAddPair(QChar openChar) const;
  QChar getMatchingChar(QChar ch) const;
  bool isInsideString() const;
  bool isInsideComment() const;
  bool isLineCommented(const QString &text) const;
  void showCompletionPopup(const QJsonArray &completions);
  void showHoverPopup(const QString &contents);
  void highlightDiagnostics(const QJsonArray &diagnostics);
  void jumpToDefinition(const QString &uri, int line, int character);
  QString getCurrentWord() const;
  QPair<int, int> getCursorPosition() const;
  QRect cursorRect() const { return m_editor->cursorRect(); }
  QWidget* viewport() const { return m_editor->viewport(); }
  QChar getStringQuoteChar() const;

  // Bracket tree methods
  void initializeBracketColors();
  void updateBracketTree();
  BracketNode *insertBracket(BracketNode *node, const BracketPair &pair);
  void clearBracketTree(BracketNode *node);
  int getHeight(BracketNode *node) const;
  int getBalance(BracketNode *node) const;
  BracketNode *rotateLeft(BracketNode *x);
  BracketNode *rotateRight(BracketNode *y);
  BracketNode *findMatchingBracket(int position) const;
  void highlightMatchingBrackets(const BracketPair &pair);
  QColor getBracketColor(int level) const;
  bool isBracket(QChar ch) const;
  bool isOpenBracket(QChar ch) const;
  bool isCloseBracket(QChar ch) const;
  QChar getMatchingBracket(QChar ch) const;

  // Code folding members
  QMap<int, bool> m_foldedBlocks;  // Maps block number to folded state
  QMap<int, int> m_foldingRanges;  // Maps start block to end block
  QMap<int, bool> m_hoveredFoldMarkers;  // Tracks which fold markers are being hovered
  void updateFoldingRanges();
  void paintFoldingMarkers(QPainter &painter, const QTextBlock &block, const QRectF &rect);
  bool isBlockVisible(const QTextBlock &block) const;
  void updateVisibleBlocks();
  void updateFoldingMarkerArea(const QRect &rect, int dy);
  QTextBlock blockAtPosition(int y) const;
  bool isFoldMarkerUnderMouse(const QPoint &pos, const QTextBlock &block) const;

  friend class LineNumberArea;
  friend class CustomPlainTextEdit;
};
