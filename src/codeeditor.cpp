#include "codeeditor.h"
#include <QCheckBox>
#include <QDialog>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QShortcut>
#include <QTextBlock>
#include <QVBoxLayout>

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

void CustomPlainTextEdit::keyPressEvent(QKeyEvent *e) {
  CodeEditor *editor = qobject_cast<CodeEditor *>(parent());
  if (!editor) {
    QPlainTextEdit::keyPressEvent(e);
    return;
  }

  // Handle auto-pairing first, before any other key handling
  QString text = e->text();
  if (text.length() == 1) {
    QChar ch = text[0];
    // Check for any pairing character including quotes
    if (ch == '(' || ch == '[' || ch == '{' || ch == '"' || ch == '\'' ||
        ch == '`' || ch == ')' || ch == ']' || ch == '}') {
      if (editor->handleAutoPair(e)) {
        e->accept();
        return;
      }
    }
  }

  // Handle other special keys
  if (e->key() == Qt::Key_Tab && editor->isIntelligentIndentEnabled()) {
    if (e->modifiers() & Qt::ShiftModifier) {
      editor->handleIndent(false);
    } else {
      editor->handleIndent(true);
    }
    e->accept();
    return;
  } else if (e->key() == Qt::Key_Backtab &&
             editor->isIntelligentIndentEnabled()) {
    editor->handleIndent(false);
    e->accept();
    return;
  } else if (e->key() == Qt::Key_Return &&
             editor->isIntelligentIndentEnabled()) {
    editor->handleNewLine();
    e->accept();
    return;
  } else if (e->key() == Qt::Key_Backspace &&
             editor->isIntelligentIndentEnabled()) {
    if (editor->handleSmartBackspace()) {
      e->accept();
      return;
    }
  }

  QPlainTextEdit::keyPressEvent(e);
}

CodeEditor::CodeEditor(QWidget *parent)
    : DockWidgetBase(parent), m_intelligentIndent(true), bracketRoot(nullptr) {
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
  setupBracketMatching();
  connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this,
          &CodeEditor::cursorPositionChanged);
}

CodeEditor::~CodeEditor() {
  clearBracketTree(bracketRoot);
  delete m_findDialog;
  delete m_replaceDialog;
}

void CodeEditor::setupUI() {
  // Set up search shortcuts
  QShortcut *findShortcut = new QShortcut(QKeySequence::Find, this);
  connect(findShortcut, &QShortcut::activated, this,
          &CodeEditor::showFindDialog);

  QShortcut *findNextShortcut = new QShortcut(QKeySequence::FindNext, this);
  connect(findNextShortcut, &QShortcut::activated, this, &CodeEditor::findNext);

  QShortcut *findPrevShortcut = new QShortcut(QKeySequence::FindPrevious, this);
  connect(findPrevShortcut, &QShortcut::activated, this,
          &CodeEditor::findPrevious);

  QShortcut *replaceShortcut = new QShortcut(QKeySequence::Replace, this);
  connect(replaceShortcut, &QShortcut::activated, this,
          &CodeEditor::showReplaceDialog);

  setupSearchDialogs();
}

