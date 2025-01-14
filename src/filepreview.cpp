#include "filepreview.h"
#include "invertedpdfview.h"
#include <QApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfSearchModel>
#include <QPdfSelection>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>
#include <qpdfbookmarkmodel.h>
#include <QTimer>

using Role = QPdfBookmarkModel::Role;

FilePreview::FilePreview(QWidget *parent)
    : QWidget(parent), isDarkMode(false), settings("ohao", "ohao_IDE") {
  customZoomFactor =
      settings.value("zoom_factor", DEFAULT_ZOOM_FACTOR).toDouble();
  
  // Setup resize timer
  resizeTimer.setSingleShot(true);
  resizeTimer.setInterval(50); // 50ms delay
  
  setupUI();
  if (pdfView) {
    pdfView->setCustomZoomFactor(customZoomFactor);
  }
}

FilePreview::~FilePreview() { cleanup(); }

void FilePreview::cleanup() {
  if (searchModel) {
    searchModel->setDocument(nullptr);
  }
  if (bookmarkModel) {
    bookmarkModel->setDocument(nullptr);
  }
  if (pdfDoc) {
    pdfDoc->close();
  }
}

void FilePreview::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Create toolbar
  toolbar = new QToolBar(this);
  mainLayout->addWidget(toolbar);

  // Create main splitter
  mainSplitter = new QSplitter(Qt::Horizontal, this);

  // Setup bookmark view
  bookmarkView = new QTreeView(this);
  bookmarkView->setHeaderHidden(true);
  bookmarkView->hide(); // Hidden by default
  mainSplitter->addWidget(bookmarkView);

  // Setup stacked widget for content
  stack = new QStackedWidget(this);
  mainSplitter->addWidget(stack);

  // Setup PDF viewer
  pdfDoc = new QPdfDocument(this);

  pdfView = new InvertedPdfView(this);
  pdfView->setDocument(pdfDoc);
  pdfView->setPageMode(QPdfView::PageMode::MultiPage);
  pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);

  // Create search model
  searchModel = new QPdfSearchModel(this);
  pdfView->setSearchModel(searchModel);

  // Setup bookmark model
  bookmarkModel = new QPdfBookmarkModel(this);
  bookmarkModel->setDocument(pdfDoc);
  bookmarkView->setModel(bookmarkModel);

  // Setup image viewer
  imageLabel = new QLabel(this);
  imageLabel->setAlignment(Qt::AlignCenter);
  imageLabel->setScaledContents(true);

  stack->addWidget(pdfView);
  stack->addWidget(imageLabel);

  mainLayout->addWidget(mainSplitter);

  // Setup zoom combo box first
  zoomCombo = new QComboBox(this);
  zoomCombo->setEditable(true);
  zoomCombo->setInsertPolicy(QComboBox::NoInsert);
  zoomCombo->lineEdit()->setAlignment(Qt::AlignCenter);
  zoomCombo->addItems({"25%", "50%", "75%", "100%", "125%", "150%", "200%", "400%"});
  zoomCombo->setCurrentText("100%");

  // Setup PDF tools after zoom combo is created
  setupPDFTools();

  // Connect zoom combo signals after it's fully set up
  connect(zoomCombo->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
    QString text = zoomCombo->currentText();
    if (!text.endsWith('%'))
      text += '%';
    handleZoomChange(0);
  });

  connect(zoomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &FilePreview::handleZoomChange);

  // Connect PDF view zoom changes to update the combo box
  connect(pdfView, &QPdfView::zoomFactorChanged, this, [this](qreal zoom) {
    int percent = qRound(zoom * 100);
    zoomCombo->setCurrentText(QString::number(percent) + "%");
  });

  // Setup zoom shortcuts
  QList<QKeySequence> zoomInSequences = {
      QKeySequence::ZoomIn, QKeySequence(Qt::CTRL | Qt::Key_Plus),
      QKeySequence(Qt::CTRL | Qt::Key_Equal)
  };

  QList<QKeySequence> zoomOutSequences = {
      QKeySequence::ZoomOut, QKeySequence(Qt::CTRL | Qt::Key_Minus)
  };

  for (const auto &sequence : zoomInSequences) {
    QShortcut *zoomInShortcut = new QShortcut(sequence, this);
    connect(zoomInShortcut, &QShortcut::activated, this, &FilePreview::zoomIn);
  }

  for (const auto &sequence : zoomOutSequences) {
    QShortcut *zoomOutShortcut = new QShortcut(sequence, this);
    connect(zoomOutShortcut, &QShortcut::activated, this, &FilePreview::zoomOut);
  }

  // Connect other signals
  connect(bookmarkView, &QTreeView::clicked, this, &FilePreview::handleBookmarkClicked);
  connect(pdfDoc, &QPdfDocument::statusChanged, this,
          [this](QPdfDocument::Status status) {
            if (status == QPdfDocument::Status::Ready) {
              updatePageInfo();
            }
          });
  connect(pdfView->pageNavigator(), &QPdfPageNavigator::currentPageChanged,
          this, &FilePreview::handlePageChange);

  // Setup shortcuts
  new QShortcut(QKeySequence::Find, this, this, &FilePreview::handleSearch);
  new QShortcut(QKeySequence::FindNext, this, this, &FilePreview::handleSearchNext);
  new QShortcut(QKeySequence::FindPrevious, this, this, &FilePreview::handleSearchPrev);
  new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), this,
                [this]() { handleZoomChange(zoomCombo->currentIndex() + 1); });
  new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this,
                [this]() { handleZoomChange(zoomCombo->currentIndex() - 1); });
}

