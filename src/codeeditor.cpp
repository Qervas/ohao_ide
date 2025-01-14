#include "codeeditor.h"
#include "highlighters/cpphighlighter.h"
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
#include <QTimer>
#include <QCompleter>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QToolTip>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QDir>

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

CodeEditor::CodeEditor(QWidget *parent)
    : DockWidgetBase(parent), m_intelligentIndent(true), bracketRoot(nullptr),
      m_lspClient(new LSPClient(this)), m_serverInitialized(false) {
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

  // Set up LSP client
  setupLSPClient();

  // Set up change timer for LSP updates
  m_changeTimer = new QTimer(this);
  m_changeTimer->setSingleShot(true);
  m_changeTimer->setInterval(500); // 500ms delay
  connect(m_changeTimer, &QTimer::timeout, this, &CodeEditor::handleTextChanged);

  // Connect text change signals
  connect(m_editor->document(), &QTextDocument::contentsChanged,
          m_changeTimer, QOverload<>::of(&QTimer::start));
  connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
          this, &CodeEditor::handleCursorPositionChanged);

  // Add keyboard shortcuts for folding
  QShortcut *foldShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketLeft), this);
  connect(foldShortcut, &QShortcut::activated, this, &CodeEditor::handleFoldShortcut);
  
  QShortcut *unfoldShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketRight), this);
  connect(unfoldShortcut, &QShortcut::activated, this, &CodeEditor::handleUnfoldShortcut);
  
  QShortcut *foldAllShortcut = new QShortcut(QKeySequence((Qt::CTRL | Qt::SHIFT) | Qt::Key_BracketLeft), this);
  connect(foldAllShortcut, &QShortcut::activated, this, &CodeEditor::foldAll);
  
  QShortcut *unfoldAllShortcut = new QShortcut(QKeySequence((Qt::CTRL | Qt::SHIFT) | Qt::Key_BracketRight), this);
  connect(unfoldAllShortcut, &QShortcut::activated, this, &CodeEditor::unfoldAll);
  
  // Connect to line number area mouse press
  connect(m_lineNumberArea, &LineNumberArea::mousePressed, this, [this](QPoint pos) {
      QTextBlock block = blockAtPosition(pos.y());
      if (block.isValid() && isFoldable(block)) {
          toggleFold(block);
      }
  });
}

CodeEditor::~CodeEditor() {
  clearBracketTree(bracketRoot);
  delete m_findDialog;
  delete m_replaceDialog;
  if (m_lspClient) {
    m_lspClient->stopServer();
  }
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
    return space + 15; // Add extra space for folding markers
}