void CodeEditor::setupSearchDialogs() {
  // Find Dialog
  m_findDialog = new QDialog(this);
  m_findDialog->setWindowTitle(tr("Find"));
  QVBoxLayout *findLayout = new QVBoxLayout(m_findDialog);

  QHBoxLayout *findInputLayout = new QHBoxLayout;
  m_findLineEdit = new QLineEdit(m_findDialog);
  findInputLayout->addWidget(new QLabel(tr("Find:")));
  findInputLayout->addWidget(m_findLineEdit);
  findLayout->addLayout(findInputLayout);

  QHBoxLayout *findOptionsLayout = new QHBoxLayout;
  m_caseSensitiveCheckBox = new QCheckBox(tr("Case Sensitive"), m_findDialog);
  m_wholeWordsCheckBox = new QCheckBox(tr("Whole Words"), m_findDialog);
  m_regexCheckBox = new QCheckBox(tr("Regular Expression"), m_findDialog);
  findOptionsLayout->addWidget(m_caseSensitiveCheckBox);
  findOptionsLayout->addWidget(m_wholeWordsCheckBox);
  findOptionsLayout->addWidget(m_regexCheckBox);
  findLayout->addLayout(findOptionsLayout);

  QHBoxLayout *findButtonLayout = new QHBoxLayout;
  m_findNextButton = new QPushButton(tr("Find Next"), m_findDialog);
  m_findPrevButton = new QPushButton(tr("Find Previous"), m_findDialog);
  QPushButton *findCloseButton = new QPushButton(tr("Close"), m_findDialog);
  findButtonLayout->addWidget(m_findNextButton);
  findButtonLayout->addWidget(m_findPrevButton);
  findButtonLayout->addWidget(findCloseButton);
  findLayout->addLayout(findButtonLayout);

  connect(m_findNextButton, &QPushButton::clicked, this, &CodeEditor::find);
  connect(m_findPrevButton, &QPushButton::clicked, this, [this]() {
    m_searchFlags |= QTextDocument::FindBackward;
    find();
  });
  connect(findCloseButton, &QPushButton::clicked, m_findDialog, &QDialog::hide);
  connect(m_findLineEdit, &QLineEdit::textChanged, this,
          &CodeEditor::updateSearchHighlight);

  // Replace Dialog
  m_replaceDialog = new QDialog(this);
  m_replaceDialog->setWindowTitle(tr("Replace"));
  QVBoxLayout *replaceLayout = new QVBoxLayout(m_replaceDialog);

  QHBoxLayout *replaceInputLayout = new QHBoxLayout;
  m_replaceLineEdit = new QLineEdit(m_replaceDialog);
  replaceInputLayout->addWidget(new QLabel(tr("Replace with:")));
  replaceInputLayout->addWidget(m_replaceLineEdit);
  replaceLayout->addLayout(replaceInputLayout);

  QHBoxLayout *replaceButtonLayout = new QHBoxLayout;
  m_replaceButton = new QPushButton(tr("Replace"), m_replaceDialog);
  m_replaceAllButton = new QPushButton(tr("Replace All"), m_replaceDialog);
  QPushButton *replaceCloseButton =
      new QPushButton(tr("Close"), m_replaceDialog);
  replaceButtonLayout->addWidget(m_replaceButton);
  replaceButtonLayout->addWidget(m_replaceAllButton);
  replaceButtonLayout->addWidget(replaceCloseButton);
  replaceLayout->addLayout(replaceButtonLayout);

  connect(m_replaceButton, &QPushButton::clicked, this, &CodeEditor::replace);
  connect(m_replaceAllButton, &QPushButton::clicked, this,
          &CodeEditor::replaceAll);
  connect(replaceCloseButton, &QPushButton::clicked, m_replaceDialog,
          &QDialog::hide);
}

void CodeEditor::showFindDialog() {
  m_findDialog->show();
  m_findLineEdit->setFocus();
  m_findLineEdit->selectAll();
}

void CodeEditor::showReplaceDialog() {
  m_replaceDialog->show();
  if (!m_findDialog->isVisible()) {
    m_findDialog->show();
  }
  m_replaceLineEdit->setFocus();
  m_replaceLineEdit->selectAll();
}

void CodeEditor::findNext() {
  if (!m_lastSearchText.isEmpty()) {
    m_searchFlags &= ~QTextDocument::FindBackward;
    findText(m_lastSearchText, m_searchFlags);
  } else {
    showFindDialog();
  }
}

void CodeEditor::findPrevious() {
  if (!m_lastSearchText.isEmpty()) {
    m_searchFlags |= QTextDocument::FindBackward;
    findText(m_lastSearchText, m_searchFlags);
  } else {
    showFindDialog();
  }
}

void CodeEditor::find() {
  QString searchText = m_findLineEdit->text();
  if (searchText.isEmpty()) {
    return;
  }

  m_searchFlags = QTextDocument::FindFlags();
  if (m_caseSensitiveCheckBox->isChecked()) {
    m_searchFlags |= QTextDocument::FindCaseSensitively;
  }
  if (m_wholeWordsCheckBox->isChecked()) {
    m_searchFlags |= QTextDocument::FindWholeWords;
  }

  m_lastSearchText = searchText;
  if (!findText(searchText, m_searchFlags)) {
    QMessageBox::information(this, tr("Find"),
                             tr("No more occurrences found."));
  }
}