void FilePreview::setupPDFTools() {
  // Navigation buttons
  QAction *prevPage = toolbar->addAction(
      style()->standardIcon(QStyle::SP_ArrowLeft), "Previous Page");
  connect(prevPage, &QAction::triggered,
          [this]() { pageSpinBox->setValue(pageSpinBox->value() - 1); });

  pageSpinBox = new QSpinBox(this);
  pageSpinBox->setMinimum(1);
  toolbar->addWidget(pageSpinBox);

  pageTotalLabel = new QLabel(this);
  toolbar->addWidget(pageTotalLabel);

  QAction *nextPage = toolbar->addAction(
      style()->standardIcon(QStyle::SP_ArrowRight), "Next Page");
  connect(nextPage, &QAction::triggered,
          [this]() { pageSpinBox->setValue(pageSpinBox->value() + 1); });

  toolbar->addSeparator();

  // Add zoom combo box to toolbar
  toolbar->addWidget(zoomCombo);

  toolbar->addSeparator();

  // Search controls
  searchEdit = new QLineEdit(this);
  searchEdit->setPlaceholderText("Search...");
  searchEdit->setMaximumWidth(200);
  toolbar->addWidget(searchEdit);

  QAction *searchPrev =
      toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowUp), "Previous");
  QAction *searchNext =
      toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowDown), "Next");

  toolbar->addSeparator();

  // TOC toggle
  QAction *tocAction = toolbar->addAction("TOC");
  connect(tocAction, &QAction::triggered,
          [this]() { bookmarkView->setVisible(!bookmarkView->isVisible()); });

  // Add dark mode toggle button
  toolbar->addSeparator();
  toggleDarkModeAction = toolbar->addAction(tr("Dark Mode"));
  toggleDarkModeAction->setCheckable(true);
  toggleDarkModeAction->setChecked(false);
  connect(toggleDarkModeAction, &QAction::triggered, this,
          &FilePreview::toggleDarkMode);

  // Connect signals
  connect(pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &FilePreview::handlePageChange);

  connect(searchEdit, &QLineEdit::returnPressed, this,
          &FilePreview::handleSearch);

  connect(searchPrev, &QAction::triggered, this,
          &FilePreview::handleSearchPrev);

  connect(searchNext, &QAction::triggered, this,
          &FilePreview::handleSearchNext);
}

