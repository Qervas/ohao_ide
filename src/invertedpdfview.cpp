#include "invertedpdfview.h"
#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <qapplication.h>

InvertedPdfView::InvertedPdfView(QWidget *parent)
    : QPdfView(parent), m_invert(false) {
  // Helps avoid background repaints that can cause recursion
  viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
}

bool InvertedPdfView::invertColors() const { return m_invert; }

void InvertedPdfView::setInvertColors(bool invert) {
  if (m_invert != invert) {
    m_invert = invert;
    update(); // Force a repaint with new setting
  }
}

void InvertedPdfView::paintEvent(QPaintEvent *event) {
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

void InvertedPdfView::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    event->accept();
    handleWheelEvent(event);
  } else {
    QPdfView::wheelEvent(event); // Handle normal scrolling
  }
}

void InvertedPdfView::handleWheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    const int delta = event->angleDelta().y();
    if (delta != 0) {
      // Store current position before zoom
      QPointF viewportPos = event->position();
      QScrollBar *hBar = horizontalScrollBar();
      QScrollBar *vBar = verticalScrollBar();

      // Calculate relative position within viewport
      qreal relX = viewportPos.x() / viewport()->width();
      qreal relY = viewportPos.y() / viewport()->height();

      // Calculate scroll fractions
      qreal hFraction = (hBar->value() + viewportPos.x()) /
                        (hBar->maximum() + viewport()->width());
      qreal vFraction = (vBar->value() + viewportPos.y()) /
                        (vBar->maximum() + viewport()->height());

      // Calculate new zoom level using custom factor
      qreal currentZoom = zoomFactor();
      qreal newZoom =
          (delta > 0) ? currentZoom * m_zoomFactor : currentZoom / m_zoomFactor;
      newZoom = qBound(0.1, newZoom, 5.0);

      // Apply the new zoom
      setZoomMode(QPdfView::ZoomMode::Custom);
      setZoomFactor(newZoom);

      // Emit signal for zoom change
      emit zoomFactorChanged(newZoom);

      // Wait for layout to update
      QApplication::processEvents();

      // Restore position
      int newHPos = qRound(hFraction * (hBar->maximum() + viewport()->width()) -
                           viewportPos.x());
      int newVPos =
          qRound(vFraction * (vBar->maximum() + viewport()->height()) -
                 viewportPos.y());

      hBar->setValue(newHPos);
      vBar->setValue(newVPos);

      event->accept();
    }
  }
}