void CodeEditor::replace() {
  QTextCursor cursor = m_editor->textCursor();
  if (cursor.hasSelection() && cursor.selectedText() == m_lastSearchText) {
    cursor.insertText(m_replaceLineEdit->text());
    find(); // Find next occurrence
  } else {
    find(); // Find first occurrence
  }
}

void CodeEditor::replaceAll() {
  QTextCursor cursor = m_editor->textCursor();
  cursor.movePosition(QTextCursor::Start);
  m_editor->setTextCursor(cursor);

  int count = 0;
  while (findText(m_lastSearchText, m_searchFlags)) {
    cursor = m_editor->textCursor();
    cursor.insertText(m_replaceLineEdit->text());
    count++;
  }

  QMessageBox::information(this, tr("Replace All"),
                           tr("Replaced %1 occurrence(s).").arg(count));
}

bool CodeEditor::findText(const QString &text, QTextDocument::FindFlags flags) {
  if (m_regexCheckBox->isChecked()) {
    QRegularExpression regex(text);
    if (!regex.isValid()) {
      QMessageBox::warning(
          this, tr("Invalid Regular Expression"),
          tr("The regular expression is invalid: %1").arg(regex.errorString()));
      return false;
    }

    QRegularExpression::PatternOptions options;
    if (!(flags & QTextDocument::FindCaseSensitively)) {
      options |= QRegularExpression::CaseInsensitiveOption;
    }
    regex.setPatternOptions(options);

    QString documentText = m_editor->toPlainText();
    QTextCursor cursor = m_editor->textCursor();
    int searchFrom = cursor.position();

    QRegularExpressionMatch match;
    if (flags & QTextDocument::FindBackward) {
      // For backward search, find all matches up to cursor and take the last
      // one
      int endPos = cursor.position();
      QRegularExpressionMatchIterator it =
          regex.globalMatch(documentText.left(endPos));
      QRegularExpressionMatch lastMatch;
      while (it.hasNext()) {
        lastMatch = it.next();
      }
      match = lastMatch;
    } else {
      match = regex.match(documentText, searchFrom);
    }

    if (match.hasMatch()) {
      cursor.setPosition(match.capturedStart());
      cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
      m_editor->setTextCursor(cursor);
      return true;
    }
  } else {
    // For non-regex search, use QPlainTextEdit's built-in find
    QTextCursor cursor = m_editor->textCursor();

    // If searching backward and there's a selection, move cursor to start of
    // selection
    if (flags & QTextDocument::FindBackward && cursor.hasSelection()) {
      cursor.setPosition(cursor.selectionStart());
      m_editor->setTextCursor(cursor);
    }

    return m_editor->find(text, flags);
  }

  return false;
}

void CodeEditor::updateSearchHighlight() {
  clearSearchHighlights();

  QString searchText = m_findLineEdit->text();
  if (searchText.isEmpty()) {
    return;
  }

  QTextCharFormat format;
  format.setBackground(QColor(255, 255, 0, 100)); // Light yellow highlight

  QTextCursor cursor = m_editor->textCursor();
  int originalPosition = cursor.position();
  cursor.movePosition(QTextCursor::Start);
  m_editor->setTextCursor(cursor);

  QTextDocument::FindFlags flags = QTextDocument::FindFlags();
  if (m_caseSensitiveCheckBox->isChecked()) {
    flags |= QTextDocument::FindCaseSensitively;
  }
  if (m_wholeWordsCheckBox->isChecked()) {
    flags |= QTextDocument::FindWholeWords;
  }

  QList<QTextEdit::ExtraSelection> extraSelections;

  if (m_regexCheckBox->isChecked()) {
    QRegularExpression regex(searchText);
    if (!regex.isValid()) {
      return;
    }

    QRegularExpression::PatternOptions options =
        QRegularExpression::NoPatternOption;
    if (!(flags & QTextDocument::FindCaseSensitively)) {
      options |= QRegularExpression::CaseInsensitiveOption;
    }
    regex.setPatternOptions(options);

    QString documentText = m_editor->toPlainText();
    QRegularExpressionMatchIterator it = regex.globalMatch(documentText);

    while (it.hasNext()) {
      QRegularExpressionMatch match = it.next();
      QTextCursor matchCursor = m_editor->textCursor();
      matchCursor.setPosition(match.capturedStart());
      matchCursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);

      QTextEdit::ExtraSelection selection;
      selection.format = format;
      selection.cursor = matchCursor;
      extraSelections.append(selection);
    }
  } else {
    // Store original flags to restore after highlighting
    QTextDocument::FindFlags originalFlags = m_searchFlags;
    m_searchFlags = flags; // Use current flags for highlighting

    while (m_editor->find(searchText, flags)) {
      QTextEdit::ExtraSelection selection;
      selection.format = format;
      selection.cursor = m_editor->textCursor();
      extraSelections.append(selection);
    }

    m_searchFlags = originalFlags; // Restore original flags
  }

  // Restore original cursor position
  cursor.setPosition(originalPosition);
  m_editor->setTextCursor(cursor);
  m_editor->setExtraSelections(extraSelections);
}