void FilePreview::loadFile(const QString &filePath) {
  QFileInfo fileInfo(filePath);
  QString suffix = fileInfo.suffix().toLower();

  if (suffix == "pdf") {
    // Clear previous document state
    if (pdfDoc) {
      pdfDoc->close();
    }
    loadPDF(filePath);
  } else if (QStringList{"jpg", "jpeg", "png", "gif", "bmp"}.contains(suffix)) {
    loadImage(filePath);
  }
}

void FilePreview::loadPDF(const QString &filePath) {
  // Load the document
  QPdfDocument::Error error = pdfDoc->load(filePath);
  if (error != QPdfDocument::Error::None) {
    qWarning() << "Failed to load PDF:" << filePath << "Error:" << error;
    return;
  }

  // Setup document and view
  pdfView->setDocument(pdfDoc);

  // Set initial view settings
  pdfView->setPageMode(QPdfView::PageMode::MultiPage);
  pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);

  // Update models - do this after document is loaded
  if (searchModel) {
    searchModel->setDocument(pdfDoc);
    pdfView->setSearchModel(searchModel);
  }
  if (bookmarkModel) {
    bookmarkModel->setDocument(pdfDoc);
  }

  // Show PDF view
  stack->setCurrentWidget(pdfView);
  toolbar->setVisible(true);

  // Reset search state
  currentSearchText.clear();
  currentSearchPage = 0;

  // Update UI
  updatePageInfo();

  // Update dark mode state
  updatePdfDarkMode();

  // Initial page navigation
  if (auto navigator = pdfView->pageNavigator()) {
    navigator->jump(0, QPointF(0, 0), 1.0); // Start at first page
  }
}

void FilePreview::loadImage(const QString &filePath) {
    originalPixmap = QPixmap(filePath);
    currentZoomFactor = 1.0;
    imageOffset = QPoint(0, 0);
    currentImageMode = ImageViewMode::FitToWindow;
    
    // Configure image label
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setMinimumSize(1, 1);
    imageLabel->setScaledContents(false);
    imageLabel->setAlignment(Qt::AlignCenter);
    
    // Show the image
    stack->setCurrentWidget(imageLabel);
    toolbar->setVisible(true);
    
    // Setup image-specific tools
    setupImageTools();
    
    // Initial display
    QTimer::singleShot(0, this, [this]() {
        updateImageViewMode(ImageViewMode::FitToWindow);
    });
}

void FilePreview::fitImageToWindow() {
    if (originalPixmap.isNull()) return;
    
    QSize viewSize = stack->size();
    if (viewSize.isEmpty()) return;
    
    // Calculate the scaling ratio to fit the image within the viewport
    double widthRatio = static_cast<double>(viewSize.width()) / originalPixmap.width();
    double heightRatio = static_cast<double>(viewSize.height()) / originalPixmap.height();
    
    // Use the smaller ratio to ensure the image fits both dimensions
    currentZoomFactor = qMin(widthRatio, heightRatio);
    
    // Cap the zoom at 100% for small images
    if (currentZoomFactor > 1.0) currentZoomFactor = 1.0;
    
    // Update the zoom combo box text and refresh the display
    updateZoomComboText(currentZoomFactor);
    updateImageDisplay();
}

