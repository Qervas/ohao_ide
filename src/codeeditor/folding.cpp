#include "folding.h"

CodeFolding::CodeFolding() {
}

CodeFolding::~CodeFolding() {
}

bool CodeFolding::isFoldable(const QTextBlock &block) const {
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

bool CodeFolding::isFolded(const QTextBlock &block) const {
    return m_foldedBlocks.value(block.blockNumber(), false);
}

void CodeFolding::setFolded(const QTextBlock &block, bool folded) {
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

void CodeFolding::toggleFold(const QTextBlock &block) {
    if (!block.isValid()) return;
    
    QTextBlock blockToFold = block;
    if (isFoldable(blockToFold)) {
        bool shouldFold = !isFolded(blockToFold);
        
        // If this block is part of a function/class declaration spanning multiple lines,
        // find the block with the opening brace
        if (!blockToFold.text().trimmed().endsWith('{')) {
            QTextBlock searchBlock = blockToFold;
            int maxLines = 3;
            while (searchBlock.isValid() && maxLines > 0) {
                QString text = searchBlock.text().trimmed();
                if (text.endsWith('{')) {
                    blockToFold = searchBlock;
                    break;
                }
                searchBlock = searchBlock.next();
                maxLines--;
            }
        }
        
        setFolded(blockToFold, shouldFold);
    }
}

void CodeFolding::foldAll() {
    QTextBlock block = block.document()->firstBlock();
    while (block.isValid()) {
        if (isFoldable(block)) {
            setFolded(block, true);
        }
        block = block.next();
    }
}

void CodeFolding::unfoldAll() {
    m_foldedBlocks.clear();
    m_foldingRanges.clear();
}

bool CodeFolding::isBlockVisible(const QTextBlock &block) const {
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

int CodeFolding::findFoldingEndBlock(const QTextBlock &startBlock) const {
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

int CodeFolding::getIndentLevel(const QString &text) const {
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