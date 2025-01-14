#include "views/content/contentview.h"
#include "views/browser/browserview.h"
#include "views/content/filepreview.h"
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QShortcut>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineView>

ContentView::ContentView(QWidget *parent) : DockWidgetBase(parent) {
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
  tabs->setElideMode(Qt::ElideMiddle);

  // Set a minimum tab width
  tabs->setStyleSheet("QTabBar::tab { min-width: 100px; max-width: 200px; }");
  mainLayout->addWidget(tabs);

  // Connect close tab signal
  connect(tabs, &QTabWidget::tabCloseRequested, this, &ContentView::closeTab);

  // Add Ctrl+W shortcut to close current tab
  QShortcut *closeTabShortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
  connect(closeTabShortcut, &QShortcut::activated, this, [this]() {
    if (tabs->count() > 0) {
      closeTab(tabs->currentIndex());
    }
  });
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

    // Truncate filename if it's too long (more than 20 characters)
    QString displayName = fileInfo.fileName();
    if (displayName.length() > 20) {
      displayName = displayName.left(17) + "...";
    }

    int index = tabs->addTab(preview, displayName);
    tabs->setTabToolTip(index, fileInfo.fileName()); // Show full name on hover
    tabs->setCurrentIndex(index);
    currentPath = filePath;
    currentTitle = fileInfo.fileName();
  }
}

void ContentView::loadWebContent(const QString &url) {
  // Create new browser view
  BrowserView *browser = new BrowserView(this);
  browser->setProperty("filePath", url);

  // Add to tabs
  int index = tabs->addTab(browser, "New Tab");
  tabs->setCurrentIndex(index);

  // Load URL and update tab when title changes
  browser->loadUrl(url);

  // Connect signals
  connect(browser, &BrowserView::titleChanged, this,
          [this, browser](const QString &title) {
            int index = tabs->indexOf(browser);
            if (index != -1) {
              QString displayTitle = title;
              if (displayTitle.length() > 20) {
                displayTitle = displayTitle.left(17) + "...";
              }
              tabs->setTabText(index, displayTitle);
              tabs->setTabToolTip(index, title);
            }
          });

  // Connect new tab signal
  connect(browser, &BrowserView::createTab, this, &ContentView::handleNewTab);
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

QWidget *ContentView::findTabByPath(const QString &path) {
  for (int i = 0; i < tabs->count(); ++i) {
    QWidget *widget = tabs->widget(i);
    QString tabPath = widget->property("filePath").toString();
    if (tabPath == path) {
      return widget;
    }
  }
  return nullptr;
}

void ContentView::handleNewTab(const QUrl &url) {
  loadWebContent(url.toString());
}

QString ContentView::getCurrentUrl() const {
  if (QWidget *widget = tabs->currentWidget()) {
    if (BrowserView *browser = qobject_cast<BrowserView *>(widget)) {
      return browser->currentUrl();
    }
  }
  return QString();
}

QString ContentView::getCurrentFilePath() const {
  if (QWidget *widget = tabs->currentWidget()) {
    if (qobject_cast<FilePreview *>(widget)) {
      return widget->property("filePath").toString();
    }
    if (qobject_cast<BrowserView *>(widget)) {
      return widget->property("filePath").toString();
    }
  }
  return currentPath;
}

QList<ContentView::TabState> ContentView::getTabStates() const {
  QList<TabState> states;
  for (int i = 0; i < tabs->count(); ++i) {
    TabState state;
    QWidget *widget = tabs->widget(i);

    if (BrowserView *browser = qobject_cast<BrowserView *>(widget)) {
      state.type = "web";
      state.url = browser->currentUrl();
      state.title = tabs->tabText(i);
      states.append(state);
    } else if (FilePreview *preview = qobject_cast<FilePreview *>(widget)) {
      state.type = "file";
      state.filePath = widget->property("filePath").toString();
      state.title = tabs->tabText(i);
      states.append(state);
    }
  }
  return states;
}

void ContentView::restoreTabStates(const QList<TabState> &states) {
  // Clear existing tabs
  while (tabs->count() > 0) {
    QWidget *widget = tabs->widget(0);
    tabs->removeTab(0);
    delete widget;
  }

  // Restore tabs
  for (const TabState &state : states) {
    if (state.type == "web") {
      loadWebContent(state.url);
    } else if (state.type == "file") {
      loadFile(state.filePath);
    }
  }
}
