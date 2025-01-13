#include "terminal.h"
#include <QAction>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>
#include <qapplication.h>

Terminal::Terminal(QWidget *parent) : DockWidgetBase(parent) { 
    setupUI(); 
}

void Terminal::setWorkingDirectory(const QString &path) {
    DockWidgetBase::setWorkingDirectory(path);
    if (TerminalWidget *terminal = getCurrentTerminal()) {
        terminal->setWorkingDirectory(path);
    }
}

void Terminal::updateTheme() {
    // Update theme-related properties
    // This will be implemented when we add theme support
}

void Terminal::focusWidget() {
    if (TerminalWidget *terminal = getCurrentTerminal()) {
        terminal->setFocus();
    }
}

void Terminal::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // Create toolbar
  createToolBar();

  // Create tab widget
  tabWidget = new QTabWidget(this);
  tabWidget->setTabsClosable(true);
  tabWidget->setMovable(true);
  tabWidget->setDocumentMode(true);
  connect(tabWidget, &QTabWidget::tabCloseRequested, this, &Terminal::closeTab);

  layout->addWidget(tabWidget);

  // Add initial tab
  addNewTab();
}

void Terminal::createToolBar() {
  QToolBar *toolbar = new QToolBar(this);
  toolbar->setStyleSheet(
      "QToolBar { border: none; background: #252526; }"
      "QToolButton { background: transparent; border: none; padding: 6px; }"
      "QToolButton:hover { background: #3D3D3D; }");

  // New Tab action
  QAction *newTabAction = toolbar->addAction(
      style()->standardIcon(QStyle::SP_FileIcon), "New Terminal");
  connect(newTabAction, &QAction::triggered, this, &Terminal::addNewTab);

  // Split actions
  QAction *splitHAction = toolbar->addAction(
      style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton),
      "Split Horizontal");
  connect(splitHAction, &QAction::triggered, this,
          &Terminal::splitHorizontally);

  QAction *splitVAction = toolbar->addAction(
      style()->standardIcon(QStyle::SP_ToolBarVerticalExtensionButton),
      "Split Vertical");
  connect(splitVAction, &QAction::triggered, this, &Terminal::splitVertically);

  // Close split action
  QAction *closeSplitAction = toolbar->addAction(
      style()->standardIcon(QStyle::SP_TitleBarCloseButton), "Close Split");
  connect(closeSplitAction, &QAction::triggered, this,
          &Terminal::closeCurrentSplit);

  layout()->addWidget(toolbar);
}

void Terminal::addNewTab() {
  QSplitter *splitter = new QSplitter(this);
  splitter->setChildrenCollapsible(false);
  splitters.append(splitter);

  TerminalWidget *terminal = createTerminal();
  splitter->addWidget(terminal);

  int index = tabWidget->addTab(splitter,
                                tr("Terminal %1").arg(tabWidget->count() + 1));
  tabWidget->setCurrentIndex(index);
}

void Terminal::splitHorizontally() {
  QSplitter *currentSplitter = getCurrentSplitter();
  if (!currentSplitter)
    return;

  currentSplitter->setOrientation(Qt::Vertical);
  TerminalWidget *newTerminal = createTerminal();
  currentSplitter->addWidget(newTerminal);
}

void Terminal::splitVertically() {
  QSplitter *currentSplitter = getCurrentSplitter();
  if (!currentSplitter)
    return;

  currentSplitter->setOrientation(Qt::Horizontal);
  TerminalWidget *newTerminal = createTerminal();
  currentSplitter->addWidget(newTerminal);
}

void Terminal::closeCurrentSplit() {
  QSplitter *splitter = getCurrentSplitter();
  if (!splitter || splitter->count() <= 1)
    return;

  QWidget *current = splitter->widget(splitter->indexOf(getCurrentTerminal()));
  if (current) {
    current->deleteLater();
  }
}

void Terminal::closeTab(int index) {
  QWidget *widget = tabWidget->widget(index);
  tabWidget->removeTab(index);
  if (splitters.contains(qobject_cast<QSplitter *>(widget))) {
    splitters.removeOne(qobject_cast<QSplitter *>(widget));
  }
  widget->deleteLater();

  if (tabWidget->count() == 0) {
    addNewTab();
  }
}

TerminalWidget *Terminal::createTerminal() {
  TerminalWidget *terminal = new TerminalWidget(this);
  connect(terminal, &TerminalWidget::closeRequested, this, [this, terminal]() {
    int index = tabWidget->indexOf(terminal->parentWidget());
    if (index >= 0) {
      closeTab(index);
    }
  });

  if (!m_workingDirectory.isEmpty()) {
    terminal->setWorkingDirectory(m_workingDirectory);
  }
  return terminal;
}

QSplitter *Terminal::getCurrentSplitter() {
  return qobject_cast<QSplitter *>(tabWidget->currentWidget());
}

TerminalWidget *Terminal::getCurrentTerminal() {
  QSplitter *splitter = getCurrentSplitter();
  if (!splitter)
    return nullptr;

  // Return the last focused terminal widget
  QWidget *focused = QApplication::focusWidget();
  while (focused && !qobject_cast<TerminalWidget *>(focused)) {
    focused = focused->parentWidget();
  }

  if (focused) {
    return qobject_cast<TerminalWidget *>(focused);
  }

  // If no terminal has focus, return the first one
  return qobject_cast<TerminalWidget *>(splitter->widget(0));
}

void Terminal::createNewTerminalTab() {
    addNewTab();
}
