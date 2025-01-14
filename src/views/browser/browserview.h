#pragma once
#include <QLineEdit>
#include <QMenu>
#include <QProgressBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWebEngineCookieStore>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QWidget>

class CustomWebPage : public QWebEnginePage {
  Q_OBJECT
public:
  CustomWebPage(QWebEngineProfile *profile, QObject *parent = nullptr)
      : QWebEnginePage(profile, parent) {}

protected:
  bool acceptNavigationRequest(const QUrl &url, NavigationType type,
                               bool isMainFrame) override;

signals:
  void linkClicked(const QUrl &url);
  void createTab(const QUrl &url);
};

class BrowserView : public QWidget {
  Q_OBJECT

public:
  explicit BrowserView(QWidget *parent = nullptr);
  void loadUrl(const QString &url);
  QString currentUrl() const;
  QWebEngineView *getWebView() { return webView; }

signals:
  void titleChanged(const QString &title);
  void createTab(const QUrl &url);

private slots:
  void handleUrlChange(const QUrl &url);
  void handleLoadProgress(int progress);
  void handleLoadFinished(bool ok);
  void navigateToAddress();
  void showContextMenu(const QPoint &pos);
  void goBack();
  void goForward();
  void refresh();
  void clearData();
  void clearAllStoredData();

private:
  void setupUI();
  QString sanitizeUrl(const QString &url);
  void handleLinkClicked(const QUrl &url);

  QWebEngineView *webView;
  QWebEngineProfile *profile;
  QLineEdit *addressBar;
  QProgressBar *progressBar;
  QToolBar *toolbar;
  QVBoxLayout *layout;
  CustomWebPage *page;

  QAction *backAction;
  QAction *forwardAction;
  QAction *refreshAction;
  QAction *clearDataAction;
  QAction *clearAllAction;
};
