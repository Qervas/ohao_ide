#include "dockmanager.h"
#include <QSettings>
#include <QByteArray>
#include <QApplication>
#include <QStyle>

DockManager::DockManager(QMainWindow *mainWindow) : QObject(mainWindow), mainWindow(mainWindow)
{
}

QDockWidget* DockManager::addDockWidget(DockWidgetType type, QWidget *widget, const QString &title)
{
    // Create new dock widget
    QDockWidget *dock = new QDockWidget(title, mainWindow);
    dock->setObjectName(getDockTypeName(type));
    dock->setWidget(widget);

    // Set features based on type
    QDockWidget::DockWidgetFeatures features;
    features = QDockWidget::DockWidgetClosable |
                QDockWidget::DockWidgetMovable |
                QDockWidget::DockWidgetFloatable;
    dock->setFeatures(features);

    // Set style
    dock->setStyleSheet(
        "QDockWidget {"
        "    border: 1px solid #3D3D3D;"
        "}"
        "QDockWidget::title {"
        "    background: #252526;"
        "    padding: 6px;"
        "    color: #D4D4D4;"
        "}"
        "QDockWidget::close-button, QDockWidget::float-button {"
        "    border: none;"
        "    background: #252526;"
        "    padding: 0px;"
        "}"
        "QDockWidget::close-button:hover, QDockWidget::float-button:hover {"
        "    background: #3D3D3D;"
        "}"
    );

    // Store in maps
    dockWidgets[type] = dock;
    reverseLookup[dock] = type;

    // Connect signals
    connectDockSignals(dock);
    mainWindow->addDockWidget(convertDockArea(DockArea::Left), dock);

    return dock;
}

void DockManager::moveDockWidget(QDockWidget *dock, DockArea area)
{
    if (!dock) return;

    Qt::DockWidgetArea qtArea = convertDockArea(area);
    if (area == DockArea::Floating) {
        dock->setFloating(true);
    } else {
        mainWindow->addDockWidget(qtArea, dock);
    }
}

void DockManager::tabifyDockWidget(QDockWidget *first, QDockWidget *second)
{
    if (!first || !second) return;
    mainWindow->tabifyDockWidget(first, second);
}

void DockManager::splitDockWidget(QDockWidget *first, QDockWidget *second, Qt::Orientation orientation)
{
    if (!first || !second) return;
    mainWindow->splitDockWidget(first, second, orientation);
}

void DockManager::saveLayout(const QString &name)
{
    QSettings settings;
    settings.setValue("layout/" + name, mainWindow->saveState());
}

void DockManager::loadLayout(const QString &name)
{
    QSettings settings;
    QByteArray state = settings.value("layout/" + name).toByteArray();
    if (!state.isEmpty()) {
        mainWindow->restoreState(state);
    }
}

void DockManager::resetLayout()
{

   // Set up default dock areas
    if (QDockWidget *projectDock = getDockWidget(DockWidgetType::ProjectTree)) {
        mainWindow->addDockWidget(Qt::LeftDockWidgetArea, projectDock);
    }

    if (QDockWidget *editorDock = getDockWidget(DockWidgetType::Editor)) {
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, editorDock);
    }

    if (QDockWidget *contentDock = getDockWidget(DockWidgetType::ContentView)) {
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, contentDock);
    }

    if (QDockWidget *terminalDock = getDockWidget(DockWidgetType::Terminal)) {
        mainWindow->addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
    }
}

QList<QDockWidget*> DockManager::getDockWidgets() const
{
    return dockWidgets.values();
}

QDockWidget* DockManager::getDockWidget(DockWidgetType type) const
{
    return dockWidgets.value(type, nullptr);
}

bool DockManager::isDockVisible(DockWidgetType type) const
{
    QDockWidget *dock = getDockWidget(type);
    return dock ? dock->isVisible() : false;
}

void DockManager::setDockVisible(DockWidgetType type, bool visible)
{
    if (QDockWidget *dock = getDockWidget(type)) {
        dock->setVisible(visible);
    }
}