void CodeEditor::updateLineNumberAreaWidth(int newBlockCount) {
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
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
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
                QRectF blockRect(0, top, m_lineNumberArea->width(), m_editor->fontMetrics().height());
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

  // Only handle if no selection
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

  // For all other cases, let the default backspace behavior handle it
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
    if (ch == '(' || ch == '[' || ch == '{' || ch == '<' || ch == '"' || ch == '\'' || ch == '`' ||
        ch == ')' || ch == ']' || ch == '}' || ch == '>' || ch == '"' || ch == '\'' || ch == '`') {
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

  // Get surrounding characters
  QString beforeChar, afterChar;
  if (!cursor.atStart()) {
    QTextCursor beforeCursor = cursor;
    beforeCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
    beforeChar = beforeCursor.selectedText();
  }
  if (!cursor.atEnd()) {
    QTextCursor afterCursor = cursor;
    afterCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    afterChar = afterCursor.selectedText();
  }

  // Get current line text
  cursor.movePosition(QTextCursor::StartOfLine);
  cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
  QString lineText = cursor.selectedText();
  cursor = m_editor->textCursor(); // Restore cursor

  // Check if we're typing a closing character
  if (isCloseBracket(ch)) {
    // If next character is the same closing character, just move cursor right
    if (afterChar == ch) {
      cursor.movePosition(QTextCursor::Right);
      m_editor->setTextCursor(cursor);
      return true;
    }
    return false;  // Let the character be typed normally
  }

  // For quotes
  if (ch == '"' || ch == '\'' || ch == '`') {
    // If next character is the same quote, just move cursor right
    if (afterChar == ch) {
      cursor.movePosition(QTextCursor::Right);
      m_editor->setTextCursor(cursor);
      return true;
    }

    // Special handling for backticks in markdown files
    if (ch == '`' && workingDirectory().endsWith(".md")) {
      // Allow triple backticks
      if (beforeChar == "`" && afterChar == "`") {
        return false;
      }
    }

    // Don't auto-pair quotes in these cases:
    // 1. Inside a word (for apostrophes)
    // 2. After a word character (for apostrophes)
    // 3. Inside a string of a different type
    // 4. When the previous character is a backslash (escaped quote)
    if (!selectedText.isEmpty()) {
      // Always wrap selected text
      cursor.beginEditBlock();
      cursor.insertText(ch + selectedText + ch);
      cursor.endEditBlock();
      m_editor->setTextCursor(cursor);
      return true;
    }

    if (isInsideString()) {
      // Only allow the quote that matches the string we're in
      QChar stringQuote = getStringQuoteChar();
      if (ch != stringQuote) {
        return false;
      }
    }

    // Don't auto-pair if we're in a word or after a word character
    if (!beforeChar.isEmpty() && beforeChar[0].isLetterOrNumber()) {
      return false;
    }
    if (!afterChar.isEmpty() && afterChar[0].isLetterOrNumber()) {
      return false;
    }

    // Don't auto-pair if previous char is backslash (escaped quote)
    if (beforeChar == "\\") {
      return false;
    }

    cursor.beginEditBlock();
    cursor.insertText(ch);
    cursor.insertText(ch);
    cursor.movePosition(QTextCursor::Left);
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
    return true;
  }

  // For opening brackets
  if (isOpenBracket(ch)) {
    // Don't auto-pair in these cases:
    // 1. Inside a string (unless it's a template literal)
    // 2. Inside a comment
    // 3. When followed by a word character
    if (isInsideString() && ch != '`') {
      return false;
    }
    if (isInsideComment()) {
      return false;
    }

    // Handle selected text
    if (!selectedText.isEmpty()) {
      QChar closingChar = getMatchingChar(ch);
      cursor.beginEditBlock();
      cursor.insertText(ch + selectedText + closingChar);
      cursor.endEditBlock();
      m_editor->setTextCursor(cursor);
      return true;
    }

    // Don't auto-pair if next char is a word character
    if (!afterChar.isEmpty() && afterChar[0].isLetterOrNumber()) {
      return false;
    }

    QChar closingChar = getMatchingChar(ch);
    cursor.beginEditBlock();
    cursor.insertText(ch);
    cursor.insertText(closingChar);
    cursor.movePosition(QTextCursor::Left);
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
    return true;
  }

  return false;
}

// Helper function to get the quote character of the string we're currently in
QChar CodeEditor::getStringQuoteChar() const {
  QTextCursor cursor = m_editor->textCursor();
  QTextBlock block = cursor.block();
  QString text = block.text();
  int pos = cursor.positionInBlock();

  bool inString = false;
  QChar quoteChar;
  bool escaped = false;

  for (int i = 0; i < pos; i++) {
    if (escaped) {
      escaped = false;
      continue;
    }

    QChar ch = text[i];
    if (ch == '\\') {
      escaped = true;
      continue;
    }

    if (ch == '"' || ch == '\'' || ch == '`') {
      if (!inString) {
        inString = true;
        quoteChar = ch;
      } else if (ch == quoteChar) {
        inString = false;
      }
    }
  }

  return inString ? quoteChar : QChar();
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
  case '<':
    return '>';
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
  bracketPairs['<'] = '>';
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
  static const QString openChars = "([{<\"'`";
  return openChars.contains(ch);
}

bool CodeEditor::isCloseBracket(QChar ch) const {
  static const QString closeChars = ")]}>\"'`";
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
  connect(m_lspClient, &LSPClient::completionReceived,
          this, &CodeEditor::handleCompletionReceived);
  connect(m_lspClient, &LSPClient::hoverReceived,
          this, &CodeEditor::handleHoverReceived);
  connect(m_lspClient, &LSPClient::definitionReceived,
          this, &CodeEditor::handleDefinitionReceived);
  connect(m_lspClient, &LSPClient::diagnosticsReceived,
          this, &CodeEditor::handleDiagnosticsReceived);
  connect(m_lspClient, &LSPClient::serverError,
          this, &CodeEditor::handleServerError);

  // Start clangd server
  if (m_lspClient->startServer("clangd")) {
    m_lspClient->initialize(workingDirectory());
  }
}

void CodeEditor::handleTextChanged() {
  if (!m_serverInitialized) return;

  QString uri = m_lspClient->uriFromPath(workingDirectory() + "/" + windowTitle());
  m_lspClient->didChange(uri, m_editor->toPlainText());
}

void CodeEditor::handleCursorPositionChanged() {
  if (!m_serverInitialized) return;

  QTextCursor cursor = m_editor->textCursor();
  int line = cursor.blockNumber();
  int character = cursor.positionInBlock();
  QString uri = m_lspClient->uriFromPath(workingDirectory() + "/" + windowTitle());

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
  for (const auto& val : completions) {
    if (val.isObject()) {
      QJsonObject item = val.toObject();
      suggestions << item["label"].toString();
    }
  }

  if (suggestions.isEmpty()) return;

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
  if (contents.isEmpty()) return;
  
  QRect rect = cursorRect();
  QPoint pos = viewport()->mapToGlobal(QPoint(rect.left(), rect.bottom()));
  QToolTip::showText(pos, contents);
}

void CodeEditor::handleDefinitionReceived(const QString &uri, int line, int character) {
  emit gotoDefinitionRequested(uri, line, character);
}

void CodeEditor::handleDiagnosticsReceived(const QString &uri, const QJsonArray &diagnostics) {
  QList<QTextEdit::ExtraSelection> selections;
  
  // Keep existing selections that are not diagnostic highlights
  for (const auto &selection : m_editor->extraSelections()) {
    if (selection.format.background().color().alpha() == 255) {
      selections.append(selection);
    }
  }

  for (const auto& val : diagnostics) {
    if (!val.isObject()) continue;
    
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
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, start["line"].toInt());
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, start["character"].toInt());
    int startPos = cursor.position();

    // Move to end position
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, end["line"].toInt());
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, end["character"].toInt());
    int endPos = cursor.position();

    // Create selection
    QTextEdit::ExtraSelection selection;
    selection.cursor = QTextCursor(m_editor->document());
    selection.cursor.setPosition(startPos);
    selection.cursor.setPosition(endPos, QTextCursor::KeepAnchor);

    // Set format based on severity
    QColor underlineColor;
    if (severity == "1") {  // Error
      underlineColor = QColor("#FF0000");  // Red
    } else if (severity == "2") {  // Warning
      underlineColor = QColor("#FFA500");  // Orange
    } else if (severity == "3") {  // Information
      underlineColor = QColor("#2196F3");  // Blue
    } else if (severity == "4") {  // Hint
      underlineColor = QColor("#4CAF50");  // Green
    } else {
      underlineColor = QColor("#FF0000");  // Default to red
    }

    // Create squiggly underline effect
    QTextCharFormat format;
    format.setUnderlineColor(underlineColor);
    format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    format.setToolTip(message);  // Show diagnostic message on hover

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
  if (!m_serverInitialized) return;

  QTextCursor cursor = m_editor->textCursor();
  int line = cursor.blockNumber();
  int character = cursor.positionInBlock();
  QString uri = m_lspClient->uriFromPath(workingDirectory() + "/" + windowTitle());

  m_lspClient->requestDefinition(uri, line, character);
}

