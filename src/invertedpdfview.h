#pragma once

#include <QPdfView>
#include <QWidget>

class InvertedPdfView : public QPdfView {
  Q_OBJECT
public:
  explicit InvertedPdfView(QWidget *parent = nullptr);

  bool invertColors() const;
  void setInvertColors(bool invert);
  void handleWheelEvent(QWheelEvent *event);
  void setCustomZoomFactor(qreal factor) { m_zoomFactor = factor; }

protected:
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  bool m_invert;
  qreal m_zoomFactor = 1.2;
};
