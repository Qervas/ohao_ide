#pragma once

#include <QChar>
#include <QString>
#include <QTextCursor>

class QuoteMatcher {
public:
  QuoteMatcher();

  bool isQuoteChar(QChar ch) const;
  bool shouldAutoClose(QChar quoteChar, const QString &beforeChar,
                       const QString &afterChar) const;
  bool handleQuoteChar(QChar ch, QTextCursor &cursor, bool isMarkdown = false);
  bool isInsideString(const QTextCursor &cursor) const;
  QChar getStringQuoteChar(const QTextCursor &cursor) const;
  QString getSurroundingText(const QTextCursor &cursor, int chars = 2) const;
  QString getCharBefore(const QTextCursor &cursor) const;
  QString getCharAfter(const QTextCursor &cursor) const;

private:
  bool isWordChar(QChar ch) const;
  bool isCloseQuoteContext(const QString &beforeText) const;
  bool shouldSkipClosingQuote(QChar quoteChar, const QTextCursor &cursor) const;
  bool handleTripleQuotes(QChar ch, QTextCursor &cursor,
                          const QString &surroundingText) const;

  void wrapSelectedText(QTextCursor &cursor, QChar quote, const QString &text);
  void insertMatchingQuotes(QTextCursor &cursor, QChar quote);

  static const QString QuoteChars;
};
