#include "browserview.h"
#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyle>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <qpainter.h>
#include <qtoolbutton.h>

BrowserView::BrowserView(QWidget *parent) : QWidget(parent) {
  // Create custom profile with persistent storage
  QString dataPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/browser-data";

  // Use default profile for persistence
  profile = QWebEngineProfile::defaultProfile();
  profile->setPersistentStoragePath(dataPath + "/storage");
  profile->setPersistentCookiesPolicy(
      QWebEngineProfile::AllowPersistentCookies);
  profile->setCachePath(dataPath + "/cache");

  // Configure profile settings
  profile->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled,
                                    true);
  profile->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  profile->settings()->setAttribute(
      QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  profile->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled,
                                    true);

  // Set a custom user agent to handle the permissions policy issue
  QString currentAgent = profile->httpUserAgent();
  QString newAgent = currentAgent + " Chrome/120.0.0.0"; // Chrome version
  profile->setHttpUserAgent(newAgent);

  setupUI();
}

void BrowserView::setupUI() {
  layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // Toolbar with dark theme
  toolbar = new QToolBar(this);
  toolbar->setStyleSheet("QToolBar { "
                         "    background: #252526; "
                         "    border: none; "
                         "    spacing: 2px;"
                         "    padding: 2px;"
                         "}"
                         "QToolButton { "
                         "    padding: 6px; "
                         "    border: none; "
                         "    border-radius: 4px;"
                         "    color: #D4D4D4;"
                         "    background: transparent;"
                         "}"
                         "QToolButton:hover { "
                         "    background: #3D3D3D; "
                         "}"
                         "QToolButton:pressed { "
                         "    background: #1E1E1E; "
                         "}"
                         "QToolButton::menu-button { "
                         "    border: none; "
                         "}"
                         "QMenu {"
                         "    background-color: #252526;"
                         "    color: #D4D4D4;"
                         "    border: 1px solid #3D3D3D;"
                         "}"
                         "QMenu::item {"
                         "    padding: 6px 32px 6px 20px;"
                         "}"
                         "QMenu::item:selected {"
                         "    background-color: #3D3D3D;"
                         "}");

  // Navigation actions with custom icons
  backAction = toolbar->addAction(
      QIcon::fromTheme("go-previous",
                       style()->standardIcon(QStyle::SP_ArrowBack)),
      "Back");
  forwardAction = toolbar->addAction(
      QIcon::fromTheme("go-next",
                       style()->standardIcon(QStyle::SP_ArrowForward)),
      "Forward");
  refreshAction = toolbar->addAction(
      QIcon::fromTheme("view-refresh",
                       style()->standardIcon(QStyle::SP_BrowserReload)),
      "Refresh");

  // Convert icons to white
  QList<QAction *> actions = {backAction, forwardAction, refreshAction};
  for (QAction *action : actions) {
    QIcon icon = action->icon();
    QPixmap pixmap = icon.pixmap(24, 24);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), QColor("#D4D4D4"));
    action->setIcon(QIcon(pixmap));
  }

  toolbar->addSeparator();

  // Address bar with dark theme
  addressBar = new QLineEdit(this);
  addressBar->setPlaceholderText("Enter URL or search terms");
  addressBar->setStyleSheet("QLineEdit {"
                            "    background-color: #3D3D3D;"
                            "    color: #D4D4D4;"
                            "    padding: 6px;"
                            "    border: 1px solid #3D3D3D;"
                            "    border-radius: 4px;"
                            "    selection-background-color: #264F78;"
                            "}"
                            "QLineEdit:focus {"
                            "    border-color: #007ACC;"
                            "}"
                            "QLineEdit::placeholder {"
                            "    color: #808080;"
                            "}");
  addressBar->setMinimumWidth(300);
  toolbar->addWidget(addressBar);

  // Menu button
  QToolButton *menuButton = new QToolButton(this);
  menuButton->setIcon(
      QIcon::fromTheme("application-menu",
                       style()->standardIcon(QStyle::SP_TitleBarMenuButton)));
  menuButton->setPopupMode(QToolButton::InstantPopup);

  // Convert menu button icon to white
  QIcon icon = menuButton->icon();
  QPixmap pixmap = icon.pixmap(24, 24);
  QPainter painter(&pixmap);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(pixmap.rect(), QColor("#D4D4D4"));
  menuButton->setIcon(QIcon(pixmap));

  QMenu *menu = new QMenu(this);
  menu->setStyleSheet("QMenu {"
                      "    background-color: #252526;"
                      "    color: #D4D4D4;"
                      "    border: 1px solid #3D3D3D;"
                      "}"
                      "QMenu::item {"
                      "    padding: 6px 32px 6px 20px;"
                      "}"
                      "QMenu::item:selected {"
                      "    background-color: #3D3D3D;"
                      "}"
                      "QMenu::separator {"
                      "    height: 1px;"
                      "    background: #3D3D3D;"
                      "    margin: 4px 0px;"
                      "}");

  clearDataAction = menu->addAction("Clear Browsing Data");
  clearAllAction = menu->addAction("Clear All Data (Including Logins)");
  menuButton->setMenu(menu);
  toolbar->addWidget(menuButton);

  // Progress bar
  progressBar = new QProgressBar(this);
  progressBar->setMaximumHeight(2);
  progressBar->setTextVisible(false);
  progressBar->setStyleSheet("QProgressBar {"
                             "    background: transparent;"
                             "    border: none;"
                             "}"
                             "QProgressBar::chunk {"
                             "    background: #007ACC;"
                             "}");
  progressBar->hide();

  // Web view
  webView = new QWebEngineView(this);
  page = new CustomWebPage(profile, webView);
  webView->setPage(page);
  // Configure web settings
  webView->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
  webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled,
                                    true);
  webView->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled,
                                    true);
  webView->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  webView->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled,
                                    true);
  webView->settings()->setAttribute(
      QWebEngineSettings::FocusOnNavigationEnabled, true);

  // Set a custom user agent to handle the permissions policy issue
  QString currentAgent = profile->httpUserAgent();
  QString newAgent = currentAgent + " Chrome/120.0.0.0"; // Add Chrome version
  profile->setHttpUserAgent(newAgent);

  webView->setContextMenuPolicy(Qt::CustomContextMenu);

  // Layout
  layout->addWidget(toolbar);
  layout->addWidget(progressBar);
  layout->addWidget(webView);

  // Connect signals
  connect(addressBar, &QLineEdit::returnPressed, this,
          &BrowserView::navigateToAddress);
  connect(webView, &QWebEngineView::urlChanged, this,
          &BrowserView::handleUrlChange);
  connect(webView, &QWebEngineView::loadProgress, this,
          &BrowserView::handleLoadProgress);
  connect(webView, &QWebEngineView::loadFinished, this,
          &BrowserView::handleLoadFinished);
  connect(webView, &QWebEngineView::customContextMenuRequested, this,
          &BrowserView::showContextMenu);
  connect(backAction, &QAction::triggered, this, &BrowserView::goBack);
  connect(forwardAction, &QAction::triggered, this, &BrowserView::goForward);
  connect(refreshAction, &QAction::triggered, this, &BrowserView::refresh);
  connect(clearDataAction, &QAction::triggered, this, &BrowserView::clearData);

  // Connect title changed signal
  connect(webView, &QWebEngineView::titleChanged, this,
          &BrowserView::titleChanged);
  connect(clearAllAction, &QAction::triggered, this,
          &BrowserView::clearAllStoredData);
  connect(page, &CustomWebPage::linkClicked, this,
          &BrowserView::handleLinkClicked);

  // Initial state
  backAction->setEnabled(false);
  forwardAction->setEnabled(false);
}

