#pragma once
#include <QPlainTextEdit>
#include "cpphighlighter.h"

class LineNumberArea;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void toggleLineComment();

private:
    LineNumberArea *lineNumberArea;
    CppHighlighter *highlighter;
    bool isLineCommented(const QString &text) const;
}; 