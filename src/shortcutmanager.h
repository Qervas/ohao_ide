#pragma once

#include <QAction>
#include <QHash>
#include <QKeySequence>
#include <QObject>
#include <QPointer>

class ShortcutManager : public QObject {
  Q_OBJECT
public:
  static ShortcutManager &instance() {
    static ShortcutManager instance;
    return instance;
  }

  void registerShortcut(const QString &id, const QKeySequence &defaultSequence,
                        QAction *action, const QString &description);
  void updateShortcut(const QString &id, const QKeySequence &sequence);
  QKeySequence getShortcut(const QString &id) const;
  QHash<QString, QPair<QKeySequence, QString>> getAllShortcuts() const;
  void clearShortcut(const QString &id);

private:
  ShortcutManager() {}
  struct ShortcutData {
    QKeySequence sequence;
    QString description;
    QPointer<QAction> action;
  };
  QHash<QString, ShortcutData> m_shortcuts;
};