void FilePreview::setupImageTools() {
    // Clear existing tools first
    toolbar->clear();
    
    // Add zoom combo box
    toolbar->addWidget(zoomCombo);
    toolbar->addSeparator();
    
    // Add fit modes
    QAction* fitWindow = toolbar->addAction("Fit Window");
    QAction* fitWidth = toolbar->addAction("Fit Width");
    QAction* fitHeight = toolbar->addAction("Fit Height");
    
    connect(fitWindow, &QAction::triggered, this, [this]() {
        updateImageViewMode(ImageViewMode::FitToWindow);
    });
    connect(fitWidth, &QAction::triggered, this, [this]() {
        updateImageViewMode(ImageViewMode::FitToWidth);
    });
    connect(fitHeight, &QAction::triggered, this, [this]() {
        updateImageViewMode(ImageViewMode::FitToHeight);
    });
    
    // Add dark mode toggle
    toolbar->addSeparator();
    toggleDarkModeAction = toolbar->addAction(tr("Dark Mode"));
    toggleDarkModeAction->setCheckable(true);
    toggleDarkModeAction->setChecked(isDarkMode);
}

void FilePreview::updateImageViewMode(ImageViewMode mode) {
    currentImageMode = mode;
    
    switch (mode) {
        case ImageViewMode::FitToWindow:
            fitImageToWindow();
            break;
        case ImageViewMode::FitToWidth:
            fitImageToWidth();
            break;
        case ImageViewMode::FitToHeight:
            fitImageToHeight();
            break;
        case ImageViewMode::Custom:
            updateImageDisplay();
            break;
    }
}

void FilePreview::fitImageToWidth() {
    if (originalPixmap.isNull()) return;
    
    QSize viewSize = stack->size();
    if (viewSize.isEmpty()) return;
    
    double widthRatio = static_cast<double>(viewSize.width()) / originalPixmap.width();
    currentZoomFactor = widthRatio;
    
    if (currentZoomFactor > 1.0) currentZoomFactor = 1.0;
    
    updateZoomComboText(currentZoomFactor);
    updateImageDisplay();
}

void FilePreview::fitImageToHeight() {
    if (originalPixmap.isNull()) return;
    
    QSize viewSize = stack->size();
    if (viewSize.isEmpty()) return;
    
    double heightRatio = static_cast<double>(viewSize.height()) / originalPixmap.height();
    currentZoomFactor = heightRatio;
    
    if (currentZoomFactor > 1.0) currentZoomFactor = 1.0;
    
    updateZoomComboText(currentZoomFactor);
    updateImageDisplay();
}

void FilePreview::centerImage() {
    if (originalPixmap.isNull()) return;
    
    QSize viewSize = stack->size();
    QSize scaledSize = calculateImageSize(viewSize, originalPixmap.size());
    
    // Center the image in the viewport
    int x = (viewSize.width() - scaledSize.width()) / 2;
    int y = (viewSize.height() - scaledSize.height()) / 2;
    
    imageOffset = QPoint(x, y);
}

QSize FilePreview::calculateImageSize(const QSize &viewportSize, const QSize &imageSize) const {
    QSize scaledSize = imageSize * currentZoomFactor;
    
    switch (currentImageMode) {
        case ImageViewMode::FitToWindow: {
            double widthRatio = static_cast<double>(viewportSize.width()) / imageSize.width();
            double heightRatio = static_cast<double>(viewportSize.height()) / imageSize.height();
            double ratio = qMin(widthRatio, heightRatio);
            if (ratio > 1.0) ratio = 1.0;
            scaledSize = imageSize * ratio;
            break;
        }
        case ImageViewMode::FitToWidth: {
            double ratio = static_cast<double>(viewportSize.width()) / imageSize.width();
            if (ratio > 1.0) ratio = 1.0;
            scaledSize = imageSize * ratio;
            break;
        }
        case ImageViewMode::FitToHeight: {
            double ratio = static_cast<double>(viewportSize.height()) / imageSize.height();
            if (ratio > 1.0) ratio = 1.0;
            scaledSize = imageSize * ratio;
            break;
        }
        case ImageViewMode::Custom:
            // Use the current zoom factor
            break;
    }
    
    return scaledSize;
}