void CodeEditor::clearSearchHighlights() {
  QList<QTextEdit::ExtraSelection> selections = m_editor->extraSelections();
  selections.removeIf([](const QTextEdit::ExtraSelection &selection) {
    return selection.format.background() == QColor(255, 255, 0, 100);
  });
  m_editor->setExtraSelections(selections);
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
  return space;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
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

  if (!hasSelection) {
    // Single line case - just insert or remove indentation at cursor position
    if (indent) {
      // Add indentation at cursor position
      cursor.insertText(getIndentString());
    } else {
      // Remove indentation before cursor if exists
      int currentPos = cursor.position();
      cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
      QString lineStart = cursor.selectedText();

      // Find how many spaces to remove before cursor
      int spacesToRemove = 0;
      int spaceCount = 0;
      for (int i = 0; i < lineStart.length(); i++) {
        if (lineStart[i].isSpace()) {
          spaceCount++;
          if (spaceCount % 4 == 0) {
            spacesToRemove = spaceCount;
          }
        } else {
          break;
        }
      }

      if (spacesToRemove > 0) {
        cursor.setPosition(currentPos);
        cursor.movePosition(
            QTextCursor::Left, QTextCursor::KeepAnchor,
            qMin(spacesToRemove, currentPos - cursor.block().position()));
        cursor.removeSelectedText();
      }
    }
  } else {
    // Multiple lines case - indent/unindent at the start of each line
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    QTextCursor c(m_editor->document());
    c.setPosition(start);
    int startBlock = c.blockNumber();
    c.setPosition(end);
    int endBlock = c.blockNumber();

    // Process each line in selection
    c.setPosition(start);
    c.beginEditBlock();

    for (int i = startBlock; i <= endBlock; ++i) {
      c.movePosition(QTextCursor::StartOfBlock);

      if (indent) {
        // Add indentation at start of line
        c.insertText(getIndentString());
      } else {
        // Remove indentation if exists
        QString line = c.block().text();
        if (line.startsWith(getIndentString())) {
          c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                         getIndentString().length());
          c.removeSelectedText();
        }
      }

      c.movePosition(QTextCursor::NextBlock);
      if (c.atEnd())
        break;
    }

    c.endEditBlock();
  }
}

QString CodeEditor::getIndentString() const {
  return "    "; // 4 spaces for indentation
}

int CodeEditor::getIndentLevel(const QString &text) const {
  int spaces = 0;
  for (QChar c : text) {
    if (c == ' ')
      spaces++;
    else if (c == '\t')
      spaces += 4;
    else
      break;
  }
  return spaces / 4;
}