void CodeEditor::requestHover(const QTextCursor &cursor) {
  if (!m_serverInitialized) return;

  int line = cursor.blockNumber();
  int character = cursor.positionInBlock();
  QString uri = m_lspClient->uriFromPath(workingDirectory() + "/" + windowTitle());

  m_lspClient->requestHover(uri, line, character);
}

QString CodeEditor::getWordUnderCursor(const QTextCursor &cursor) const {
  QTextCursor wordCursor = cursor;
  wordCursor.select(QTextCursor::WordUnderCursor);
  return wordCursor.selectedText();
}

bool CodeEditor::isInsideString() const {
    QTextCursor cursor = m_editor->textCursor();
    int position = cursor.position();
    QString text = m_editor->toPlainText();
    
    // Scan from the start of the line to the cursor position
    int lineStart = text.lastIndexOf('\n', position - 1) + 1;
    if (lineStart == 0) lineStart = 0;
    
    bool inString = false;
    QChar stringChar;
    
    for (int i = lineStart; i < position; i++) {
        if (i >= text.length()) break;
        
        QChar currentChar = text[i];
        
        // Handle escape sequences
        if (currentChar == '\\' && i + 1 < text.length()) {
            i++; // Skip next character
            continue;
        }
        
        // Toggle string state if we encounter a quote
        if ((currentChar == '"' || currentChar == '\'' || currentChar == '`')) {
            if (!inString) {
                inString = true;
                stringChar = currentChar;
            } else if (currentChar == stringChar) {
                inString = false;
            }
        }
    }
    
    return inString;
}

bool CodeEditor::isInsideComment() const {
    QTextCursor cursor = m_editor->textCursor();
    int position = cursor.position();
    QString text = m_editor->toPlainText();
    
    // Find start of the line
    int lineStart = text.lastIndexOf('\n', position - 1) + 1;
    if (lineStart == 0) lineStart = 0;
    
    // Check for // comments
    for (int i = lineStart; i < position; i++) {
        if (i + 1 >= text.length()) break;
        
        if (text[i] == '/' && text[i + 1] == '/') {
            return true;
        }
    }
    
    return false;
}

