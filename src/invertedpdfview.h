#pragma once

#include <QPdfView>
#include <QWidget>

class InvertedPdfView : public QPdfView {
  Q_OBJECT
public:
  explicit InvertedPdfView(QWidget *parent = nullptr);

  bool invertColors() const;
  void setInvertColors(bool invert);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  bool m_invert;
}; 