bool CodeEditor::handleSmartBackspace() {
  QTextCursor cursor = m_editor->textCursor();

  // Only handle if no selection and at start of line or in indentation area
  if (cursor.hasSelection()) {
    return false;
  }

  // Check if we're between a pair of symbols
  if (!cursor.atStart()) {
    QTextCursor leftCursor = cursor;
    leftCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
    QString leftChar = leftCursor.selectedText();

    if (!cursor.atEnd()) {
      QTextCursor rightCursor = cursor;
      rightCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
      QString rightChar = rightCursor.selectedText();

      // Check if we're between a pair
      if ((leftChar == "(" && rightChar == ")") ||
          (leftChar == "[" && rightChar == "]") ||
          (leftChar == "{" && rightChar == "}") ||
          (leftChar == rightChar && (leftChar == "\"" || leftChar == "'" || leftChar == "`"))) {
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
        cursor.removeSelectedText();
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        cursor.removeSelectedText();
        cursor.endEditBlock();
        m_editor->setTextCursor(cursor);
        return true;
      }
    }
  }

  // Get the text from start of line to cursor
  cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
  QString lineStart = cursor.selectedText();

  // Check if we're in the indentation area
  bool onlySpaces = true;
  for (QChar c : lineStart) {
    if (!c.isSpace()) {
      onlySpaces = false;
      break;
    }
  }

  if (!onlySpaces) {
    return false;
  }

  // Calculate spaces to remove
  int spacesToRemove = lineStart.length() % 4;
  if (spacesToRemove == 0)
    spacesToRemove = 4;

  // Remove the spaces
  cursor = m_editor->textCursor();
  for (int i = 0; i < spacesToRemove; ++i) {
    cursor.deletePreviousChar();
  }

  return true;
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
    if (ch == '(' || ch == '[' || ch == '{' || ch == '"' || ch == '\'' || ch == '`') {
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
  QString selectedText = cursor.selectedText();

  // For quotes, just add the pair and place cursor between
  if (ch == '"' || ch == '\'' || ch == '`') {
    cursor.beginEditBlock();
    if (!selectedText.isEmpty()) {
      cursor.insertText(ch + selectedText + ch);
    } else {
      cursor.insertText(ch);
      cursor.insertText(ch);
      cursor.movePosition(QTextCursor::Left);
    }
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
    return true;
  }

  // For brackets, check if we should add the pair
  if (shouldAddPair(ch)) {
    QChar closingChar = getMatchingChar(ch);
    cursor.beginEditBlock();
    if (!selectedText.isEmpty()) {
      cursor.insertText(ch + selectedText + closingChar);
    } else {
      cursor.insertText(ch);
      cursor.insertText(closingChar);
      cursor.movePosition(QTextCursor::Left);
    }
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
    return true;
  }

  return false;
}

bool CodeEditor::shouldAddPair(QChar openChar) const {
  // For brackets only - check if next char is letter/number
  if (openChar != '"' && openChar != '\'' && openChar != '`') {
    QTextCursor cursor = m_editor->textCursor();
    if (!cursor.atEnd()) {
      QTextCursor nextCharCursor = cursor;
      nextCharCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
      QString nextChar = nextCharCursor.selectedText();
      if (!nextChar.isEmpty() && (nextChar[0].isLetterOrNumber() || nextChar[0] == '_')) {
        return false;
      }
    }
  }
  return true;
}

QChar CodeEditor::getMatchingChar(QChar ch) const {
  switch (ch.unicode()) {
  case '(':
    return ')';
  case '[':
    return ']';
  case '{':
    return '}';
  case '"':
    return '"';
  case '\'':
    return '\'';
  case '`':
    return '`';
  default:
    return ch;
  }
}

void CodeEditor::setupBracketMatching() {
  // Initialize bracket pairs map with VSCode's default pairs
  bracketPairs.clear();
  bracketPairs['{'] = '}';
  bracketPairs['['] = ']';
  bracketPairs['('] = ')';
  bracketPairs['"'] = '"';
  bracketPairs['\''] = '\'';
  bracketPairs['`'] = '`';

  // Initialize colors with VSCode-style colors
  bracketColors.clear();
  bracketColors = {
      QColor("#FFB200"),  // Gold
      QColor("#DA70D6"),  // Orchid
      QColor("#179FFF"),  // Light Blue
      QColor("#00B28B"),  // Turquoise
      QColor("#FF7AB2"),  // Pink
      QColor("#B48EAD")   // Purple Gray
  };

  // Initialize bracket tree
  bracketRoot = nullptr;
  updateBracketTree();

  // Connect cursor position changes to bracket matching
  connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this,
          &CodeEditor::cursorPositionChanged);
  connect(m_editor, &QPlainTextEdit::textChanged, this,
          &CodeEditor::updateBracketTree);
}