void FilePreview::updateImageDisplay() {
    if (originalPixmap.isNull()) return;
    
    QSize viewSize = stack->size();
    if (viewSize.isEmpty()) return;
    
    // Calculate the scaled size based on current mode and zoom
    QSize scaledSize = calculateImageSize(viewSize, originalPixmap.size());
    
    // Create scaled pixmap with high-quality scaling
    QPixmap scaledPixmap = originalPixmap.scaled(scaledSize,
                                                Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation);
    
    // Center the image
    centerImage();
    
    // Apply dark mode if enabled
    if (isDarkMode) {
        QImage image = scaledPixmap.toImage();
        image.invertPixels();
        scaledPixmap = QPixmap::fromImage(image);
    }
    
    imageLabel->setPixmap(scaledPixmap);
}

void FilePreview::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (stack->currentWidget() == imageLabel) {
        resizeTimer.stop();
        resizeTimer.start(RESIZE_TIMER_DELAY);
        
        disconnect(&resizeTimer, &QTimer::timeout, nullptr, nullptr);
        connect(&resizeTimer, &QTimer::timeout, this, [this]() {
            updateImageViewMode(currentImageMode);
        });
    }
}

void FilePreview::zoomIn() { handleZoom(ZOOM_FACTOR); }

void FilePreview::zoomOut() { handleZoom(1.0 / ZOOM_FACTOR); }

void FilePreview::handleZoom(qreal factor) {
  if (stack->currentWidget() == imageLabel) {
    // Calculate new zoom factor with bounds checking
    qreal newZoom = currentZoomFactor * factor;
    newZoom = qBound(MIN_ZOOM, newZoom, MAX_ZOOM);
    
    if (newZoom != currentZoomFactor) {
      currentZoomFactor = newZoom;
      updateImageDisplay();
      updateZoomComboText(currentZoomFactor);
    }
  } else if (stack->currentWidget() == pdfView) {
    qreal currentZoom = pdfView->zoomFactor();
    qreal newZoom = currentZoom * factor;
    
    if (newZoom >= MIN_ZOOM && newZoom <= MAX_ZOOM) {
      pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
      pdfView->setZoomFactor(newZoom);
      updateZoomComboText(newZoom);
    }
  }
}

void FilePreview::handlePageChange(int page) {
  if (!pdfDoc || page < 1 || page > pdfDoc->pageCount()) {
    return;
  }
  if (auto navigator = pdfView->pageNavigator()) {
    qreal currentZoom = navigator->currentZoom();
    QPointF currentLoc = navigator->currentLocation();
    navigator->jump(page - 1, currentLoc, currentZoom);
  }
}

void FilePreview::handleZoomChange(int) { // Index parameter not needed
  QString text = zoomCombo->currentText();
  text.remove('%'); // Remove '%' if present
  bool ok;
  int percent = text.toInt(&ok);
  if (!ok)
    return;

  qreal factor = percent / 100.0;
  if (factor >= MIN_ZOOM && factor <= MAX_ZOOM) {
    if (stack->currentWidget() == imageLabel) {
      currentZoomFactor = factor;
      updateImageDisplay();
    } else if (stack->currentWidget() == pdfView) {
      pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
      pdfView->setZoomFactor(factor);
    }
  }

  // Ensure the text ends with %
  if (!text.endsWith('%')) {
    zoomCombo->setCurrentText(QString::number(percent) + "%");
  }
}

void FilePreview::handleSearch() {
  currentSearchText = searchEdit->text();
  if (currentSearchText.isEmpty()) {
    return;
  }
  searchModel->setSearchString(currentSearchText);

  if (auto navigator = pdfView->pageNavigator()) {
    currentSearchPage = navigator->currentPage();
  }
  if (searchModel->rowCount(QModelIndex()) > 0) {
    pdfView->setCurrentSearchResultIndex(0);
  }
}

void FilePreview::handleSearchNext() { searchDocument(true); }

void FilePreview::handleSearchPrev() { searchDocument(false); }

