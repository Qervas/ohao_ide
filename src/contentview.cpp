#include "contentview.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QUrl>
#include <QIcon>
#include <QStyle>
#include <QFileInfo>
#include <QWebEngineView>

ContentView::ContentView(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void ContentView::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Create tab widget
    tabs = new QTabWidget(this);
    tabs->setTabsClosable(true);
    tabs->setMovable(true);
    tabs->setDocumentMode(true);

    mainLayout->addWidget(tabs);

    // Connect close tab signal
    connect(tabs, &QTabWidget::tabCloseRequested, this, &ContentView::closeTab);
}

void ContentView::loadFile(const QString &filePath) {
    // Check if file is already open
    if (QWidget *existing = findTabByPath(filePath)) {
        tabs->setCurrentWidget(existing);
        return;
    }

    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.isFile()) {
        FilePreview *preview = new FilePreview(this);
        preview->loadFile(filePath);
        preview->setProperty("filePath", filePath);

        int index = tabs->addTab(preview, fileInfo.fileName());
        tabs->setCurrentIndex(index);
        currentPath = filePath;
        currentTitle = fileInfo.fileName();
    }
}

void ContentView::loadWebContent(const QString &url) {
    // Check if URL is already open
    if (QWidget *existing = findTabByPath(url)) {
        tabs->setCurrentWidget(existing);
        return;
    }

    // Create new web view
    QWebEngineView *webView = new QWebEngineView(this);
    webView->setUrl(QUrl(url));
    webView->setProperty("filePath", url);

    // Add to tabs
    int index = tabs->addTab(webView, "Web");
    tabs->setCurrentIndex(index);

    // Update tab title when page is loaded
    connect(webView, &QWebEngineView::titleChanged, this, [this, webView](const QString &title) {
        int index = tabs->indexOf(webView);
        if (index != -1) {
            tabs->setTabText(index, title);
        }
    });
}

bool ContentView::isWebContent(const QString &path) const {
    return path.startsWith("http://") || path.startsWith("https://");
}

void ContentView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void ContentView::updateTheme() {
    // Theme update logic if needed
}

void ContentView::closeTab(int index) {
    QWidget *widget = tabs->widget(index);
    tabs->removeTab(index);
    delete widget;
}

QWidget* ContentView::findTabByPath(const QString &path) {
    for (int i = 0; i < tabs->count(); ++i) {
        QWidget *widget = tabs->widget(i);
        QString tabPath = widget->property("filePath").toString();
        if (tabPath == path) {
            return widget;
        }
    }
    return nullptr;
}
