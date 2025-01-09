#ifndef FILEPREVIEW_H
#define FILEPREVIEW_H

#include "invertedpdfview.h"
#include <QAbstractItemModel>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfSearchModel>
#include <QSettings>
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
  void handleZoom(qreal factor);

private:
  // Core widgets
  QSplitter *mainSplitter;
  QStackedWidget *stack;

  // PDF components
  QPdfDocument *pdfDoc;
  InvertedPdfView *pdfView;
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

  bool isDarkMode;
  QAction *toggleDarkModeAction;
  void toggleDarkMode();
  void updatePdfDarkMode();

  QSettings settings;
  qreal customZoomFactor;
  static constexpr qreal DEFAULT_ZOOM_FACTOR = 1.2;

  static constexpr qreal ZOOM_FACTOR = 1.2;
  static constexpr qreal MIN_ZOOM = 0.1;
  static constexpr qreal MAX_ZOOM = 5.0;

  void setupUI();
  void setupPDFTools();
  void loadPDF(const QString &filePath);
  void loadImage(const QString &filePath);
  void searchDocument(bool forward = true);
  void updateImageDisplay();
  void updateZoomComboText(qreal zoomFactor);
};

#endif // FILEPREVIEW_H