void CodeEditor::toggleFold(const QTextBlock &blockToFold) {
    if (!blockToFold.isValid()) return;
    
    QTextBlock block = blockToFold;
    if (isFoldable(block)) {
        bool shouldFold = !isFolded(block);
        
        // If this block is part of a function/class declaration spanning multiple lines,
        // find the block with the opening brace
        if (!block.text().trimmed().endsWith('{')) {
            QTextBlock searchBlock = block;
            int maxLines = 3;
            while (searchBlock.isValid() && maxLines > 0) {
                QString text = searchBlock.text().trimmed();
                if (text.endsWith('{')) {
                    block = searchBlock;
                    break;
                }
                searchBlock = searchBlock.next();
                maxLines--;
            }
        }
        
        setFolded(block, shouldFold);
        updateVisibleBlocks();
        m_editor->viewport()->update();
        m_lineNumberArea->update();
    }
}

void CodeEditor::foldAll() {
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        if (isFoldable(block)) {
            setFolded(block, true);
        }
        block = block.next();
    }
    updateVisibleBlocks();
    viewport()->update();
}

void CodeEditor::unfoldAll() {
    m_foldedBlocks.clear();
    updateVisibleBlocks();
    viewport()->update();
}

bool CodeEditor::isFoldable(const QTextBlock &block) const {
    if (!block.isValid()) return false;
    
    QString text = block.text().trimmed();
    if (text.isEmpty()) return false;

    // Case 1: Block ends with an opening brace
    if (text.endsWith('{')) return true;

    // Case 2: Function or class declaration that might span multiple lines
    if (text.startsWith("class ") || text.startsWith("struct ") ||
        text.contains("function") || text.contains("(")) {
        // Check if any following block has an opening brace
        QTextBlock nextBlock = block.next();
        int maxLines = 3; // Look ahead maximum 3 lines
        while (nextBlock.isValid() && maxLines > 0) {
            QString nextText = nextBlock.text().trimmed();
            if (nextText.endsWith('{')) return true;
            if (!nextText.isEmpty() && !nextText.contains("(")) break;
            nextBlock = nextBlock.next();
            maxLines--;
        }
    }

    // Case 3: Indentation-based folding
    int currentIndent = getIndentLevel(block.text());
    if (currentIndent == 0) return false; // Don't fold non-indented blocks

    QTextBlock nextBlock = block.next();
    while (nextBlock.isValid() && nextBlock.text().trimmed().isEmpty()) {
        nextBlock = nextBlock.next();
    }
    
    if (!nextBlock.isValid()) return false;
    
    int nextIndent = getIndentLevel(nextBlock.text());
    return nextIndent > currentIndent;
}

bool CodeEditor::isFolded(const QTextBlock &block) const {
    return m_foldedBlocks.value(block.blockNumber(), false);
}

void CodeEditor::setFolded(const QTextBlock &block, bool folded) {
    if (!isFoldable(block)) return;
    
    int blockNum = block.blockNumber();
    if (folded) {
        m_foldedBlocks[blockNum] = true;
        // Find and store the end of the folding range
        int endBlockNum = findFoldingEndBlock(block);
        if (endBlockNum > blockNum) {
            m_foldingRanges[blockNum] = endBlockNum;
        }
    } else {
        m_foldedBlocks.remove(blockNum);
        m_foldingRanges.remove(blockNum);
    }
}

int CodeEditor::findFoldingEndBlock(const QTextBlock &startBlock) const {
    if (!startBlock.isValid()) return -1;
    
    QString startText = startBlock.text().trimmed();
    int startIndent = getIndentLevel(startBlock.text());
    
    // Case 1: Block starts with a brace
    bool hasBrace = startText.endsWith('{');
    int braceCount = hasBrace ? 1 : 0;
    
    // Case 2: Function/class declaration that might span multiple lines
    if (!hasBrace && (startText.startsWith("class ") || startText.startsWith("struct ") ||
        startText.contains("function") || startText.contains("("))) {
        QTextBlock nextBlock = startBlock.next();
        int maxLines = 3;
        while (nextBlock.isValid() && maxLines > 0) {
            QString nextText = nextBlock.text().trimmed();
            if (nextText.endsWith('{')) {
                hasBrace = true;
                braceCount = 1;
                break;
            }
            if (!nextText.isEmpty() && !nextText.contains("(")) break;
            nextBlock = nextBlock.next();
            maxLines--;
        }
    }
    
    QTextBlock block = startBlock.next();
    bool foundContent = false; // Track if we've found any non-empty content
    
    while (block.isValid()) {
        QString text = block.text().trimmed();
        if (text.isEmpty()) {
            block = block.next();
            continue;
        }
        
        foundContent = true;
        int indent = getIndentLevel(block.text());
        
        if (hasBrace) {
            // Count braces in the line
            braceCount += text.count('{') - text.count('}');
            if (braceCount == 0) {
                return block.blockNumber();
            }
            // If we find a closing brace at the same indent level, it's probably the end
            if (braceCount > 0 && indent == startIndent && text.endsWith('}')) {
                return block.blockNumber();
            }
        } else {
            // For indentation-based folding
            if (indent <= startIndent && foundContent) {
                return block.previous().blockNumber();
            }
        }
        
        block = block.next();
    }
    
    // If we reach the end, return the last valid block
    return block.isValid() ? block.blockNumber() : startBlock.document()->lastBlock().blockNumber();
}

