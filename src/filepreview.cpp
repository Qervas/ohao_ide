#include "filepreview.h"
#include <QVBoxLayout>
#include <QImageReader>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDir>
#include <QWheelEvent>
#include <QToolButton>
#include <QApplication>
#include <QStyle>
#include <QtPdf/QPdfDocument>
#include <QtPdf/QPdfPageNavigator>

FilePreview::FilePreview(QWidget *parent) : QWidget(parent), isDarkMode(false) {
    setupUI();
}

void FilePreview::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Initialize PDF components first
    pdfDocument = new QPdfDocument(this);
    pdfView = new QPdfView(this);
    pdfView->setDocument(pdfDocument);
    pdfView->setPageMode(QPdfView::PageMode::MultiPage);
    pdfView->installEventFilter(this);

    // Setup PDF toolbar
    pdfToolBar = new QToolBar(this);
    pdfToolBar->setFloatable(false);
    pdfToolBar->setMovable(false);
    pdfToolBar->setIconSize(QSize(16, 16));
    setupPdfControls();
    pdfToolBar->hide();
    layout->addWidget(pdfToolBar);

    // Scroll area for images
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Image label
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    scrollArea->setWidget(imageLabel);
    
    // Initialize hide timer for toolbar
    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    hideTimer->setInterval(2000);
    connect(hideTimer, &QTimer::timeout, this, &FilePreview::hideToolBar);

    layout->addWidget(scrollArea);
    layout->addWidget(pdfView);

    // Apply initial theme
    updatePdfTheme();
}

void FilePreview::setupPdfControls() {
    // Navigation buttons
    auto prevButton = pdfToolBar->addAction(QApplication::style()->standardIcon(QStyle::SP_MediaSkipBackward),
                                          tr("Previous Page"));
    auto nextButton = pdfToolBar->addAction(QApplication::style()->standardIcon(QStyle::SP_MediaSkipForward),
                                          tr("Next Page"));
    
    // Page navigation
    pageSpinBox = new QSpinBox(this);
    pageSpinBox->setMinimum(1);
    pageTotalLabel = new QLabel(this);
    pdfToolBar->addWidget(pageSpinBox);
    pdfToolBar->addWidget(pageTotalLabel);
    
    pdfToolBar->addSeparator();
    
    // Zoom controls
    auto zoomOutAction = pdfToolBar->addAction(QApplication::style()->standardIcon(QStyle::SP_MediaVolumeMuted),
                                             tr("Zoom Out"));
    
    zoomComboBox = new QComboBox(this);
    for (int zoom : zoomLevels) {
        zoomComboBox->addItem(QString("%1%").arg(zoom), zoom);
    }
    zoomComboBox->setCurrentText("100%");
    zoomComboBox->setEditable(true);
    pdfToolBar->addWidget(zoomComboBox);
    
    auto zoomInAction = pdfToolBar->addAction(QApplication::style()->standardIcon(QStyle::SP_MediaVolume),
                                            tr("Zoom In"));

    // Connect signals
    connect(prevButton, &QAction::triggered, [this]() {
        if (pageSpinBox->value() > 1) {
            pageSpinBox->setValue(pageSpinBox->value() - 1);
        }
    });
    
    connect(nextButton, &QAction::triggered, [this]() {
        if (pageSpinBox->value() < pageSpinBox->maximum()) {
            pageSpinBox->setValue(pageSpinBox->value() + 1);
        }
    });
    
    connect(pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FilePreview::pageChanged);
    
    connect(zoomOutAction, &QAction::triggered, this, &FilePreview::zoomOut);
    connect(zoomInAction, &QAction::triggered, this, &FilePreview::zoomIn);
    
    connect(zoomComboBox, &QComboBox::currentTextChanged, [this](const QString &text) {
        QString numStr = text;
        numStr.remove('%');
        bool ok;
        int zoom = numStr.toInt(&ok);
        if (ok && zoom >= 25 && zoom <= 400) {
            currentZoomLevel = zoom;
            pdfView->setZoomFactor(zoom / 100.0);
            pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
        }
    });

    pdfToolBar->addSeparator();
    
    // Add dark mode toggle button
    auto darkModeAction = pdfToolBar->addAction(QApplication::style()->standardIcon(QStyle::SP_DialogResetButton),
                                              tr("Toggle Dark Mode"));
    connect(darkModeAction, &QAction::triggered, this, &FilePreview::toggleDarkMode);
}

