#include "cpphighlighter.h"

CppHighlighter::CppHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Keywords
    keywordFormat.setForeground(QColor("#569CD6"));
    keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        // C++ keywords
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
        QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bnoexcept\\b"),
        QStringLiteral("\\bnullptr\\b"), QStringLiteral("\\bconstexpr\\b"),
        QStringLiteral("\\bconcept\\b"), QStringLiteral("\\brequires\\b"),
        // Control flow
        QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"),
        QStringLiteral("\\bfor\\b"), QStringLiteral("\\bwhile\\b"),
        QStringLiteral("\\bdo\\b"), QStringLiteral("\\bswitch\\b"),
        QStringLiteral("\\bcase\\b"), QStringLiteral("\\bbreak\\b"),
        QStringLiteral("\\bcontinue\\b"), QStringLiteral("\\breturn\\b"),
        QStringLiteral("\\btry\\b"), QStringLiteral("\\bcatch\\b"),
        QStringLiteral("\\bthrow\\b"), QStringLiteral("\\busing\\b"),
        // Types
        QStringLiteral("\\bvoid\\b"), QStringLiteral("\\bchar\\b"),
        QStringLiteral("\\bshort\\b"), QStringLiteral("\\bint\\b"),
        QStringLiteral("\\blong\\b"), QStringLiteral("\\bfloat\\b"),
        QStringLiteral("\\bdouble\\b"), QStringLiteral("\\bbool\\b"),
        QStringLiteral("\\bsigned\\b"), QStringLiteral("\\bunsigned\\b"),
        QStringLiteral("\\bauto\\b"), QStringLiteral("\\bregister\\b"),
        QStringLiteral("\\bextern\\b"), QStringLiteral("\\bmutable\\b"),
        QStringLiteral("\\bthis\\b"), QStringLiteral("\\bthread_local\\b")
    };

    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Class names
    classFormat.setForeground(QColor("#4EC9B0"));
    classFormat.setFontWeight(QFont::Bold);
    HighlightingRule rule;
    rule.pattern = QRegularExpression(QStringLiteral("\\bQ[A-Za-z]+\\b"));
    rule.format = classFormat;
    highlightingRules.append(rule);

    // Functions
    functionFormat.setForeground(QColor("#DCDCAA"));
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // Preprocessor
    preprocessorFormat.setForeground(QColor("#BD63C5"));
    rule.pattern = QRegularExpression(QStringLiteral("^\\s*#\\s*[a-zA-Z]+"));
    rule.format = preprocessorFormat;
    highlightingRules.append(rule);

    // Numbers
    numberFormat.setForeground(QColor("#B5CEA8"));
    rule.pattern = QRegularExpression(QStringLiteral("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b"));
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // Operators
    operatorFormat.setForeground(QColor("#D4D4D4"));
    rule.pattern = QRegularExpression(QStringLiteral("[!%^&*()+=|~{}\\[\\]<>/-]"));
    rule.format = operatorFormat;
    highlightingRules.append(rule);

    // Single-line comments
    singleLineCommentFormat.setForeground(QColor("#6A9955"));
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // Quotations
    quotationFormat.setForeground(QColor("#CE9178"));
    rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Multi-line comments
    multiLineCommentFormat.setForeground(QColor("#6A9955"));
    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void CppHighlighter::highlightBlock(const QString &text)
{
    // Apply regular highlighting rules
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
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
        int commentLength;
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