void DockManager::handleDockLocationChanged(Qt::DockWidgetArea area)
{
    Q_UNUSED(area);
    emit layoutChanged();
}

void DockManager::handleDockVisibilityChanged(bool visible)
{
    if (QDockWidget *dock = qobject_cast<QDockWidget*>(sender())) {
        DockWidgetType type = reverseLookup.value(dock);
        emit dockVisibilityChanged(type, visible);
    }
}

void DockManager::handleTopLevelChanged(bool topLevel)
{
    Q_UNUSED(topLevel);
    emit layoutChanged();
}

void DockManager::setupDockWidget(QDockWidget *dock, DockWidgetType type, const QString &title)
{
    dock->setObjectName(getDockTypeName(type));
    dock->setWindowTitle(title);
    connectDockSignals(dock);
}

Qt::DockWidgetArea DockManager::convertDockArea(DockArea area) const
{
    switch (area) {
        case DockArea::Left:   return Qt::LeftDockWidgetArea;
        case DockArea::Right:  return Qt::RightDockWidgetArea;
        case DockArea::Top:    return Qt::TopDockWidgetArea;
        case DockArea::Bottom: return Qt::BottomDockWidgetArea;
        case DockArea::Center: return Qt::RightDockWidgetArea; // Center is handled specially
        default:               return Qt::LeftDockWidgetArea;
    }
}

void DockManager::connectDockSignals(QDockWidget *dock)
{
    connect(dock, &QDockWidget::dockLocationChanged,
            this, &DockManager::handleDockLocationChanged);
    connect(dock, &QDockWidget::visibilityChanged,
            this, &DockManager::handleDockVisibilityChanged);
    connect(dock, &QDockWidget::topLevelChanged,
            this, &DockManager::handleTopLevelChanged);

    dock->widget()->installEventFilter(this);
}

bool DockManager::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Resize) {
        QWidget *widget = qobject_cast<QWidget*>(watched);
        if (widget) {
            // Find the corresponding dock widget
            QDockWidget *dock = qobject_cast<QDockWidget*>(widget->parent());
            if (dock) {
                const int minSize = 5;  // minimum size in pixels
                if (dock->widget()->height() < minSize || dock->widget()->width() < minSize) {
                    dock->hide();
                }
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

QString DockManager::getDockTypeName(DockWidgetType type) const
{
    switch (type) {
        case DockWidgetType::ProjectTree:  return "ProjectTree";
        case DockWidgetType::Editor:       return "Editor";
        case DockWidgetType::ContentView:  return "ContentView";
        case DockWidgetType::Terminal:     return "Terminal";
        default:                           return "Unknown";
    }
}

void DockManager::createDefaultLayout()
{
    // Get dock widgets
    QDockWidget *projectDock = getDockWidget(DockWidgetType::ProjectTree);
    QDockWidget *editorDock = getDockWidget(DockWidgetType::Editor);
    QDockWidget *contentDock = getDockWidget(DockWidgetType::ContentView);
    QDockWidget *terminalDock = getDockWidget(DockWidgetType::Terminal);

    if (!projectDock || !editorDock || !contentDock || !terminalDock)
        return;

    // Show all docks
    projectDock->show();
    editorDock->show();
    contentDock->show();
    terminalDock->show();

    // Create default layout
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, projectDock);
    mainWindow->addDockWidget(Qt::RightDockWidgetArea, editorDock);
    mainWindow->addDockWidget(Qt::RightDockWidgetArea, contentDock);
    mainWindow->addDockWidget(Qt::BottomDockWidgetArea, terminalDock);

    // Set sizes
    projectDock->setMinimumWidth(200);
    projectDock->setMaximumWidth(400);
    terminalDock->setMinimumHeight(100);

    // Split editor and content view
    mainWindow->splitDockWidget(editorDock, contentDock, Qt::Horizontal);

    // Ensure terminal dock is properly set up
    terminalDock->setFeatures(QDockWidget::DockWidgetClosable |
                             QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable |
                             QDockWidget::DockWidgetVerticalTitleBar);
}
