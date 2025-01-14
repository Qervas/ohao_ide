#pragma once
#include <QSyntaxHighlighter>
#include <QString>

class BaseHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit BaseHighlighter(QTextDocument *parent = nullptr) 
        : QSyntaxHighlighter(parent), m_enabled(true) {}
    virtual ~BaseHighlighter() = default;

    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QString filePattern() const = 0;

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { 
        m_enabled = enabled;
        rehighlight();
    }

protected:
    void highlightBlock(const QString &text) override {
        if (!m_enabled) return;
        doHighlightBlock(text);
    }

    virtual void doHighlightBlock(const QString &text) = 0;

private:
    bool m_enabled;
}; 