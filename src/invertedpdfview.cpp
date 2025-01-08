#include "invertedpdfview.h"
#include <QPainter>
#include <QPaintEvent>
#include <QScrollBar>
#include <QDebug>

InvertedPdfView::InvertedPdfView(QWidget *parent)
    : QPdfView(parent),
      m_invert(false)
{
    // Helps avoid background repaints that can cause recursion
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
}

bool InvertedPdfView::invertColors() const
{
    return m_invert;
}

void InvertedPdfView::setInvertColors(bool invert)
{
    if (m_invert != invert) {
        m_invert = invert;
        update(); // Force a repaint with new setting
    }
}

void InvertedPdfView::paintEvent(QPaintEvent *event)
{
    // If we are NOT in dark mode, do normal painting
    if (!m_invert) {
        QPdfView::paintEvent(event);
        return;
    }

    // 1) Let the default QPdfView rendering happen first.
    //    Then we will “invert” what was just drawn.
    QPdfView::paintEvent(event);

    // 2) Over-paint the viewport with a difference composition
    //    so that bright goes dark, dark goes bright, etc.
    QPainter painter(viewport());
    painter.setCompositionMode(QPainter::CompositionMode_Difference);
    // Fill with white, so black <-> white, etc.
    painter.fillRect(viewport()->rect(), Qt::white);
} 