void CodeEditor::updateBracketTree() {
  clearBracketTree(bracketRoot);
  bracketRoot = nullptr;

  QString text = m_editor->toPlainText();
  QVector<BracketPair> openBrackets;
  int level = 0;
  bool inString = false;
  bool inComment = false;
  QChar stringChar;

  for (int i = 0; i < text.length(); i++) {
    QChar ch = text[i];

    // Handle line comments
    if (ch == '/' && i + 1 < text.length() && text[i + 1] == '/') {
      inComment = true;
      continue;
    }
    if (ch == '\n') {
      inComment = false;
      continue;
    }
    if (inComment)
      continue;

    // Handle string literals
    if ((ch == '"' || ch == '\'' || ch == '`') &&
        (i == 0 || text[i - 1] != '\\')) {
      if (!inString) {
        inString = true;
        stringChar = ch;
      } else if (ch == stringChar) {
        inString = false;
      }
      continue;
    }
    if (inString)
      continue;

    // Handle brackets
    if (isOpenBracket(ch)) {
      openBrackets.push_back(
          BracketPair(i, -1, level++, ch, getMatchingBracket(ch)));
    } else if (isCloseBracket(ch) && !openBrackets.isEmpty()) {
      // Find matching opening bracket
      for (int j = openBrackets.size() - 1; j >= 0; j--) {
        if (openBrackets[j].closePos == -1 && openBrackets[j].closeChar == ch) {
          openBrackets[j].closePos = i;
          bracketRoot = insertBracket(bracketRoot, openBrackets[j]);
          level = j;
          openBrackets.resize(j);
          break;
        }
      }
    }
  }

  // Mark remaining unmatched brackets as invalid
  for (const BracketPair &pair : openBrackets) {
    if (pair.closePos == -1) {
      BracketPair invalidPair = pair;
      invalidPair.isInvalid = true;
      bracketRoot = insertBracket(bracketRoot, invalidPair);
    }
  }
}

void CodeEditor::updateBracketMatching() {
  QList<QTextEdit::ExtraSelection> selections = m_editor->extraSelections();

  // Remove previous bracket highlights
  selections.removeIf([](const QTextEdit::ExtraSelection &selection) {
    return selection.format.background().color().alpha() < 255;
  });

  // Update the bracket tree with current text
  updateBracketTree();

  // Find brackets at or around cursor position
  QTextCursor cursor = m_editor->textCursor();
  int position = cursor.position();

  BracketNode *match = findMatchingBracket(position);
  if (!match && position > 0) {
    match = findMatchingBracket(position - 1);
  }

  if (match) {
    const BracketPair &pair = match->pair;
    if (pair.closePos != -1) {
      // Valid bracket pair - highlight with color based on nesting level
      QColor color = getBracketColor(pair.level);
      color.setAlpha(40); // VSCode-style transparency

      // Highlight opening bracket
      QTextEdit::ExtraSelection openSel;
      openSel.format.setBackground(color);
      openSel.format.setProperty(QTextFormat::FullWidthSelection, false);
      QTextCursor openCursor = m_editor->textCursor();
      openCursor.setPosition(pair.openPos);
      openCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
      openSel.cursor = openCursor;
      selections.append(openSel);

      // Highlight closing bracket
      QTextEdit::ExtraSelection closeSel;
      closeSel.format.setBackground(color);
      closeSel.format.setProperty(QTextFormat::FullWidthSelection, false);
      QTextCursor closeCursor = m_editor->textCursor();
      closeCursor.setPosition(pair.closePos);
      closeCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
      closeSel.cursor = closeCursor;
      selections.append(closeSel);
    } else if (pair.isInvalid) {
      // Highlight unmatched bracket in error color
      QTextEdit::ExtraSelection errorSel;
      errorSel.format.setBackground(QColor(255, 0, 0, 40));
      errorSel.format.setProperty(QTextFormat::FullWidthSelection, false);
      QTextCursor errorCursor = m_editor->textCursor();
      errorCursor.setPosition(pair.openPos);
      errorCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
      errorSel.cursor = errorCursor;
      selections.append(errorSel);
    }
  }

  m_editor->setExtraSelections(selections);
}

