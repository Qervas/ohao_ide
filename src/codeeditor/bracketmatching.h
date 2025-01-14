#pragma once

#include <QChar>
#include <QColor>
#include <QMap>

struct BracketPair {
    int openPos;
    int closePos;
    int level;
    QChar openChar;
    QChar closeChar;
    bool isInvalid;

    BracketPair(int open = -1, int close = -1, int lvl = 0, QChar oChar = QChar(), QChar cChar = QChar())
        : openPos(open), closePos(close), level(lvl), openChar(oChar), closeChar(cChar), isInvalid(false) {}
};

struct BracketNode {
    BracketPair pair;
    BracketNode *left;
    BracketNode *right;
    int height;

    explicit BracketNode(const BracketPair &p)
        : pair(p), left(nullptr), right(nullptr), height(1) {}
};

class BracketMatcher {
public:
    BracketMatcher();
    ~BracketMatcher();

    void updateBracketTree(const QString &text);
    BracketNode* findMatchingBracket(int position) const;
    QColor getBracketColor(int level) const;
    void clearBracketTree();

private:
    BracketNode* insertBracket(BracketNode* node, const BracketPair &pair);
    int getHeight(BracketNode* node) const;
    int getBalance(BracketNode* node) const;
    BracketNode* rotateLeft(BracketNode* x);
    BracketNode* rotateRight(BracketNode* y);
    void clearBracketTree(BracketNode* node);
    bool isOpenBracket(QChar ch) const;
    bool isCloseBracket(QChar ch) const;
    QChar getMatchingBracket(QChar ch) const;

    BracketNode* bracketRoot;
    QMap<QChar, QChar> bracketPairs;
    QVector<QColor> bracketColors;
}; 