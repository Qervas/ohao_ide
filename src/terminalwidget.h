#pragma once
#include <QWidget>
#include <QProcess>
#include <QPlainTextEdit>
#include "commandlineedit.h"

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    void setWorkingDirectory(const QString &path);

protected:
    void focusInEvent(QFocusEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QProcess *process;
    QPlainTextEdit *outputDisplay;
    CommandLineEdit *commandInput;
    QString currentWorkingDirectory;
    QString username;
    QString hostname;

    void setupUI();
    void setupProcess();
    void executeCommand(const QString &command);
    void appendOutput(const QString &text, const QColor &color = QColor("#D4D4D4"));
    QString getShell();
    void updatePrompt();
    QString getColoredPrompt() const;

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleReturnPressed();
    void handleTabCompletion();
    QStringList getCompletions(const QString &prefix);
    void showCompletions(const QStringList &completions);
};
