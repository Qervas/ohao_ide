#pragma once
#include "basehighlighter.h"
#include <QRegularExpression>
#include <QTextCharFormat>

class CppHighlighter : public BaseHighlighter {
    Q_OBJECT

public:
    explicit CppHighlighter(QTextDocument *parent = nullptr);

    QString name() const override { return "C++"; }
    QString description() const override { return "Syntax highlighting for C/C++ files"; }
    QString filePattern() const override { return "*.cpp;*.h;*.hpp;*.c;*.cc;*.cxx"; }

protected:
    void doHighlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat preprocessorFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat operatorFormat;

    void setupFormats();
    void setupRules();
}; 