#pragma once
#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QPdfDocument>
#include <QPdfView>
#include <QResizeEvent>
#include <QToolBar>
#include <QSpinBox>
#include <QComboBox>
#include <QTimer>

class FilePreview : public QWidget {
    Q_OBJECT

public:
    explicit FilePreview(QWidget *parent = nullptr);
    void previewFile(const QString &filePath);
    void loadFile(const QString &filePath);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void pageChanged(int page);
    void hideToolBar();
    void showToolBar();
    void toggleDarkMode();

private:
    QScrollArea *scrollArea;
    QLabel *imageLabel;
    QPdfView *pdfView;
    QPdfDocument *pdfDocument;
    QToolBar *pdfToolBar;
    QSpinBox *pageSpinBox;
    QLabel *pageTotalLabel;
    QComboBox *zoomComboBox;
    QTimer *hideTimer;
    bool isDarkMode;
    
    const QList<int> zoomLevels = {25, 50, 75, 100, 125, 150, 200, 300, 400};
    int currentZoomLevel = 100;

    void setupUI();
    void setupPdfControls();
    void previewImage(const QString &filePath);
    void previewPDF(const QString &filePath);
    void clearPreview();
    void updateImageDisplay(const QImage &image);
    bool isImageFile(const QString &filePath);
    bool isPDFFile(const QString &filePath);
    void updateZoomLevel(int delta);
    void updatePdfTheme();
}; 