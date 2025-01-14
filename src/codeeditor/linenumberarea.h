#pragma once

#include <QWidget>

class CodeEditor;

class LineNumberArea : public QWidget {
    Q_OBJECT

public:
    explicit LineNumberArea(CodeEditor *editor);

    QSize sizeHint() const override;

signals:
    void mousePressed(QPoint pos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    CodeEditor *m_codeEditor;
}; 