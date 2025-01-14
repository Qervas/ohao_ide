#include "linenumberarea.h"
#include "codeeditor.h"
#include <QPainter>
#include <QMouseEvent>

LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor), m_codeEditor(editor) {
}

QSize LineNumberArea::sizeHint() const {
    return QSize(m_codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
    m_codeEditor->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit mousePressed(event->pos());
    }
    QWidget::mousePressEvent(event);
} 