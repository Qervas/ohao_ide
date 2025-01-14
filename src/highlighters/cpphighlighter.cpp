#include "cpphighlighter.h"

CppHighlighter::CppHighlighter(QTextDocument *parent)
    : BaseHighlighter(parent) {
    setupFormats();
    setupRules();
}

void CppHighlighter::setupFormats() {
    // Keywords
    keywordFormat.setForeground(QColor("#569CD6")); // Blue
    keywordFormat.setFontWeight(QFont::Bold);

    // Classes
    classFormat.setForeground(QColor("#4EC9B0")); // Teal

    // Single line comments
    singleLineCommentFormat.setForeground(QColor("#608B4E")); // Green

    // Multi line comments
    multiLineCommentFormat.setForeground(QColor("#608B4E")); // Green

    // Quotations
    quotationFormat.setForeground(QColor("#D69D85")); // Brown

    // Functions
    functionFormat.setForeground(QColor("#DCDCAA")); // Yellow

    // Preprocessor
    preprocessorFormat.setForeground(QColor("#BD63C5")); // Purple

    // Numbers
    numberFormat.setForeground(QColor("#B5CEA8")); // Light green

    // Operators
    operatorFormat.setForeground(QColor("#D4D4D4")); // Light gray
}

void CppHighlighter::setupRules() {
    highlightingRules.clear();
    HighlightingRule rule;

    // Keywords
    const QString keywordPatterns[] = {
        QStringLiteral("\\bclass\\b"), QStringLiteral("\\bconst\\b"),
        QStringLiteral("\\benum\\b"), QStringLiteral("\\bexplicit\\b"),
        QStringLiteral("\\bfriend\\b"), QStringLiteral("\\binline\\b"),
        QStringLiteral("\\bnamespace\\b"), QStringLiteral("\\boperator\\b"),
        QStringLiteral("\\bprivate\\b"), QStringLiteral("\\bprotected\\b"),
        QStringLiteral("\\bpublic\\b"), QStringLiteral("\\bsignals\\b"),
        QStringLiteral("\\bslots\\b"), QStringLiteral("\\bstatic\\b"),
        QStringLiteral("\\bstruct\\b"), QStringLiteral("\\btemplate\\b"),
        QStringLiteral("\\btypedef\\b"), QStringLiteral("\\btypename\\b"),
        QStringLiteral("\\bunion\\b"), QStringLiteral("\\bvirtual\\b"),
        QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bbreak\\b"),
        QStringLiteral("\\bcase\\b"), QStringLiteral("\\bcatch\\b"),
        QStringLiteral("\\bcontinue\\b"), QStringLiteral("\\bdefault\\b"),
        QStringLiteral("\\bdelete\\b"), QStringLiteral("\\bdo\\b"),
        QStringLiteral("\\belse\\b"), QStringLiteral("\\bfor\\b"),
        QStringLiteral("\\bgoto\\b"), QStringLiteral("\\bif\\b"),
        QStringLiteral("\\bnew\\b"), QStringLiteral("\\breturn\\b"),
        QStringLiteral("\\bswitch\\b"), QStringLiteral("\\btry\\b"),
        QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bauto\\b"),
        QStringLiteral("\\bbool\\b"), QStringLiteral("\\bchar\\b"),
        QStringLiteral("\\bdouble\\b"), QStringLiteral("\\bfloat\\b"),
        QStringLiteral("\\bint\\b"), QStringLiteral("\\blong\\b"),
        QStringLiteral("\\bshort\\b"), QStringLiteral("\\bsigned\\b"),
        QStringLiteral("\\bunsigned\\b"), QStringLiteral("\\bvoid\\b"),
        QStringLiteral("\\boverride\\b"), QStringLiteral("\\bfinal\\b"),
        QStringLiteral("\\bnullptr\\b"), QStringLiteral("\\bthis\\b"),
        QStringLiteral("\\btrue\\b"), QStringLiteral("\\bfalse\\b")
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Class
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z_][A-Za-z0-9_]*\\b(?=\\s*[:{])"));
    rule.format = classFormat;
    highlightingRules.append(rule);

    // Function
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // Preprocessor
    rule.pattern = QRegularExpression(QStringLiteral("^\\s*#\\s*[a-zA-Z_][a-zA-Z0-9_]*\\b"));
    rule.format = preprocessorFormat;
    highlightingRules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression(QStringLiteral("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b"));
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // Operators
    rule.pattern = QRegularExpression(QStringLiteral("[!%&\\*\\+\\-/:<=>\\?\\^\\|~]"));
    rule.format = operatorFormat;
    highlightingRules.append(rule);

    // Single line comment
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // Quotation
    rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Multi-line comments
    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void CppHighlighter::doHighlightBlock(const QString &text) {
    // Apply regular highlighting rules
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Handle multi-line comments
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }

        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
} 