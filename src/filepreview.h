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
#include <QTimer>

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
  // PDF-related slots
  void handlePageChange(int page);
  void handleBookmarkClicked(const QModelIndex &index);
  void updatePageInfo();
  
  // Common slots for both PDF and Image
  void handleZoomChange(int zoom);
  void handleSearch();
  void handleSearchNext();
  void handleSearchPrev();
  void zoomIn();
  void zoomOut();
  void handleZoom(qreal factor);

private:
  // Image viewing modes
  enum class ImageViewMode {
    FitToWindow,
    FitToWidth,
    FitToHeight,
    Custom
  };

  // Core UI setup
  void setupUI();
  void setupPDFTools();
  void setupImageTools();

  // File loading
  void loadPDF(const QString &filePath);
  void loadImage(const QString &filePath);
  void searchDocument(bool forward = true);

  // Image handling
  void updateImageDisplay();
  void updateZoomComboText(qreal zoomFactor);
  void fitImageToWindow();
  void fitImageToWidth();
  void fitImageToHeight();
  void centerImage();
  void updateImageViewMode(ImageViewMode mode);
  QSize calculateImageSize(const QSize &viewportSize, const QSize &imageSize) const;

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
  QTimer resizeTimer;
  ImageViewMode currentImageMode;
  QPixmap originalPixmap;
  qreal currentZoomFactor;
  QPoint imageOffset;  // For panning support

  // Toolbar and controls
  QToolBar *toolbar;
  QSpinBox *pageSpinBox;
  QLabel *pageTotalLabel;
  QComboBox *zoomCombo;
  QLineEdit *searchEdit;

  // Current state
  QString currentSearchText;
  int currentSearchPage;
  bool isDarkMode;
  QAction *toggleDarkModeAction;
  void toggleDarkMode();
  void updatePdfDarkMode();

  // Settings
  QSettings settings;
  qreal customZoomFactor;
  
  // Constants
  static constexpr qreal DEFAULT_ZOOM_FACTOR = 1.2;
  static constexpr qreal ZOOM_FACTOR = 1.2;
  static constexpr qreal MIN_ZOOM = 0.1;
  static constexpr qreal MAX_ZOOM = 5.0;
  static constexpr int RESIZE_TIMER_DELAY = 50;  // ms
};

#endif // FILEPREVIEW_H
