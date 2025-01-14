#include "codeeditor/codeeditor.h"
#include "customtextedit.h"
#include "highlighters/cpphighlighter.h"
#include "search.h"
#include "settings/shortcutmanager.h"
#include <QAbstractItemView>
#include <QCheckBox>
#include <QCompleter>
#include <QDialog>
#include <QDir>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QShortcut>
#include <QTextBlock>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>

CodeEditor::CodeEditor(QWidget *parent)
    : DockWidgetBase(parent), m_intelligentIndent(true),
      m_lspClient(new LSPClient(this)), m_serverInitialized(false) {
  m_editor = new CustomPlainTextEdit(this);
  m_lineNumberArea = new LineNumberArea(this);
  m_highlighter = new CppHighlighter(m_editor->document());

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_editor);

  // Initialize dialogs to nullptr
  m_findDialog = nullptr;
  m_replaceDialog = nullptr;

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
  m_editor->setLineWrapMode(wordWrap ? QPlainTextEdit::WidgetWidth
                                     : QPlainTextEdit::NoWrap);

  // Set up line number area
  connect(m_editor, &QPlainTextEdit::blockCountChanged, this,
          &CodeEditor::updateLineNumberAreaWidth);
  connect(m_editor, &QPlainTextEdit::updateRequest, this,
          &CodeEditor::updateLineNumberArea);
  connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this,
          &CodeEditor::highlightCurrentLine);

  updateLineNumberAreaWidth(0);
  highlightCurrentLine();

  // Set up shortcuts
  QShortcut *commentShortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this);
  connect(commentShortcut, &QShortcut::activated, this,
          &CodeEditor::toggleLineComment);

  // Set scrollbar policy
  m_editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  setupUI();
  setupSearchDialogs();
  setupBracketMatching();
  connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this,
          &CodeEditor::cursorPositionChanged);

  // Set up LSP client
  setupLSPClient();

  // Set up change timer for LSP updates
  m_changeTimer = new QTimer(this);
  m_changeTimer->setSingleShot(true);
  m_changeTimer->setInterval(500); // 500ms delay
  connect(m_changeTimer, &QTimer::timeout, this,
          &CodeEditor::handleTextChanged);

  // Connect text change signals
  connect(m_editor->document(), &QTextDocument::contentsChanged, m_changeTimer,
          QOverload<>::of(&QTimer::start));
  connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this,
          &CodeEditor::handleCursorPositionChanged);

  // Add keyboard shortcuts for folding
  QShortcut *foldShortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketLeft), this);
  connect(foldShortcut, &QShortcut::activated, this,
          &CodeEditor::handleFoldShortcut);

  QShortcut *unfoldShortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketRight), this);
  connect(unfoldShortcut, &QShortcut::activated, this,
          &CodeEditor::handleUnfoldShortcut);

  QShortcut *foldAllShortcut = new QShortcut(
      QKeySequence((Qt::CTRL | Qt::SHIFT) | Qt::Key_BracketLeft), this);
  connect(foldAllShortcut, &QShortcut::activated, this, &CodeEditor::foldAll);

  QShortcut *unfoldAllShortcut = new QShortcut(
      QKeySequence((Qt::CTRL | Qt::SHIFT) | Qt::Key_BracketRight), this);
  connect(unfoldAllShortcut, &QShortcut::activated, this,
          &CodeEditor::unfoldAll);

  // Connect to line number area mouse press
  connect(m_lineNumberArea, &LineNumberArea::mousePressed, this,
          [this](QPoint pos) {
            QTextBlock block = blockAtPosition(pos.y());
            if (block.isValid() && isFoldable(block)) {
              toggleFold(block);
            }
          });

  // Add search shortcuts
  QShortcut *findShortcut = new QShortcut(QKeySequence::Find, this);
  connect(findShortcut, &QShortcut::activated, this,
          &CodeEditor::showFindDialog);

  QShortcut *findNextShortcut = new QShortcut(QKeySequence(Qt::Key_F3), this);
  connect(findNextShortcut, &QShortcut::activated, this, &CodeEditor::findNext);

  QShortcut *findPrevShortcut =
      new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F3), this);
  connect(findPrevShortcut, &QShortcut::activated, this,
          &CodeEditor::findPrevious);
}

