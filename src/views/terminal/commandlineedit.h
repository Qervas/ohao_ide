#pragma once
#include <QLineEdit>
#include <QKeyEvent>
#include <QStringList>

class CommandLineEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit CommandLineEdit(QWidget *parent = nullptr) : QLineEdit(parent), historyIndex(-1) {
        setFocusPolicy(Qt::StrongFocus);
    }

    void addToHistory(const QString &command) {
        history.append(command);
        historyIndex = history.size();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        switch (event->key()) {
            case Qt::Key_Up:
                // Navigate history backwards
                if (!history.isEmpty() && historyIndex > 0) {
                    historyIndex--;
                    setText(history.at(historyIndex));
                }
                break;

            case Qt::Key_Down:
                // Navigate history forwards
                if (!history.isEmpty() && historyIndex < history.size() - 1) {
                    historyIndex++;
                    setText(history.at(historyIndex));
                } else if (historyIndex == history.size() - 1) {
                    // Clear text when reaching the end of history
                    historyIndex = history.size();
                    clear();
                }
                break;

            default:
                QLineEdit::keyPressEvent(event);
                break;
        }
    }

    bool event(QEvent *e) override {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent *>(e);
            if (ke->key() == Qt::Key_Tab) {
                emit tabPressed();
                return true;  // Prevent focus change
            }
        }
        return QLineEdit::event(e);
    }

signals:
    void tabPressed();

public:
    QStringList history;
    int historyIndex;
};
