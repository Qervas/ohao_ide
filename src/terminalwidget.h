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
  void setFontSize(int size);
  void zoomIn();
  void zoomOut();
  void resetZoom();
  void find();
  void findNext();
  void findPrevious();
  void setIntelligentIndent(bool enabled);
  bool intelligentIndentEnabled() const { return m_intelligentIndent; }

signals:
  void closeRequested();
  void fontSizeChanged(int size);

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

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
  QString searchString;
  int baseFontSize;
  bool ctrlPressed;
  bool m_intelligentIndent;

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
  void handleIndent(bool indent);
  QString getIndentString() const;
  int getIndentLevel(const QString &text) const;
  QStringList getCompletions(const QString &prefix) const;
  void showCompletions(const QStringList &completions);
  void appendFormattedOutput(const QString &text);
  void createContextMenu(const QPoint &pos);
  void searchHistory(const QString &searchTerm);
  void copySelectedText();
  void pasteClipboard();
  void selectAll();
  void clearScrollback();
  void handleZoom(int delta);
  void updateFont();
  void highlightSearchText();
  void setupShortcuts();
  static QColor getAnsiColor(int colorCode, bool bright = false);
  static QString getAnsiColorTable();
  
  // Constants for ANSI colors
  static const QColor DefaultForeground;
  static const QColor DefaultBackground;
  static const QVector<QColor> AnsiColors;
  static const QVector<QColor> AnsiBrightColors;

private slots:
  void onReadyReadStandardOutput();
  void onReadyReadStandardError();
  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
  bool eventFilter(QObject *obj, QEvent *event) override;
};