void FilePreview::searchDocument(bool forward) {
  if (currentSearchText.isEmpty()) {
    return;
  }
  searchModel->setSearchString(currentSearchText);

  int currentIndex = pdfView->currentSearchResultIndex();
  int totalResults = searchModel->rowCount(QModelIndex());

  if (forward) {
    if (currentIndex < totalResults - 1) {
      pdfView->setCurrentSearchResultIndex(currentIndex + 1);
    } else {
      pdfView->setCurrentSearchResultIndex(0); // wrap
    }
  } else {
    if (currentIndex > 0) {
      pdfView->setCurrentSearchResultIndex(currentIndex - 1);
    } else {
      pdfView->setCurrentSearchResultIndex(totalResults - 1); // wrap
    }
  }
}

void FilePreview::handleBookmarkClicked(const QModelIndex &index) {
  if (!index.isValid() || !pdfDoc) {
    return;
  }
  int page = bookmarkModel->data(index, static_cast<int>(Role::Page)).toInt();
  QPointF location =
      bookmarkModel->data(index, static_cast<int>(Role::Location)).toPointF();
  qreal zoom =
      bookmarkModel->data(index, static_cast<int>(Role::Zoom)).toReal();

  if (auto navigator = pdfView->pageNavigator()) {
    if (zoom <= 0) {
      zoom = navigator->currentZoom();
    }
    navigator->jump(page, location, zoom);
  }
}

void FilePreview::updatePageInfo() {
  if (!pdfDoc || !pdfView) {
    return;
  }
  int pageCount = pdfDoc->pageCount();
  pageSpinBox->setMaximum(pageCount);
  pageTotalLabel->setText(QString(" / %1").arg(pageCount));

  if (auto navigator = pdfView->pageNavigator()) {
    // Avoid duplicate connections
    disconnect(navigator, &QPdfPageNavigator::currentPageChanged, this,
               nullptr);

    connect(navigator, &QPdfPageNavigator::currentPageChanged, this,
            [this](int page) {
              pageSpinBox->blockSignals(true);
              pageSpinBox->setValue(page + 1);
              pageSpinBox->blockSignals(false);
            });

    connect(navigator, &QPdfPageNavigator::currentZoomChanged, this,
            [this](qreal zoom) {
              int percent = int(zoom * 100);
              int idx = zoomCombo->findText(QString::number(percent) + "%");
              if (idx >= 0) {
                zoomCombo->setCurrentIndex(idx);
              }
            });
  }
}

void FilePreview::wheelEvent(QWheelEvent *event) {
  // Let the PDF view handle its own wheel events
  if (stack->currentWidget() == pdfView) {
    pdfView->handleWheelEvent(event);
  } else if (event->modifiers() & Qt::ControlModifier) {
    const int delta = event->angleDelta().y();
    const qreal factor =
        (delta > 0) ? customZoomFactor : 1.0 / customZoomFactor;
    handleZoom(factor);
    event->accept();
  } else {
    QWidget::wheelEvent(event);
  }
}

void FilePreview::toggleDarkMode() {
  isDarkMode = toggleDarkModeAction->isChecked();
  updatePdfDarkMode();
}

void FilePreview::updatePdfDarkMode() {
  if (!pdfView || !pdfDoc) {
    return;
  }
  // Because pdfView is now an InvertedPdfView, we can call setInvertColors().
  pdfView->setInvertColors(isDarkMode);

  // Optional: adjust style sheets or background colors
  if (isDarkMode) {
    // pdfView->setStyleSheet("background: #202020;");
  } else {
    // pdfView->setStyleSheet("");
  }
}

void FilePreview::updateZoomComboText(qreal zoomFactor) {
  int percent = qRound(zoomFactor * 100);
  QString zoomText = QString::number(percent) + "%";

  // Block signals to prevent recursive calls
  zoomCombo->blockSignals(true);
  zoomCombo->setCurrentText(zoomText);
  zoomCombo->blockSignals(false);
}
