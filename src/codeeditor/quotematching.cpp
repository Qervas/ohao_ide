#include "quotematching.h"
#include <QTextBlock>

const QString QuoteMatcher::QuoteChars = "\"'`";

QuoteMatcher::QuoteMatcher() {}

bool QuoteMatcher::isQuoteChar(QChar ch) const {
  return QuoteChars.contains(ch);
}

bool QuoteMatcher::shouldAutoClose(QChar quoteChar, const QString &beforeChar,
                                   const QString &afterChar) const {
  // Don't auto-close if:
  // 1. Previous char is backslash (escape)
  if (beforeChar == "\\")
    return false;

  // 2. Inside a word (for apostrophes)
  if (!beforeChar.isEmpty() && !afterChar.isEmpty() &&
      isWordChar(beforeChar[0]) && isWordChar(afterChar[0])) {
    return false;
  }

  // 3. After a word character (for apostrophes)
  if (!beforeChar.isEmpty() && isWordChar(beforeChar[0])) {
    return false;
  }

  // 4. In a string literal context
  if (isCloseQuoteContext(beforeChar)) {
    return false;
  }

  return true;
}

bool QuoteMatcher::handleQuoteChar(QChar ch, QTextCursor &cursor,
                                   bool isMarkdown) {
  if (!isQuoteChar(ch))
    return false;

  QString selectedText = cursor.selectedText();
  QString surroundingText = getSurroundingText(cursor);
  QString beforeChar = getCharBefore(cursor);
  QString afterChar = getCharAfter(cursor);

  // Special handling for markdown backticks
  if (isMarkdown && ch == '`') {
    if (handleTripleQuotes(ch, cursor, surroundingText)) {
      return true;
    }
  }

  // Skip if we're at a closing quote
  if (shouldSkipClosingQuote(ch, cursor)) {
    cursor.movePosition(QTextCursor::Right);
    return true;
  }

  // Handle selected text
  if (!selectedText.isEmpty()) {
    wrapSelectedText(cursor, ch, selectedText);
    return true;
  }

  // Handle auto-closing
  if (shouldAutoClose(ch, beforeChar, afterChar)) {
    insertMatchingQuotes(cursor, ch);
    return true;
  }

  // Default: just insert the quote
  cursor.insertText(QString(ch));
  return true;
}

void QuoteMatcher::wrapSelectedText(QTextCursor &cursor, QChar quote,
                                    const QString &text) {
  cursor.beginEditBlock();
  cursor.insertText(QString(quote) + text + QString(quote));
  cursor.endEditBlock();
}

void QuoteMatcher::insertMatchingQuotes(QTextCursor &cursor, QChar quote) {
  cursor.beginEditBlock();
  cursor.insertText(QString(quote));
  cursor.insertText(QString(quote));
  cursor.movePosition(QTextCursor::Left);
  cursor.endEditBlock();
}

bool QuoteMatcher::isInsideString(const QTextCursor &cursor) const {
  QTextBlock block = cursor.block();
  QString text = block.text().left(cursor.positionInBlock());

  bool inString = false;
  QChar stringChar;
  bool escaped = false;

  for (const QChar &ch : text) {
    if (escaped) {
      escaped = false;
      continue;
    }

    if (ch == '\\') {
      escaped = true;
      continue;
    }

    if (isQuoteChar(ch)) {
      if (!inString) {
        inString = true;
        stringChar = ch;
      } else if (ch == stringChar) {
        inString = false;
      }
    }
  }

  return inString;
}

QChar QuoteMatcher::getStringQuoteChar(const QTextCursor &cursor) const {
  if (!isInsideString(cursor))
    return QChar();

  QTextBlock block = cursor.block();
  QString text = block.text().left(cursor.positionInBlock());

  QChar lastQuote;
  bool escaped = false;

  for (const QChar &ch : text) {
    if (escaped) {
      escaped = false;
      continue;
    }

    if (ch == '\\') {
      escaped = true;
      continue;
    }

    if (isQuoteChar(ch)) {
      lastQuote = ch;
    }
  }

  return lastQuote;
}

bool QuoteMatcher::isWordChar(QChar ch) const {
  return ch.isLetterOrNumber() || ch == '_';
}

bool QuoteMatcher::isCloseQuoteContext(const QString &beforeText) const {
  // Check if we're in a context where we're closing a quote
  if (beforeText.isEmpty())
    return false;

  int quoteCount = 0;
  bool escaped = false;

  for (const QChar &ch : beforeText) {
    if (escaped) {
      escaped = false;
      continue;
    }

    if (ch == '\\') {
      escaped = true;
      continue;
    }

    if (isQuoteChar(ch)) {
      quoteCount++;
    }
  }

  return quoteCount % 2 == 1;
}

bool QuoteMatcher::shouldSkipClosingQuote(QChar quoteChar,
                                          const QTextCursor &cursor) const {
  QString afterChar = getCharAfter(cursor);
  return afterChar == QString(quoteChar) && isInsideString(cursor);
}

bool QuoteMatcher::handleTripleQuotes(QChar ch, QTextCursor &cursor,
                                      const QString &surroundingText) const {
  // Check for triple backtick scenario
  if (surroundingText.contains("``")) {
    return false; // Let editor handle it normally
  }

  // Check if we're starting a code block
  QString beforeText = cursor.block().text().left(cursor.positionInBlock());
  if (beforeText.trimmed().isEmpty()) {
    cursor.beginEditBlock();
    cursor.insertText("```\n\n```");
    cursor.movePosition(QTextCursor::Up);
    cursor.endEditBlock();
    return true;
  }

  return false;
}

QString QuoteMatcher::getSurroundingText(const QTextCursor &cursor,
                                         int chars) const {
  QTextCursor tempCursor = cursor;
  tempCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, chars);
  QString before = tempCursor.selectedText();

  tempCursor = cursor;
  tempCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, chars);
  QString after = tempCursor.selectedText();

  return before + after;
}

QString QuoteMatcher::getCharBefore(const QTextCursor &cursor) const {
  if (cursor.atStart())
    return QString();

  QTextCursor tempCursor = cursor;
  tempCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
  return tempCursor.selectedText();
}

QString QuoteMatcher::getCharAfter(const QTextCursor &cursor) const {
  if (cursor.atEnd())
    return QString();

  QTextCursor tempCursor = cursor;
  tempCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
  return tempCursor.selectedText();
}
