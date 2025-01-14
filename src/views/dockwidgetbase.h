#pragma once
#include <QWidget>
#include <QString>

class DockWidgetBase : public QWidget {
    Q_OBJECT

public:
    explicit DockWidgetBase(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~DockWidgetBase() = default;

    // Common interface for all dock widgets
    virtual void setWorkingDirectory(const QString &path) { m_workingDirectory = path; }
    virtual QString getWorkingDirectory() const { return m_workingDirectory; }
    
    // Focus management
    virtual void focusWidget() { setFocus(); }
    virtual bool canClose() { return true; }
    virtual bool hasUnsavedChanges() { return false; }
    
    // Common operations
    virtual void saveState() {}
    virtual void restoreState() {}
    virtual void updateTheme() {}

signals:
    void focusChanged(bool focused);
    void contentChanged();
    void titleChanged(const QString &title);
    void closeRequested();

protected:
    // Common state
    QString m_workingDirectory;
    QString m_title;
    bool m_isDirty = false;

    // Focus events
    void focusInEvent(QFocusEvent *event) override {
        QWidget::focusInEvent(event);
        emit focusChanged(true);
    }
    
    void focusOutEvent(QFocusEvent *event) override {
        QWidget::focusOutEvent(event);
        emit focusChanged(false);
    }
}; 