void BrowserView::clearAllStoredData() {
  QMessageBox::StandardButton reply = QMessageBox::question(
      this, "Clear All Data",
      "This will clear all stored data including login information. Continue?",
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    // Clear various types of data
    profile->clearAllVisitedLinks();
    profile->clearHttpCache();

    // Clear local storage
    QString dataPath = profile->persistentStoragePath();
    QDir(dataPath).removeRecursively();

    // Recreate storage directory
    QDir().mkpath(dataPath);

    // Clear cookies
    if (profile->cookieStore())
      profile->cookieStore()->deleteAllCookies();

    QMessageBox::information(this, "Cleared",
                             "All browsing data has been cleared.");

    // Reload current page
    webView->reload();
  }
}

void BrowserView::loadUrl(const QString &url) {
  webView->setUrl(QUrl(sanitizeUrl(url)));
}

QString BrowserView::currentUrl() const { return webView->url().toString(); }

void BrowserView::handleUrlChange(const QUrl &url) {
  addressBar->setText(url.toString());
  backAction->setEnabled(webView->history()->canGoBack());
  forwardAction->setEnabled(webView->history()->canGoForward());
}

void BrowserView::handleLoadProgress(int progress) {
  if (progress < 100) {
    progressBar->show();
    progressBar->setValue(progress);
  } else {
    progressBar->hide();
  }
}

