#include "keyboardshortcutsdialog.h"
#include "shortcutmanager.h"
#include <QKeySequenceEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

KeyboardShortcutsDialog::KeyboardShortcutsDialog(QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(tr("Configure Keyboard Shortcuts"));
  setMinimumSize(400, 300);

  QVBoxLayout *layout = new QVBoxLayout(this);

  m_shortcutsTree = new QTreeWidget(this);
  m_shortcutsTree->setHeaderLabels(
      {tr("Command"), tr("Shortcut"), tr("Description")});
  m_shortcutsTree->setColumnWidth(0, 150);
  m_shortcutsTree->setColumnWidth(1, 100);
  layout->addWidget(m_shortcutsTree);

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  QPushButton *resetButton = new QPushButton(tr("Reset All"), this);
  QPushButton *okButton = new QPushButton(tr("OK"), this);
  QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);

  buttonLayout->addWidget(resetButton);
  buttonLayout->addStretch();
  buttonLayout->addWidget(okButton);
  buttonLayout->addWidget(cancelButton);
  layout->addLayout(buttonLayout);

  connect(m_shortcutsTree, &QTreeWidget::itemDoubleClicked, this,
          &KeyboardShortcutsDialog::handleItemDoubleClicked);
  connect(okButton, &QPushButton::clicked, this,
          &KeyboardShortcutsDialog::accept);
  connect(cancelButton, &QPushButton::clicked, this,
          &KeyboardShortcutsDialog::reject);
  connect(this, &QDialog::accepted, this,
          &KeyboardShortcutsDialog::applyShortcuts);

  loadShortcuts();
}

void KeyboardShortcutsDialog::loadShortcuts() {
  m_shortcutsTree->clear();
  auto shortcuts = ShortcutManager::instance().getAllShortcuts();

  for (auto it = shortcuts.begin(); it != shortcuts.end(); ++it) {
    QTreeWidgetItem *item = new QTreeWidgetItem(m_shortcutsTree);
    item->setText(0, it.key());
    item->setText(1, it.value().first.toString());
    item->setText(2, it.value().second);
    item->setData(0, Qt::UserRole, it.key());
  }
}

void KeyboardShortcutsDialog::handleItemDoubleClicked(QTreeWidgetItem *item,
                                                      int column) {
  if (column == 1) {
    // Create a persistent container widget
    QWidget *container = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    // Create key sequence editor
    QKeySequenceEdit *edit =
        new QKeySequenceEdit(QKeySequence(item->text(1)), container);
    layout->addWidget(edit);

    // Create clear button
    QPushButton *clearButton = new QPushButton(tr("Clear"), container);
    clearButton->setFixedWidth(60);
    layout->addWidget(clearButton);

    // Set up connections
    connect(clearButton, &QPushButton::clicked, this, [=]() {
      item->setText(column, QString());
      m_shortcutsTree->removeItemWidget(item, column);
      container->deleteLater();
    });

    connect(edit, &QKeySequenceEdit::editingFinished, this, [=]() {
      QString newSequence = edit->keySequence().toString();
      item->setText(column, newSequence);
      m_shortcutsTree->removeItemWidget(item, column);
      container->deleteLater();
    });

    // Set the container as the item widget
    m_shortcutsTree->setItemWidget(item, column, container);
    edit->setFocus();
  }
}

void KeyboardShortcutsDialog::applyShortcuts() {
  QHash<QString, QKeySequence> newShortcuts;

  // First collect all new shortcuts
  for (int i = 0; i < m_shortcutsTree->topLevelItemCount(); ++i) {
    QTreeWidgetItem *item = m_shortcutsTree->topLevelItem(i);
    QString id = item->data(0, Qt::UserRole).toString();
    QString shortcutText = item->text(1);

    if (shortcutText.isEmpty()) {
      ShortcutManager::instance().clearShortcut(id);
    } else {
      QKeySequence sequence(shortcutText);
      if (!sequence.isEmpty()) {
        ShortcutManager::instance().updateShortcut(id, sequence);
      }
    }
  }
}
