#include "filepreview.h"
#include <QVBoxLayout>
#include <QImageReader>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDir>

FilePreview::FilePreview(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void FilePreview::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

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

    // PDF view
    pdfDocument = new QPdfDocument(this);
    pdfView = new QPdfView(this);
    pdfView->setDocument(pdfDocument);
    pdfView->setPageMode(QPdfView::PageMode::SinglePage);
    pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);
    pdfView->hide();

    layout->addWidget(scrollArea);
    layout->addWidget(pdfView);
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
    // Show PDF preview
    scrollArea->hide();
    pdfView->show();

    // Load PDF document
    pdfDocument->load(filePath);
    if (pdfDocument->status() == QPdfDocument::Status::Error) {
        imageLabel->setText(tr("Cannot load PDF %1").arg(filePath));
        scrollArea->show();
        pdfView->hide();
        return;
    }
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