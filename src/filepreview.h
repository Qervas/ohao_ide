#pragma once

#include <QAbstractItemModel>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfSearchModel>
#include <QPdfView>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolBar>
#include <QTreeView>
#include <QWidget>

class FilePreview : public QWidget {
  Q_OBJECT
public:
  explicit FilePreview(QWidget *parent = nullptr);
  ~FilePreview();
  void cleanup();
  void loadFile(const QString &filePath);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private slots:
  void handlePageChange(int page);
  void handleZoomChange(int zoom);
  void handleSearch();
  void handleSearchNext();
  void handleSearchPrev();
  void handleBookmarkClicked(const QModelIndex &index);
  void updatePageInfo();
  void zoomIn();
  void zoomOut();

private:
  // Core widgets
  QSplitter *mainSplitter;
  QStackedWidget *stack;

  // PDF components
  QPdfDocument *pdfDoc;
  QPdfView *pdfView;
  QPdfBookmarkModel *bookmarkModel;
  QTreeView *bookmarkView;
  QPdfSearchModel *searchModel;

  // Image components
  QLabel *imageLabel;

  // Toolbar and controls
  QToolBar *toolbar;
  QSpinBox *pageSpinBox;
  QLabel *pageTotalLabel;
  QComboBox *zoomCombo;
  QLineEdit *searchEdit;

  // Current state
  QString currentSearchText;
  int currentSearchPage;

  // Image viewing members
  QPixmap originalPixmap;
  qreal currentZoomFactor;

  void setupUI();
  void setupPDFTools();
  void loadPDF(const QString &filePath);
  void loadImage(const QString &filePath);
  void searchDocument(bool forward = true);
  void updateImageDisplay();
};
