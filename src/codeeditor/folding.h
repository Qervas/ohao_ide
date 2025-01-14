#pragma once

#include <QMap>
#include <QTextBlock>

class CodeFolding {
public:
  CodeFolding();
  ~CodeFolding();

  bool isFoldable(const QTextBlock &block) const;
  bool isFolded(const QTextBlock &block) const;
  void setFolded(const QTextBlock &block, bool folded);
  void toggleFold(const QTextBlock &block);
  void foldAll(QTextDocument *doc);
  void unfoldAll();
  bool isBlockVisible(const QTextBlock &block) const;
  int findFoldingEndBlock(const QTextBlock &startBlock) const;
  int getIndentLevel(const QString &text) const;
  void paintFoldingMarkers(QPainter &painter, const QTextBlock &block,
                           const QRectF &rect, bool folded, bool hovered,
                           int lineNumberAreaWidth);
  bool isFoldMarkerUnderMouse(const QPoint &pos, const QTextBlock &block,
                              int top, int height,
                              int lineNumberAreaWidth) const;

private:
  QMap<int, bool> m_foldedBlocks;
  QMap<int, int> m_foldingRanges;
};
