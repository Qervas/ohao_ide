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
    void foldAll();
    void unfoldAll();
    bool isBlockVisible(const QTextBlock &block) const;
    int findFoldingEndBlock(const QTextBlock &startBlock) const;
    int getIndentLevel(const QString &text) const;

private:
    QMap<int, bool> m_foldedBlocks;
    QMap<int, int> m_foldingRanges;
}; 