#pragma once
#include <QWidget>
#include <QProcess>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QStringList>

class CommandLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit CommandLineEdit(QWidget *parent = nullptr);
    void addToHistory(const QString &command);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *e) override;

signals:
    void tabPressed();

public:
    QStringList history;
    int historyIndex;
};

class Terminal : public QWidget {
    Q_OBJECT

public:
    explicit Terminal(QWidget *parent = nullptr);
    void setWorkingDirectory(const QString &path);

protected:
    void focusInEvent(QFocusEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleReturnPressed();
    void handleTabCompletion();

private:
    QProcess *process;
    QPlainTextEdit *outputDisplay;
    CommandLineEdit *commandInput;
    QString currentWorkingDirectory;
    QStringList commandHistory;
    QStringList fileCompletions;
    int historyIndex;
    QString username;
    QString hostname;

    void setupUI();
    void setupProcess();
    void executeCommand(const QString &command);
    void appendOutput(const QString &text, const QColor &color = QColor("#D4D4D4"));
    QStringList getCompletions(const QString &prefix);
    QString getCurrentCommand() const;
    void showCompletions(const QStringList &completions);
    QString getShell();
    void updatePrompt();
    QString getColoredPrompt() const;
}; 