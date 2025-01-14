#pragma once
#include "cpphighlighter.h"
#include "dockwidgetbase.h"
#include <QMap>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QVector>

class LineNumberArea;
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

// Custom editor class to access protected members
class CustomPlainTextEdit : public QPlainTextEdit {
  Q_OBJECT
public:
  explicit CustomPlainTextEdit(QWidget *parent = nullptr)
      : QPlainTextEdit(parent) {}
  QTextBlock firstVisibleBlock() const {
    return QPlainTextEdit::firstVisibleBlock();
  }
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
  bool hasUnsavedChanges() override {
    return m_editor->document()->isModified();
  }

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
  void updateBracketMatching();
  void cursorPositionChanged();

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

  // Bracket handling
  QMap<QChar, QChar> bracketPairs;
  QVector<QColor> bracketColors;
  BracketNode *bracketRoot;

  // VSCode-style bracket tree methods
  void setupBracketMatching();
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

  // Bracket utilities
  bool isBracket(QChar ch) const;
  bool isOpenBracket(QChar ch) const;
  bool isCloseBracket(QChar ch) const;
  QChar getMatchingBracket(QChar ch) const;

  void setupUI();
  void setupEditor();
  void setupSearchDialogs();
  QString getIndentString() const;
  int getIndentLevel(const QString &text) const;
  bool findText(const QString &text, QTextDocument::FindFlags flags);
  void clearSearchHighlights();
  void updateTabWidth();

  // Auto-pairing methods
  bool handleKeyPress(QKeyEvent *e);
  bool handleAutoPair(QKeyEvent *e);
  bool shouldAddPair(QChar openChar) const;
  QChar getMatchingChar(QChar ch) const;
  bool isInsideString() const;
  bool isInsideComment() const;

  friend class LineNumberArea;
  friend class CustomPlainTextEdit;
};