CodeEditor::~CodeEditor() {
  delete m_findDialog;
  delete m_replaceDialog;
  if (m_lspClient) {
    m_lspClient->stopServer();
  }
}

void CodeEditor::setupUI() {
  auto &shortcutMgr = ShortcutManager::instance();

  // Register editor-specific shortcuts
  shortcutMgr.registerShortcut("editor.find", QKeySequence::Find, nullptr,
                               tr("Find in editor"));
  shortcutMgr.registerShortcut("editor.findNext", QKeySequence(Qt::Key_F3),
                               nullptr, tr("Find next occurrence"));
  shortcutMgr.registerShortcut("editor.findPrev",
                               QKeySequence(Qt::SHIFT | Qt::Key_F3), nullptr,
                               tr("Find previous occurrence"));

  // Create shortcuts using the registered sequences
  new QShortcut(shortcutMgr.getShortcut("editor.find"), this,
                [this]() { showFindDialog(); });
  new QShortcut(shortcutMgr.getShortcut("editor.findNext"), this,
                [this]() { findNext(); });
  new QShortcut(shortcutMgr.getShortcut("editor.findPrev"), this,
                [this]() { findPrevious(); });
}

void CodeEditor::setupSearchDialogs() {
  if (!m_findDialog) {
    m_findDialog = new SearchDialog(m_editor, this);
    m_findDialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
  }
}

void CodeEditor::showFindDialog() {
  if (m_findDialog) {
    m_findDialog->showFind();
    m_findDialog->raise();
    m_findDialog->activateWindow();
  }
}

void CodeEditor::showReplaceDialog() {
  if (m_findDialog) {
    m_findDialog->showReplace();
    m_findDialog->raise();
    m_findDialog->activateWindow();
  }
}

void CodeEditor::findNext() {
  if (m_findDialog) {
    m_findDialog->findNext();
  }
}

void CodeEditor::findPrevious() {
  if (m_findDialog) {
    m_findDialog->findPrevious();
  }
}

int CodeEditor::lineNumberAreaWidth() const {
  int digits = 1;
  int max = qMax(1, m_editor->document()->blockCount());
  while (max >= 10) {
    max /= 10;
    ++digits;
  }

  int space =
      3 + m_editor->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
  return space + 15; // Add extra space for folding markers
}

