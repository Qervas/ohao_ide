#pragma once

#include <QPlainTextEdit>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>

class CodeEditor;

class CustomPlainTextEdit : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CustomPlainTextEdit(QWidget *parent = nullptr);
    ~CustomPlainTextEdit();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    QTimer *m_hoverTimer;
    friend class CodeEditor;
}; 