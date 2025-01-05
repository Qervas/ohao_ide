#pragma once

#include <QWidget>
#include <QPdfDocument>
#include <QPdfView>
#include <QLabel>
#include <QStackedWidget>
#include <QToolBar>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPdfBookmarkModel>
#include <QTreeView>
#include <QSplitter>
#include <QPdfSearchModel>
#include <QPdfPageNavigator>
#include <QAbstractItemModel>

class FilePreview : public QWidget {
    Q_OBJECT
public:
    explicit FilePreview(QWidget *parent = nullptr);
    ~FilePreview();
    void cleanup();
    void loadFile(const QString &filePath);

private slots:
    void handlePageChange(int page);
    void handleZoomChange(int zoom);
    void handleSearch();
    void handleSearchNext();
    void handleSearchPrev();
    void handleBookmarkClicked(const QModelIndex &index);
    void updatePageInfo();

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

    void setupUI();
    void setupPDFTools();
    void loadPDF(const QString &filePath);
    void loadImage(const QString &filePath);
    void searchDocument(bool forward = true);
};