void CodeEditor::updateLineNumberAreaWidth(int newBlockCount) {
  m_editor->setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
  if (dy)
    m_lineNumberArea->scroll(0, dy);
  else
    m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(),
                             rect.height());

  if (rect.contains(m_editor->viewport()->rect()))
    updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
  DockWidgetBase::resizeEvent(e);
  QRect cr = m_editor->contentsRect();
  m_lineNumberArea->setGeometry(
      QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine() {
  QList<QTextEdit::ExtraSelection> extraSelections;

  if (!m_editor->isReadOnly()) {
    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor(45, 45, 45); // Dark gray color
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
  int top = qRound(m_editor->blockBoundingGeometry(block)
                       .translated(m_editor->contentOffset())
                       .top());
  int bottom = top + qRound(m_editor->blockBoundingRect(block).height());

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      QString number = QString::number(blockNumber + 1);
      painter.setPen(Qt::black);
      painter.drawText(0, top, m_lineNumberArea->width(),
                       m_editor->fontMetrics().height(), Qt::AlignRight,
                       number);

      // Draw folding marker if block is foldable
      if (isFoldable(block)) {
        QRectF blockRect(0, top, m_lineNumberArea->width(),
                         m_editor->fontMetrics().height());
        paintFoldingMarkers(painter, block, blockRect);
      }
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
                               tr("This document has unsaved changes. Do you "
                                  "want to close it anyway?"),
                               QMessageBox::Yes | QMessageBox::No) ==
             QMessageBox::Yes;
}

void CodeEditor::updateTheme() {
  // This will be implemented when we add theme support
  // For now, we can set some default colors
  QPalette p = m_editor->palette();
  p.setColor(QPalette::Base, QColor("#1E1E1E"));
  p.setColor(QPalette::Text, QColor("#D4D4D4"));
  m_editor->setPalette(p);
}

void CodeEditor::handleIndent(bool indent) {
  QTextCursor cursor = m_editor->textCursor();

  // Get the selected text
  QString selectedText = cursor.selectedText();
  bool hasSelection = !selectedText.isEmpty();

  cursor.beginEditBlock();

  if (!hasSelection) {
    // Single line case
    if (indent) {
      // Add indentation at cursor position
      cursor.insertText(getIndentString());
    } else {
      // Remove one level of indentation from start of line
      cursor.movePosition(QTextCursor::StartOfLine);
      QTextCursor testCursor = cursor;
      testCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 4);
      QString possibleIndent = testCursor.selectedText();

      // Only remove if it's spaces at the start of line
      if (possibleIndent == getIndentString()) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 4);
        cursor.removeSelectedText();
      }
    }
  } else {
    // Multiple lines case
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    QTextCursor c(m_editor->document());
    c.setPosition(start);
    int startBlock = c.blockNumber();
    c.setPosition(end);
    int endBlock = c.blockNumber();

    // Process each line in selection
    c.setPosition(start);
    for (int i = startBlock; i <= endBlock; ++i) {
      c.movePosition(QTextCursor::StartOfBlock);

      if (indent) {
        // Add indentation at start of line
        c.insertText(getIndentString());
      } else {
        // Remove one level of indentation if it exists at start of line
        QTextCursor testCursor = c;
        testCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 4);
        QString possibleIndent = testCursor.selectedText();

        if (possibleIndent == getIndentString()) {
          c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 4);
          c.removeSelectedText();
        }
      }

      c.movePosition(QTextCursor::NextBlock);
      if (c.atEnd() && i < endBlock)
        break;
    }
  }

  cursor.endEditBlock();
}

QString CodeEditor::getIndentString() const {
  return "    "; // 4 spaces for indentation
}

int CodeEditor::getIndentLevel(const QString &text) const {
  return m_folding.getIndentLevel(text);
}

bool CodeEditor::handleSmartBackspace() {
  QTextCursor cursor = m_editor->textCursor();
  if (cursor.hasSelection())
    return false;

  QString beforeChar = m_quoteMatcher.getCharBefore(cursor);
  QString afterChar = m_quoteMatcher.getCharAfter(cursor);

  if (beforeChar.isEmpty() || afterChar.isEmpty())
    return false;

  // Check if we're between matching pairs
  bool isQuotePair =
      m_quoteMatcher.isQuoteChar(beforeChar[0]) && beforeChar == afterChar;
  bool isBracketPair =
      m_bracketMatcher.isMatchingPair(beforeChar[0], afterChar[0]);

  if (isQuotePair || isBracketPair) {
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
    return true;
  }

  return false;
}

void CodeEditor::handleNewLine() {
  QTextCursor cursor = m_editor->textCursor();

  // Get the current line's text
  cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
  QString currentLine = cursor.selectedText();

  // Calculate indentation level
  int indentLevel = getIndentLevel(currentLine);

  // Check if we need to increase indent (e.g., after '{')
  if (currentLine.trimmed().endsWith('{')) {
    indentLevel++;
  }

  // Insert newline and indentation
  cursor = m_editor->textCursor();
  cursor.insertText("\n" + getIndentString().repeated(indentLevel));
}

void CodeEditor::setFont(const QFont &font) {
  m_editor->setFont(font);
  updateTabWidth();
}

void CodeEditor::updateTabWidth() {
  QFontMetrics metrics(m_editor->font());
  m_editor->setTabStopDistance(4 * metrics.horizontalAdvance(' '));
}

