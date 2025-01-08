#include "filepreview.h"
#include <QApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QPdfSearchModel>
#include "invertedpdfview.h"
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

using Role = QPdfBookmarkModel::Role;

FilePreview::FilePreview(QWidget *parent)
    : QWidget(parent), isDarkMode(false) {
  setupUI();
}

FilePreview::~FilePreview() {
  cleanup();
}

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
  setupPDFTools();
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

  // Zoom shortcuts
  QShortcut *zoomInShortcut = new QShortcut(QKeySequence::ZoomIn, this);
  connect(zoomInShortcut, &QShortcut::activated, this, &FilePreview::zoomIn);

  QShortcut *zoomOutShortcut = new QShortcut(QKeySequence::ZoomOut, this);
  connect(zoomOutShortcut, &QShortcut::activated, this, &FilePreview::zoomOut);

  // Additional Ctrl++ and Ctrl+- shortcuts
  QShortcut *zoomInPlusShortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), this);
  connect(zoomInPlusShortcut, &QShortcut::activated,
          this, &FilePreview::zoomIn);

  QShortcut *zoomOutMinusShortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this);
  connect(zoomOutMinusShortcut, &QShortcut::activated,
          this, &FilePreview::zoomOut);

  // Connect signals
  connect(bookmarkView, &QTreeView::clicked, this,
          &FilePreview::handleBookmarkClicked);
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
  new QShortcut(QKeySequence::FindNext, this, this,
                &FilePreview::handleSearchNext);
  new QShortcut(QKeySequence::FindPrevious, this, this,
                &FilePreview::handleSearchPrev);
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

  // Zoom controls
  zoomCombo = new QComboBox(this);
  zoomCombo->addItems({"50%", "75%", "100%", "125%", "150%", "200%", "300%"});
  zoomCombo->setCurrentText("100%");
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
  connect(pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &FilePreview::handlePageChange);

  connect(zoomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &FilePreview::handleZoomChange);

  connect(searchEdit, &QLineEdit::returnPressed,
          this, &FilePreview::handleSearch);

  connect(searchPrev, &QAction::triggered,
          this, &FilePreview::handleSearchPrev);

  connect(searchNext, &QAction::triggered,
          this, &FilePreview::handleSearchNext);
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

  imageLabel->setScaledContents(false);

  updateImageDisplay();
  stack->setCurrentWidget(imageLabel);
  toolbar->setVisible(true);
}

void FilePreview::updateImageDisplay() {
  if (originalPixmap.isNull()) {
    return;
  }
  QSize scaledSize = originalPixmap.size() * currentZoomFactor;
  QPixmap scaledPixmap = originalPixmap.scaled(
      scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  imageLabel->setPixmap(scaledPixmap);
}

void FilePreview::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  updateImageDisplay();
}

void FilePreview::zoomIn() {
  if (stack->currentWidget() == imageLabel) {
    currentZoomFactor *= 1.2;
    updateImageDisplay();
  } else if (pdfView && stack->currentWidget() == pdfView) {
    int currentIndex = zoomCombo->currentIndex();
    if (currentIndex < zoomCombo->count() - 1) {
      handleZoomChange(currentIndex + 1);
    }
  }
}

void FilePreview::zoomOut() {
  if (stack->currentWidget() == imageLabel) {
    currentZoomFactor *= 0.8;
    updateImageDisplay();
  } else if (pdfView && stack->currentWidget() == pdfView) {
    int currentIndex = zoomCombo->currentIndex();
    if (currentIndex > 0) {
      handleZoomChange(currentIndex - 1);
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

void FilePreview::handleZoomChange(int index) {
  if (index < 0 || index >= zoomCombo->count()) {
    return;
  }
  QString zoomText = zoomCombo->itemText(index);
  zoomText.chop(1); // Remove '%'
  qreal zoomFactor = zoomText.toDouble() / 100.0;

  if (auto navigator = pdfView->pageNavigator()) {
    navigator->update(navigator->currentPage(),
                      navigator->currentLocation(),
                      zoomFactor);
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

void FilePreview::handleSearchNext() {
  searchDocument(true);
}

void FilePreview::handleSearchPrev() {
  searchDocument(false);
}

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
    disconnect(navigator, &QPdfPageNavigator::currentPageChanged,
               this, nullptr);

    connect(navigator, &QPdfPageNavigator::currentPageChanged,
            this, [this](int page) {
              pageSpinBox->blockSignals(true);
              pageSpinBox->setValue(page + 1);
              pageSpinBox->blockSignals(false);
            });

    connect(navigator, &QPdfPageNavigator::currentZoomChanged,
            this, [this](qreal zoom) {
              int percent = int(zoom * 100);
              int idx = zoomCombo->findText(QString::number(percent) + "%");
              if (idx >= 0) {
                zoomCombo->setCurrentIndex(idx);
              }
            });
  }
}

void FilePreview::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    if (stack->currentWidget() == imageLabel) {
      // For images
      qreal factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
      currentZoomFactor *= factor;
      updateImageDisplay();
    } else if (stack->currentWidget() == pdfView) {
      // For PDFs
      int idx = zoomCombo->currentIndex();
      if (event->angleDelta().y() > 0) {
        if (idx < zoomCombo->count() - 1) {
          handleZoomChange(idx + 1);
        }
      } else {
        if (idx > 0) {
          handleZoomChange(idx - 1);
        }
      }
    }
    event->accept();
    return;
  }
  QWidget::wheelEvent(event);
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