bool CodeEditor::isBracket(QChar ch) const {
  return isOpenBracket(ch) || isCloseBracket(ch);
}

bool CodeEditor::isOpenBracket(QChar ch) const {
  static const QString openChars = "([{\"'`";
  return openChars.contains(ch);
}

bool CodeEditor::isCloseBracket(QChar ch) const {
  static const QString closeChars = ")]}\"'`";
  return closeChars.contains(ch);
}

QChar CodeEditor::getMatchingBracket(QChar ch) const {
  if (isOpenBracket(ch)) {
    return bracketPairs[ch];
  }
  for (auto it = bracketPairs.begin(); it != bracketPairs.end(); ++it) {
    if (it.value() == ch) {
      return it.key();
    }
  }
  return ch;
}

BracketNode *CodeEditor::insertBracket(BracketNode *node,
                                       const BracketPair &pair) {
  if (!node) {
    return new BracketNode(pair);
  }

  // Use opening position as key for the AVL tree
  if (pair.openPos < node->pair.openPos) {
    node->left = insertBracket(node->left, pair);
  } else if (pair.openPos > node->pair.openPos) {
    node->right = insertBracket(node->right, pair);
  } else {
    return node; // Duplicate positions not allowed
  }

  // Update height
  node->height = 1 + qMax(getHeight(node->left), getHeight(node->right));

  // Get balance factor
  int balance = getBalance(node);

  // Left Left Case
  if (balance > 1 && pair.openPos < node->left->pair.openPos) {
    return rotateRight(node);
  }

  // Right Right Case
  if (balance < -1 && pair.openPos > node->right->pair.openPos) {
    return rotateLeft(node);
  }

  // Left Right Case
  if (balance > 1 && pair.openPos > node->left->pair.openPos) {
    node->left = rotateLeft(node->left);
    return rotateRight(node);
  }

  // Right Left Case
  if (balance < -1 && pair.openPos < node->right->pair.openPos) {
    node->right = rotateRight(node->right);
    return rotateLeft(node);
  }

  return node;
}

void CodeEditor::clearBracketTree(BracketNode *node) {
  if (node) {
    clearBracketTree(node->left);
    clearBracketTree(node->right);
    delete node;
  }
}

int CodeEditor::getHeight(BracketNode *node) const {
  if (!node)
    return 0;
  return node->height;
}

int CodeEditor::getBalance(BracketNode *node) const {
  if (!node)
    return 0;
  return getHeight(node->left) - getHeight(node->right);
}

BracketNode *CodeEditor::rotateLeft(BracketNode *x) {
  BracketNode *y = x->right;
  BracketNode *T2 = y->left;

  y->left = x;
  x->right = T2;

  x->height = 1 + qMax(getHeight(x->left), getHeight(x->right));
  y->height = 1 + qMax(getHeight(y->left), getHeight(y->right));

  return y;
}

BracketNode *CodeEditor::rotateRight(BracketNode *y) {
  BracketNode *x = y->left;
  BracketNode *T2 = x->right;

  x->right = y;
  y->left = T2;

  y->height = 1 + qMax(getHeight(y->left), getHeight(y->right));
  x->height = 1 + qMax(getHeight(x->left), getHeight(x->right));

  return x;
}

BracketNode *CodeEditor::findMatchingBracket(int position) const {
  BracketNode *current = bracketRoot;
  while (current) {
    if (position == current->pair.openPos ||
        position == current->pair.closePos) {
      return current;
    }
    if (position < current->pair.openPos) {
      current = current->left;
    } else {
      current = current->right;
    }
  }
  return nullptr;
}

QColor CodeEditor::getBracketColor(int level) const {
  if (bracketColors.isEmpty())
    return QColor(Qt::white);
  return bracketColors[level % bracketColors.size()];
}

void CodeEditor::cursorPositionChanged() { updateBracketMatching(); }