bool CodeEditor::handleKeyPress(QKeyEvent *e) {
  // Handle auto-pairing
  if (e->text().length() == 1) {
    QChar ch = e->text().at(0);
    if (ch == '(' || ch == '[' || ch == '{' || ch == '<' || ch == '"' ||
        ch == '\'' || ch == '`' || ch == ')' || ch == ']' || ch == '}' ||
        ch == '>' || ch == '"' || ch == '\'' || ch == '`') {
      return handleAutoPair(e);
    }
  }
  return false;
}

bool CodeEditor::handleAutoPair(QKeyEvent *e) {
  QString text = e->text();
  if (text.isEmpty())
    return false;

  QChar ch = text[0];
  QTextCursor cursor = m_editor->textCursor();

  // Handle quotes using QuoteMatcher
  if (m_quoteMatcher.isQuoteChar(ch)) {
    bool isMarkdown = workingDirectory().endsWith(".md");
    return m_quoteMatcher.handleQuoteChar(ch, cursor, isMarkdown);
  }

  // Handle brackets using BracketMatcher
  if (m_bracketMatcher.isOpenBracket(ch) ||
      m_bracketMatcher.isCloseBracket(ch)) {
    return handleBracketPair(ch, cursor, cursor.selectedText(),
                             m_bracketMatcher.getCharAfter(cursor));
  }

  return false;
}

void CodeEditor::setupBracketMatching() {
  // Connect cursor position changes to bracket matching
  connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this,
          &CodeEditor::cursorPositionChanged);
  connect(m_editor, &QPlainTextEdit::textChanged, this,
          &CodeEditor::updateBracketMatching);
}

void CodeEditor::updateBracketMatching() {
  QList<QTextEdit::ExtraSelection> selections = m_editor->extraSelections();

  // Remove previous bracket highlights
  selections.removeIf([](const QTextEdit::ExtraSelection &selection) {
    return selection.format.background().color().alpha() < 255;
  });

  // Update the bracket tree
  m_bracketMatcher.updateBracketTree(m_editor->toPlainText());

  // Find brackets at cursor position
  QTextCursor cursor = m_editor->textCursor();
  int position = cursor.position();

  BracketNode *match = m_bracketMatcher.findMatchingBracket(position);
  if (!match && position > 0) {
    match = m_bracketMatcher.findMatchingBracket(position - 1);
  }

  if (match) {
    const BracketPair &pair = match->pair;
    if (pair.closePos != -1) {
      QColor color = m_bracketMatcher.getBracketColor(pair.level);
      color.setAlpha(40);

      selections.append(createBracketSelection(pair.openPos, color));
      selections.append(createBracketSelection(pair.closePos, color));
    } else if (pair.isInvalid) {
      selections.append(
          createBracketSelection(pair.openPos, QColor(255, 0, 0, 40)));
    }
  }

  m_editor->setExtraSelections(selections);
}

void CodeEditor::cursorPositionChanged() { updateBracketMatching(); }

void CodeEditor::setSyntaxHighlighting(bool enabled) {
  if (m_highlighter) {
    m_highlighter->setEnabled(enabled);
  }
}

bool CodeEditor::isSyntaxHighlightingEnabled() const {
  return m_highlighter && m_highlighter->isEnabled();
}

void CodeEditor::setupLSPClient() {
  // Connect LSP client signals
  connect(m_lspClient, &LSPClient::initialized,
          [this]() { m_serverInitialized = true; });
  connect(m_lspClient, &LSPClient::completionReceived, this,
          &CodeEditor::handleCompletionReceived);
  connect(m_lspClient, &LSPClient::hoverReceived, this,
          &CodeEditor::handleHoverReceived);
  connect(m_lspClient, &LSPClient::definitionReceived, this,
          &CodeEditor::handleDefinitionReceived);
  connect(m_lspClient, &LSPClient::diagnosticsReceived, this,
          &CodeEditor::handleDiagnosticsReceived);
  connect(m_lspClient, &LSPClient::serverError, this,
          &CodeEditor::handleServerError);

  // Start clangd server
  if (m_lspClient->startServer("clangd")) {
    m_lspClient->initialize(workingDirectory());
  }
}

