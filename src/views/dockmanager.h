#pragma once
#include "views/terminal/terminal.h"
#include "views/terminal/terminalwidget.h"
#include <QDockWidget>
#include <QMainWindow>
#include <QMap>
#include <QString>

class DockManager : public QObject {
  Q_OBJECT

public:
  enum class DockArea { Left, Right, Top, Bottom, Center, Floating };

  enum class DockWidgetType { ProjectTree, Editor, ContentView, Terminal };

  explicit DockManager(QMainWindow *mainWindow);

  // Dock management
  QDockWidget *addDockWidget(DockWidgetType type, QWidget *widget,
                             const QString &title);
  void moveDockWidget(QDockWidget *dock, DockArea area);
  void tabifyDockWidget(QDockWidget *first, QDockWidget *second);
  void splitDockWidget(QDockWidget *first, QDockWidget *second,
                       Qt::Orientation orientation);

  // Layout management
  void saveLayout(const QString &name = "default");
  void loadLayout(const QString &name = "default");
  void resetLayout();

  // Utility functions
  QList<QDockWidget *> getDockWidgets() const;
  QDockWidget *getDockWidget(DockWidgetType type) const;
  bool isDockVisible(DockWidgetType type) const;
  void setDockVisible(DockWidgetType type, bool visible);
  void createDefaultLayout();
  void hideAllDocks();

  TerminalWidget *getTerminalWidget() {
    if (auto dock = getDockWidget(DockWidgetType::Terminal)) {
      return qobject_cast<TerminalWidget *>(dock->widget());
    }
    return nullptr;
  }

  void createNewTerminal() {
    if (auto dock = getDockWidget(DockWidgetType::Terminal)) {
      auto terminal = qobject_cast<Terminal *>(dock->widget());
      if (terminal) {
        terminal->createNewTerminalTab();
      }
    }
  }

signals:
  void layoutChanged();
  void dockVisibilityChanged(DockWidgetType type, bool visible);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void handleDockLocationChanged(Qt::DockWidgetArea area);
  void handleDockVisibilityChanged(bool visible);
  void handleTopLevelChanged(bool topLevel);

private:
  QMainWindow *mainWindow;
  QMap<DockWidgetType, QDockWidget *> dockWidgets;
  QMap<QDockWidget *, DockWidgetType> reverseLookup;

  void setupDockWidget(QDockWidget *dock, DockWidgetType type,
                       const QString &title);
  Qt::DockWidgetArea convertDockArea(DockArea area) const;
  void connectDockSignals(QDockWidget *dock);
  QString getDockTypeName(DockWidgetType type) const;
};
