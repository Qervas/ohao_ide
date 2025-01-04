#pragma once
#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QPdfDocument>
#include <QPdfView>
#include <QResizeEvent>

class FilePreview : public QWidget {
    Q_OBJECT

public:
    explicit FilePreview(QWidget *parent = nullptr);
    void previewFile(const QString &filePath);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QScrollArea *scrollArea;
    QLabel *imageLabel;
    QPdfView *pdfView;
    QPdfDocument *pdfDocument;

    void setupUI();
    void previewImage(const QString &filePath);
    void previewPDF(const QString &filePath);
    void clearPreview();
    void updateImageDisplay(const QImage &image);
    bool isImageFile(const QString &filePath);
    bool isPDFFile(const QString &filePath);
}; 