void CodeEditor::handleTextChanged() {
  if (!m_serverInitialized)
    return;

  QString uri =
      m_lspClient->uriFromPath(workingDirectory() + "/" + windowTitle());
  m_lspClient->didChange(uri, m_editor->toPlainText());
}

void CodeEditor::handleCursorPositionChanged() {
  if (!m_serverInitialized)
    return;

  QTextCursor cursor = m_editor->textCursor();
  int line = cursor.blockNumber();
  int character = cursor.positionInBlock();
  QString uri =
      m_lspClient->uriFromPath(workingDirectory() + "/" + windowTitle());

  // Request hover information
  m_lspClient->requestHover(uri, line, character);

  // Request completion if typing
  QString currentWord = getCurrentWord();
  if (!currentWord.isEmpty()) {
    m_lspClient->requestCompletion(uri, line, character);
  }
}

void CodeEditor::handleCompletionReceived(const QJsonArray &completions) {
  QStringList suggestions;
  for (const auto &val : completions) {
    if (val.isObject()) {
      QJsonObject item = val.toObject();
      suggestions << item["label"].toString();
    }
  }

  if (suggestions.isEmpty())
    return;

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

void CodeEditor::handleHoverReceived(const QString &contents) {
  if (contents.isEmpty())
    return;

  QRect rect = cursorRect();
  QPoint pos = viewport()->mapToGlobal(QPoint(rect.left(), rect.bottom()));
  QToolTip::showText(pos, contents);
}

void CodeEditor::handleDefinitionReceived(const QString &uri, int line,
                                          int character) {
  emit gotoDefinitionRequested(uri, line, character);
}

void CodeEditor::handleDiagnosticsReceived(const QString &uri,
                                           const QJsonArray &diagnostics) {
  QList<QTextEdit::ExtraSelection> selections;

  // Keep existing selections that are not diagnostic highlights
  for (const auto &selection : m_editor->extraSelections()) {
    if (selection.format.background().color().alpha() == 255) {
      selections.append(selection);
    }
  }

  for (const auto &val : diagnostics) {
    if (!val.isObject())
      continue;

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
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
                        start["line"].toInt());
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                        start["character"].toInt());
    int startPos = cursor.position();

    // Move to end position
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
                        end["line"].toInt());
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                        end["character"].toInt());
    int endPos = cursor.position();

    // Create selection
    QTextEdit::ExtraSelection selection;
    selection.cursor = QTextCursor(m_editor->document());
    selection.cursor.setPosition(startPos);
    selection.cursor.setPosition(endPos, QTextCursor::KeepAnchor);

    // Set format based on severity
    QColor underlineColor;
    if (severity == "1") {                // Error
      underlineColor = QColor("#FF0000"); // Red
    } else if (severity == "2") {         // Warning
      underlineColor = QColor("#FFA500"); // Orange
    } else if (severity == "3") {         // Information
      underlineColor = QColor("#2196F3"); // Blue
    } else if (severity == "4") {         // Hint
      underlineColor = QColor("#4CAF50"); // Green
    } else {
      underlineColor = QColor("#FF0000"); // Default to red
    }

    // Create squiggly underline effect
    QTextCharFormat format;
    format.setUnderlineColor(underlineColor);
    format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    format.setToolTip(message); // Show diagnostic message on hover

    selection.format = format;
    selections.append(selection);
  }
  m_editor->setExtraSelections(selections);
}

void CodeEditor::handleServerError(const QString &message) {
  qDebug() << "LSP Server Error:" << message;
}

QString CodeEditor::getCurrentWord() const {
  QTextCursor cursor = m_editor->textCursor();
  cursor.select(QTextCursor::WordUnderCursor);
  return cursor.selectedText();
}

QPair<int, int> CodeEditor::getCursorPosition() const {
  QTextCursor cursor = m_editor->textCursor();
  return qMakePair(cursor.blockNumber(), cursor.positionInBlock());
}

