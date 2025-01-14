#pragma once

#include <QDialog>
#include <QTreeWidget>

class KeyboardShortcutsDialog : public QDialog {
  Q_OBJECT
public:
  explicit KeyboardShortcutsDialog(QWidget *parent = nullptr);

private slots:
  void handleItemDoubleClicked(QTreeWidgetItem *item, int column);
  void applyShortcuts();

private:
  void loadShortcuts();
  QTreeWidget *m_shortcutsTree;
};
