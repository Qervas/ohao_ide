#include "bracketmatching.h"

BracketMatcher::BracketMatcher() : bracketRoot(nullptr) {
  // Initialize bracket pairs
  bracketPairs['{'] = '}';
  bracketPairs['['] = ']';
  bracketPairs['('] = ')';
  bracketPairs['<'] = '>';
  bracketPairs['"'] = '"';
  bracketPairs['\''] = '\'';
  bracketPairs['`'] = '`';

  // Initialize colors with VSCode-style colors
  bracketColors = {
      QColor("#FFB200"), // Gold
      QColor("#DA70D6"), // Orchid
      QColor("#179FFF"), // Light Blue
      QColor("#00B28B"), // Turquoise
      QColor("#FF7AB2"), // Pink
      QColor("#B48EAD")  // Purple Gray
  };
}

BracketMatcher::~BracketMatcher() { clearBracketTree(); }

void BracketMatcher::updateBracketTree(const QString &text) {
  clearBracketTree();
  bracketRoot = nullptr;

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

BracketNode *BracketMatcher::findMatchingBracket(int position) const {
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

QColor BracketMatcher::getBracketColor(int level) const {
  if (bracketColors.isEmpty())
    return QColor(Qt::white);
  return bracketColors[level % bracketColors.size()];
}

void BracketMatcher::clearBracketTree() {
  clearBracketTree(bracketRoot);
  bracketRoot = nullptr;
}

BracketNode *BracketMatcher::insertBracket(BracketNode *node,
                                           const BracketPair &pair) {
  if (!node) {
    return new BracketNode(pair);
  }

  if (pair.openPos < node->pair.openPos) {
    node->left = insertBracket(node->left, pair);
  } else if (pair.openPos > node->pair.openPos) {
    node->right = insertBracket(node->right, pair);
  } else {
    return node;
  }

  node->height = 1 + qMax(getHeight(node->left), getHeight(node->right));

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

int BracketMatcher::getHeight(BracketNode *node) const {
  if (!node)
    return 0;
  return node->height;
}

int BracketMatcher::getBalance(BracketNode *node) const {
  if (!node)
    return 0;
  return getHeight(node->left) - getHeight(node->right);
}

BracketNode *BracketMatcher::rotateLeft(BracketNode *x) {
  BracketNode *y = x->right;
  BracketNode *T2 = y->left;

  y->left = x;
  x->right = T2;

  x->height = 1 + qMax(getHeight(x->left), getHeight(x->right));
  y->height = 1 + qMax(getHeight(y->left), getHeight(y->right));

  return y;
}

BracketNode *BracketMatcher::rotateRight(BracketNode *y) {
  BracketNode *x = y->left;
  BracketNode *T2 = x->right;

  x->right = y;
  y->left = T2;

  y->height = 1 + qMax(getHeight(y->left), getHeight(y->right));
  x->height = 1 + qMax(getHeight(x->left), getHeight(x->right));

  return x;
}

void BracketMatcher::clearBracketTree(BracketNode *node) {
  if (node) {
    clearBracketTree(node->left);
    clearBracketTree(node->right);
    delete node;
  }
}

bool BracketMatcher::isOpenBracket(QChar ch) const {
  static const QString openChars = "([{<\"'`";
  return openChars.contains(ch);
}

bool BracketMatcher::isCloseBracket(QChar ch) const {
  static const QString closeChars = ")]}>\"'`";
  return closeChars.contains(ch);
}

QChar BracketMatcher::getMatchingBracket(QChar ch) const {
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

bool BracketMatcher::isInsideComment(const QTextCursor &cursor) const {
  QTextBlock block = cursor.block();
  int position = cursor.positionInBlock();
  QString text = block.text().left(position);

  // Check for // comments
  for (int i = 0; i < text.length() - 1; i++) {
    if (text[i] == '/' && text[i + 1] == '/') {
      return true;
    }
  }

  return false;
}

bool BracketMatcher::isMatchingPair(QChar open, QChar close) const {
  return bracketPairs.value(open) == close;
}

QString BracketMatcher::getCharAfter(const QTextCursor &cursor) const {
  if (cursor.atEnd())
    return QString();

  QTextCursor tempCursor = cursor;
  tempCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
  return tempCursor.selectedText();
}

QString BracketMatcher::getCharBefore(const QTextCursor &cursor) const {
  if (cursor.atStart())
    return QString();

  QTextCursor tempCursor = cursor;
  tempCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
  return tempCursor.selectedText();
}