void CodeEditor::requestDefinition() {
  if (!m_serverInitialized)
    return;

  QTextCursor cursor = m_editor->textCursor();
  int line = cursor.blockNumber();
  int character = cursor.positionInBlock();
  QString uri =
      m_lspClient->uriFromPath(workingDirectory() + "/" + windowTitle());

  m_lspClient->requestDefinition(uri, line, character);
}

void CodeEditor::requestHover(const QTextCursor &cursor) {
  if (!m_serverInitialized)
    return;

  int line = cursor.blockNumber();
  int character = cursor.positionInBlock();
  QString uri =
      m_lspClient->uriFromPath(workingDirectory() + "/" + windowTitle());

  m_lspClient->requestHover(uri, line, character);
}

QString CodeEditor::getWordUnderCursor(const QTextCursor &cursor) const {
  QTextCursor wordCursor = cursor;
  wordCursor.select(QTextCursor::WordUnderCursor);
  return wordCursor.selectedText();
}

void CodeEditor::toggleFold(const QTextBlock &blockToFold) {
  m_folding.toggleFold(blockToFold);
  updateVisibleBlocks();
  m_editor->viewport()->update();
  m_lineNumberArea->update();
}

void CodeEditor::foldAll() {
  m_folding.foldAll(m_editor->document());
  updateVisibleBlocks();
  viewport()->update();
}

void CodeEditor::unfoldAll() {
  m_folding.unfoldAll();
  updateVisibleBlocks();
  viewport()->update();
}

bool CodeEditor::isFoldable(const QTextBlock &block) const {
  return m_folding.isFoldable(block);
}

bool CodeEditor::isFolded(const QTextBlock &block) const {
  return m_folding.isFolded(block);
}

void CodeEditor::setFolded(const QTextBlock &block, bool folded) {
  m_folding.setFolded(block, folded);
}

int CodeEditor::findFoldingEndBlock(const QTextBlock &startBlock) const {
  return m_folding.findFoldingEndBlock(startBlock);
}

bool CodeEditor::isBlockVisible(const QTextBlock &block) const {
  return m_folding.isBlockVisible(block);
}

void CodeEditor::updateVisibleBlocks() {
  QTextBlock block = document()->firstBlock();
  while (block.isValid()) {
    block.setVisible(isBlockVisible(block));
    block = block.next();
  }

  // Update line number area and viewport
  m_editor->viewport()->update();
  m_lineNumberArea->update();
  updateLineNumberAreaWidth(0);
}

void CodeEditor::updateFoldingMarkerArea(const QRect &rect, int dy) {
  if (dy) {
    viewport()->scroll(0, dy);
  } else {
    viewport()->update(0, rect.y(), viewport()->width(), rect.height());
  }
}

void CodeEditor::paintFoldingMarkers(QPainter &painter, const QTextBlock &block,
                                     const QRectF &rect) {
  m_folding.paintFoldingMarkers(
      painter, block, rect, isFolded(block),
      m_hoveredFoldMarkers.value(block.blockNumber(), false),
      lineNumberAreaWidth());
}

bool CodeEditor::isFoldMarkerUnderMouse(const QPoint &pos,
                                        const QTextBlock &block) const {
  int top = m_editor->blockBoundingGeometry(block)
                .translated(m_editor->contentOffset())
                .top();
  int height = m_editor->blockBoundingRect(block).height();

  return m_folding.isFoldMarkerUnderMouse(pos, block, top, height,
                                          lineNumberAreaWidth());
}