bool FilePreview::eventFilter(QObject *obj, QEvent *event) {
    if (obj == pdfView) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
                updateZoomLevel(wheelEvent->angleDelta().y());
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            showToolBar();
            hideTimer->start();
        } else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_D) {
                toggleDarkMode();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FilePreview::updateZoomLevel(int delta) {
    int index = zoomLevels.indexOf(currentZoomLevel);
    if (delta > 0 && index < zoomLevels.size() - 1) {
        currentZoomLevel = zoomLevels[index + 1];
    } else if (delta < 0 && index > 0) {
        currentZoomLevel = zoomLevels[index - 1];
    }
    zoomComboBox->setCurrentText(QString("%1%").arg(currentZoomLevel));
}

void FilePreview::previewFile(const QString &filePath) {
    clearPreview();

    if (filePath.isEmpty()) {
        return;
    }

    // Show the file name at the top
    QString fileName = QFileInfo(filePath).fileName();
    setWindowTitle(fileName);

    if (isImageFile(filePath)) {
        previewImage(filePath);
    } else if (isPDFFile(filePath)) {
        previewPDF(filePath);
    }
}

void FilePreview::previewImage(const QString &filePath) {
    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    QImage image = reader.read();

    if (image.isNull()) {
        imageLabel->setText(tr("Cannot load image %1").arg(filePath));
        return;
    }

    // Show image preview
    scrollArea->show();
    pdfView->hide();
    
    // Scale image to fit the widget while maintaining aspect ratio
    updateImageDisplay(image);
}

void FilePreview::updateImageDisplay(const QImage &image) {
    QSize viewSize = scrollArea->viewport()->size();
    QPixmap pixmap = QPixmap::fromImage(image);
    
    if (image.width() > viewSize.width() || image.height() > viewSize.height()) {
        // Scale down if image is larger than viewport
        imageLabel->setPixmap(pixmap.scaled(viewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // Show original size if image is smaller
        imageLabel->setPixmap(pixmap);
    }
}

void FilePreview::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    // Re-scale image if one is displayed
    if (!imageLabel->pixmap(Qt::ReturnByValue).isNull()) {
        QPixmap currentPixmap = imageLabel->pixmap(Qt::ReturnByValue);
        updateImageDisplay(currentPixmap.toImage());
    }
}

void FilePreview::previewPDF(const QString &filePath) {
    scrollArea->hide();
    pdfView->show();
    pdfToolBar->show();

    pdfDocument->load(filePath);
    if (pdfDocument->status() == QPdfDocument::Status::Error) {
        imageLabel->setText(tr("Cannot load PDF %1").arg(filePath));
        scrollArea->show();
        pdfView->hide();
        pdfToolBar->hide();
        return;
    }

    // Update page navigation controls
    pageSpinBox->setMaximum(pdfDocument->pageCount());
    pageSpinBox->setValue(1);
    pageTotalLabel->setText(tr(" of %1").arg(pdfDocument->pageCount()));
    
    // Reset zoom level
    resetZoom();
    
    // Set initial zoom mode to fit width
    pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);

    // Apply current theme
    updatePdfTheme();
}

void FilePreview::clearPreview() {
    imageLabel->clear();
    pdfDocument->close();
}

bool FilePreview::isImageFile(const QString &filePath) {
    QMimeDatabase db;
    QString mimeType = db.mimeTypeForFile(filePath).name();
    return mimeType.startsWith("image/");
}

bool FilePreview::isPDFFile(const QString &filePath) {
    QMimeDatabase db;
    QString mimeType = db.mimeTypeForFile(filePath).name();
    return mimeType == "application/pdf";
}

void FilePreview::zoomIn() {
    int index = zoomLevels.indexOf(currentZoomLevel);
    if (index < zoomLevels.size() - 1) {
        currentZoomLevel = zoomLevels[index + 1];
        zoomComboBox->setCurrentText(QString("%1%").arg(currentZoomLevel));
    }
}

void FilePreview::zoomOut() {
    int index = zoomLevels.indexOf(currentZoomLevel);
    if (index > 0) {
        currentZoomLevel = zoomLevels[index - 1];
        zoomComboBox->setCurrentText(QString("%1%").arg(currentZoomLevel));
    }
}

void FilePreview::resetZoom() {
    currentZoomLevel = 100;
    zoomComboBox->setCurrentText("100%");
}

void FilePreview::pageChanged(int page) {
    if (pdfView && pdfView->pageNavigator()) {
        pdfView->pageNavigator()->jump(page - 1, QPoint(0, 0));
    }
}

void FilePreview::showToolBar() {
    pdfToolBar->show();
}

void FilePreview::hideToolBar() {
    pdfToolBar->hide();
}

void FilePreview::toggleDarkMode() {
    isDarkMode = !isDarkMode;
    updatePdfTheme();
}

void FilePreview::updatePdfTheme() {
    if (!pdfView) {
        return;
    }

    if (isDarkMode) {
        // Dark mode colors
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        
        pdfView->setPalette(darkPalette);
        pdfView->setStyleSheet(
            "QPdfView {"
            "   background-color: #232323;"
            "   color: #ffffff;"
            "}"
            "QPdfView::page {"
            "   background-color: #232323;"
            "   border: 1px solid #404040;"
            "}"
        );
    } else {
        // Light mode colors
        QPalette lightPalette;
        lightPalette.setColor(QPalette::Window, Qt::white);
        lightPalette.setColor(QPalette::WindowText, Qt::black);
        lightPalette.setColor(QPalette::Base, Qt::white);
        lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
        lightPalette.setColor(QPalette::Text, Qt::black);
        lightPalette.setColor(QPalette::Button, Qt::white);
        lightPalette.setColor(QPalette::ButtonText, Qt::black);
        
        pdfView->setPalette(lightPalette);
        pdfView->setStyleSheet(
            "QPdfView {"
            "   background-color: #ffffff;"
            "   color: #000000;"
            "}"
            "QPdfView::page {"
            "   background-color: #ffffff;"
            "   border: 1px solid #e0e0e0;"
            "}"
        );
    }
} 