void BrowserView::handleLoadFinished(bool ok) {
  progressBar->hide();
  if (!ok) {
    // Handle error
  }
}

void BrowserView::navigateToAddress() {
  QString input = addressBar->text().trimmed();
  loadUrl(input);
}

void BrowserView::showContextMenu(const QPoint &pos) {
  QMenu *menu = new QMenu(this);
  menu->setStyleSheet("QMenu {"
                      "    background-color: #252526;"
                      "    color: #D4D4D4;"
                      "    border: 1px solid #3D3D3D;"
                      "}"
                      "QMenu::item {"
                      "    padding: 6px 32px 6px 20px;"
                      "}"
                      "QMenu::item:selected {"
                      "    background-color: #3D3D3D;"
                      "}"
                      "QMenu::separator {"
                      "    height: 1px;"
                      "    background: #3D3D3D;"
                      "    margin: 4px 0px;"
                      "}");

  menu->addAction(webView->pageAction(QWebEnginePage::Back));
  menu->addAction(webView->pageAction(QWebEnginePage::Forward));
  menu->addAction(webView->pageAction(QWebEnginePage::Reload));
  menu->addSeparator();
  menu->addAction(webView->pageAction(QWebEnginePage::Cut));
  menu->addAction(webView->pageAction(QWebEnginePage::Copy));
  menu->addAction(webView->pageAction(QWebEnginePage::Paste));
  menu->addSeparator();
  menu->addAction(webView->pageAction(QWebEnginePage::SavePage));
  menu->addAction(webView->pageAction(QWebEnginePage::ViewSource));

  menu->exec(webView->mapToGlobal(pos));
  delete menu;
}

void BrowserView::goBack() { webView->back(); }

void BrowserView::goForward() { webView->forward(); }

void BrowserView::refresh() { webView->reload(); }

void BrowserView::clearData() {
  QMessageBox::StandardButton reply = QMessageBox::question(
      this, "Clear Browsing Data",
      "Clear temporary browsing data? (This will not clear login information)",
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    // Only clear cache and visited links
    profile->clearHttpCache();
    profile->clearAllVisitedLinks();
    QMessageBox::information(this, "Cleared",
                             "Browsing data has been cleared.");
    webView->reload();
  }
}

QString BrowserView::sanitizeUrl(const QString &url) {
  if (url.isEmpty()) {
    return "about:blank";
  }

  QString sanitized = url;
  if (!url.contains("://")) {
    // Check if it's a valid domain
    if (url.contains('.')) {
      sanitized = "http://" + url;
    } else {
      // Treat as search query
      sanitized =
          "https://www.google.com/search?q=" + QUrl::toPercentEncoding(url);
    }
  }
  return sanitized;
}

void BrowserView::handleLinkClicked(const QUrl &url) { emit createTab(url); }

bool CustomWebPage::acceptNavigationRequest(const QUrl &url, NavigationType type,
                                             bool isMainFrame) {
  if (type == NavigationTypeLinkClicked) {
    QWidget* widget = qobject_cast<QWidget*>(parent());
    if (widget && QApplication::keyboardModifiers() & Qt::ControlModifier) {
      emit createTab(url);
      return false;
    }
    emit linkClicked(url);
    return false; // Don't navigate in current view
  }
  return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}