void CodeEditor::mouseMoveEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  bool needsUpdate = false;

  // Clear previous hover states if mouse is outside the folding marker area
  if (pos.x() > lineNumberAreaWidth()) {
    if (!m_hoveredFoldMarkers.isEmpty()) {
      m_hoveredFoldMarkers.clear();
      m_lineNumberArea->update();
    }
    DockWidgetBase::mouseMoveEvent(event);
    return;
  }

  // Update hover states for visible blocks
  QTextBlock block = m_editor->firstVisibleBlock();
  while (block.isValid()) {
    int blockNumber = block.blockNumber();
    bool wasHovered = m_hoveredFoldMarkers.value(blockNumber, false);
    bool isHovered = isFoldMarkerUnderMouse(pos, block);

    if (wasHovered != isHovered) {
      if (isHovered) {
        m_hoveredFoldMarkers[blockNumber] = true;
      } else {
        m_hoveredFoldMarkers.remove(blockNumber);
      }
      needsUpdate = true;
    }

    block = block.next();
    if (!block.isVisible())
      continue;

    // Stop if we're past the visible area
    int bottom = m_editor->blockBoundingGeometry(block)
                     .translated(m_editor->contentOffset())
                     .bottom();
    if (bottom > m_editor->viewport()->height())
      break;
  }

  if (needsUpdate) {
    m_lineNumberArea->update();
  }

  DockWidgetBase::mouseMoveEvent(event);
}

void CodeEditor::leaveEvent(QEvent *event) {
  // Clear all hover states when mouse leaves the widget
  if (!m_hoveredFoldMarkers.isEmpty()) {
    m_hoveredFoldMarkers.clear();
    m_lineNumberArea->update();
  }
  DockWidgetBase::leaveEvent(event);
}

void CodeEditor::handleFoldShortcut() {
  toggleFold(m_editor->textCursor().block());
}

void CodeEditor::handleUnfoldShortcut() {
  toggleFold(m_editor->textCursor().block());
}

QTextBlock CodeEditor::blockAtPosition(int y) const {
  QTextBlock block = m_editor->firstVisibleBlock();
  int top = m_editor->blockBoundingGeometry(block)
                .translated(m_editor->contentOffset())
                .top();
  int bottom = top + m_editor->blockBoundingRect(block).height();

  do {
    if (block.isValid() && top <= y && y <= bottom) {
      return block;
    }

    block = block.next();
    if (!block.isValid())
      break;

    top = bottom;
    bottom = top + m_editor->blockBoundingRect(block).height();
  } while (block.isValid() && top <= y);

  return QTextBlock();
}

QTextEdit::ExtraSelection
CodeEditor::createBracketSelection(int position, const QColor &color) {
  QTextEdit::ExtraSelection selection;
  selection.format.setBackground(color);
  selection.format.setProperty(QTextFormat::FullWidthSelection, false);
  QTextCursor cursor = m_editor->textCursor();
  cursor.setPosition(position);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
  selection.cursor = cursor;
  return selection;
}

bool CodeEditor::handleBracketPair(QChar ch, QTextCursor &cursor,
                                   const QString &selectedText,
                                   const QString &afterChar) {
  if (m_quoteMatcher.isInsideString(cursor) ||
      m_bracketMatcher.isInsideComment(cursor)) {
    return false;
  }

  // Handle selected text
  if (!selectedText.isEmpty()) {
    QChar closingChar = m_bracketMatcher.getMatchingBracket(ch);
    cursor.beginEditBlock();
    cursor.insertText(ch + selectedText + closingChar);
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
    return true;
  }

  // Handle auto-pairing
  if (!afterChar.isEmpty() && afterChar[0].isLetterOrNumber()) {
    return false;
  }

  QChar closingChar = m_bracketMatcher.getMatchingBracket(ch);
  cursor.beginEditBlock();
  cursor.insertText(ch);
  cursor.insertText(closingChar);
  cursor.movePosition(QTextCursor::Left);
  cursor.endEditBlock();
  m_editor->setTextCursor(cursor);
  return true;
}

void CodeEditor::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    QPoint pos = event->pos();
    if (pos.x() <= lineNumberAreaWidth()) {
      // Click in line number area
      QTextBlock block = blockAtPosition(pos.y());
      if (block.isValid() && isFoldable(block)) {
        if (isFoldMarkerUnderMouse(pos, block)) {
          toggleFold(block);
          event->accept();
          return;
        }
      }
    }
  }
  DockWidgetBase::mousePressEvent(event);
}

#include "moc_codeeditor.cpp"
