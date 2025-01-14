#include "shortcutmanager.h"
#include <QSettings>

void ShortcutManager::registerShortcut(const QString &id,
                                       const QKeySequence &defaultSequence,
                                       QAction *action,
                                       const QString &description) {
  QSettings settings;
  QKeySequence sequence = QKeySequence(
      settings.value("shortcuts/" + id, defaultSequence.toString()).toString());

  ShortcutData data;
  data.sequence = sequence;
  data.description = description;
  data.action = action;

  m_shortcuts[id] = data;

  if (action) {
    action->setShortcut(sequence);
  }
}

void ShortcutManager::updateShortcut(const QString &id,
                                     const QKeySequence &sequence) {
  if (!m_shortcuts.contains(id)) {
    return;
  }

  ShortcutData &data = m_shortcuts[id];
  data.sequence = sequence;

  // Only update the action if it still exists
  if (data.action) {
    data.action->setShortcut(sequence);
  }

  QSettings settings;
  settings.setValue("shortcuts/" + id, sequence.toString());
}

void ShortcutManager::clearShortcut(const QString &id) {
  if (m_shortcuts.contains(id)) {
    ShortcutData &data = m_shortcuts[id];
    if (data.action) {
      data.action->setShortcut(QKeySequence());
    }
    data.sequence = QKeySequence();

    QSettings settings;
    settings.setValue("shortcuts/" + id, QString());
  }
}

QKeySequence ShortcutManager::getShortcut(const QString &id) const {
  return m_shortcuts.value(id).sequence;
}

QHash<QString, QPair<QKeySequence, QString>>
ShortcutManager::getAllShortcuts() const {
  QHash<QString, QPair<QKeySequence, QString>> result;
  for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
    result[it.key()] = qMakePair(it.value().sequence, it.value().description);
  }
  return result;
}
