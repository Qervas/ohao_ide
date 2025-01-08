#pragma once
#include <QPlainTextEdit>
#include <QProcess>
#include <QTextCharFormat>
#include <QWidget>

class TerminalWidget : public QWidget {
  Q_OBJECT

public:
  explicit TerminalWidget(QWidget *parent = nullptr);
  void setWorkingDirectory(const QString &path);

signals:
  void closeRequested();

private:
  QProcess *process;
  QPlainTextEdit *terminal;
  QString currentWorkingDirectory;
  QString username;
  QString hostname;
  QStringList commandHistory;
  int historyIndex;
  int promptPosition;
  QString previousWorkingDirectory;

  void setupUI();
  void setupProcess();
  void executeCommand(const QString &command);
  void appendOutput(const QString &text,
                    const QColor &color = QColor("#D4D4D4"));
  QString getColoredPrompt() const;
  void displayPrompt();
  void handleInput(QKeyEvent *event);
  void handleCommandExecution();
  void handleHistoryNavigation(bool up);
  QString getCurrentCommand() const;
  void setCurrentCommand(const QString &command);
  bool isInPromptArea() const;
  void handleCtrlC();
  void handleCtrlL();
  void handleCtrlD();
  void handleCdCommand(const QString &command);
  void handleTabCompletion();
  QStringList getCompletions(const QString &prefix) const;
  void showCompletions(const QStringList &completions);
  void appendFormattedOutput(const QString &text);

private slots:
  void onReadyReadStandardOutput();
  void onReadyReadStandardError();
  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
  bool eventFilter(QObject *obj, QEvent *event) override;
};