bool CodeEditor::isBlockVisible(const QTextBlock &block) const {
    if (!block.isValid()) return false;
    
    // Check if any parent blocks are folded
    QTextBlock current = block.previous();
    int currentBlockNum = block.blockNumber();
    
    while (current.isValid()) {
        int parentBlockNum = current.blockNumber();
        if (m_foldedBlocks.value(parentBlockNum, false)) {
            int foldEnd = m_foldingRanges.value(parentBlockNum, -1);
            if (currentBlockNum <= foldEnd) {
                return false;
            }
        }
        current = current.previous();
    }
    
    return true;
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

void CodeEditor::paintFoldingMarkers(QPainter &painter, const QTextBlock &block, const QRectF &rect) {
    if (!isFoldable(block)) return;
    
    bool folded = isFolded(block);
    bool hovered = m_hoveredFoldMarkers.value(block.blockNumber(), false);
    int top = rect.top();
    
    // Larger base size for the marker
    qreal markerSize = hovered ? 16.0 : 12.0;
    qreal yOffset = (rect.height() - markerSize) / 2; // Center vertically in the line
    
    // Draw folding marker background
    QRectF markerRect(4, top + yOffset, markerSize, markerSize);
    painter.save();
    
    // Draw background with hover effect
    painter.setPen(Qt::NoPen);
    if (hovered) {
        // More vibrant sky blue when hovered
        painter.setBrush(QColor(100, 181, 246, 220));
    } else {
        // Slightly more visible when not hovered
        painter.setBrush(QColor(100, 100, 100, 160));
    }
    
    // Draw a rounded rectangle for better visual appeal
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawRoundedRect(markerRect, 3, 3);
    
    // Draw marker symbol
    painter.setPen(QPen(hovered ? Qt::white : QColor(240, 240, 240), hovered ? 2.0 : 1.5));
    qreal centerY = markerRect.center().y();
    qreal centerX = markerRect.center().x();
    qreal lineLength = markerSize * 0.7; // Slightly larger lines
    
    // Draw the horizontal line
    painter.drawLine(QPointF(centerX - lineLength/2, centerY),
                    QPointF(centerX + lineLength/2, centerY));
    
    if (!folded) {
        // Draw the vertical line for expanded state
        painter.drawLine(QPointF(centerX, centerY - lineLength/2),
                        QPointF(centerX, centerY + lineLength/2));
    }
    
    painter.restore();
}

bool CodeEditor::isFoldMarkerUnderMouse(const QPoint &pos, const QTextBlock &block) const {
    if (!block.isValid() || !isFoldable(block)) return false;
    
    int top = m_editor->blockBoundingGeometry(block)
                .translated(m_editor->contentOffset()).top();
    int height = m_editor->blockBoundingRect(block).height();
    
    // Much larger hit area for better sensitivity (entire left margin width x line height)
    QRectF hitArea(0, top, lineNumberAreaWidth() - 5, height);
    return hitArea.contains(pos.x(), pos.y());
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
        if (!block.isVisible()) continue;
        
        // Stop if we're past the visible area
        int bottom = m_editor->blockBoundingGeometry(block)
                        .translated(m_editor->contentOffset()).bottom();
        if (bottom > m_editor->viewport()->height()) break;
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
                .translated(m_editor->contentOffset()).top();
    int bottom = top + m_editor->blockBoundingRect(block).height();
    
    do {
        if (block.isValid() && top <= y && y <= bottom) {
            return block;
        }
        
        block = block.next();
        if (!block.isValid()) break;
        
        top = bottom;
        bottom = top + m_editor->blockBoundingRect(block).height();
    } while (block.isValid() && top <= y);
    
    return QTextBlock();
}

#include "moc_codeeditor.cpp"
