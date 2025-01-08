#include "terminalwidget.h"
#include <QDir>
#include <QFontDatabase>
#include <QHostInfo>
#include <QKeyEvent>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QScrollBar>
#include <QVBoxLayout>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent), historyIndex(-1), promptPosition(0) {

  username = qgetenv("USER");
  if (username.isEmpty()) {
    username = qgetenv("USERNAME");
  }
  hostname = QHostInfo::localHostName();

  setupUI();
  setupProcess();
  setWorkingDirectory(QDir::homePath());
}

void TerminalWidget::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  terminal = new QPlainTextEdit(this);
  terminal->setFrameStyle(QFrame::NoFrame);
  terminal->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  terminal->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  terminal->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  terminal->installEventFilter(this);

  // Set terminal font
  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setPointSize(10);
  terminal->setFont(font);

  // Set modern terminal colors
  QPalette p = terminal->palette();
  p.setColor(QPalette::Base, QColor("#282828")); // Darker background
  p.setColor(QPalette::Text, QColor("#F8F8F2")); // Brighter text
  terminal->setPalette(p);

  // Style sheet for better appearance
  terminal->setStyleSheet("QPlainTextEdit {"
                          "   background-color: #282828;"
                          "   color: #F8F8F2;"
                          "   border: none;"
                          "   padding: 4px;"
                          "}"
                          "QPlainTextEdit:focus {"
                          "   border: none;"
                          "   outline: none;"
                          "}");

  layout->addWidget(terminal);
  displayPrompt();
}

void TerminalWidget::setupProcess() {
  process = new QProcess(this);
  process->setProcessChannelMode(QProcess::MergedChannels);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  process->setProcessEnvironment(env);

  connect(process, &QProcess::readyReadStandardOutput, this,
          &TerminalWidget::onReadyReadStandardOutput);
  connect(process, &QProcess::readyReadStandardError, this,
          &TerminalWidget::onReadyReadStandardError);
  connect(process, &QProcess::finished, this,
          &TerminalWidget::onProcessFinished);
}

QString TerminalWidget::getColoredPrompt() const {
  QString dirName = QDir(currentWorkingDirectory).dirName();
  if (dirName.isEmpty()) {
    dirName = "/";
  }

  return QString("<span style='color:#87FF5F'>%1@%2</span>"
                 "<span style='color:#5F87FF'>:%3</span>"
                 "<span style='color:#FF5F5F'>$</span> ")
      .arg(username, hostname, dirName);
}

void TerminalWidget::displayPrompt() {
  QTextCursor cursor = terminal->textCursor();
  cursor.movePosition(QTextCursor::End);
  cursor.insertHtml(getColoredPrompt());
  promptPosition = cursor.position();
  terminal->setTextCursor(cursor);
  terminal->ensureCursorVisible();
}

void TerminalWidget::handleCommandExecution() {
  QString command = getCurrentCommand();
  terminal->appendPlainText(""); // New line

  if (!command.isEmpty()) {
    commandHistory.append(command);
    historyIndex = commandHistory.size();
    executeCommand(command);
  } else {
    displayPrompt();
  }
}

void TerminalWidget::handleHistoryNavigation(bool up) {
  if (commandHistory.isEmpty())
    return;

  if (up) {
    if (historyIndex > 0) {
      historyIndex--;
      setCurrentCommand(commandHistory[historyIndex]);
    }
  } else {
    if (historyIndex < commandHistory.size() - 1) {
      historyIndex++;
      setCurrentCommand(commandHistory[historyIndex]);
    } else {
      historyIndex = commandHistory.size();
      setCurrentCommand("");
    }
  }
}

QString TerminalWidget::getCurrentCommand() const {
  QTextCursor cursor = terminal->textCursor();
  cursor.movePosition(QTextCursor::End);
  cursor.setPosition(promptPosition, QTextCursor::KeepAnchor);
  return cursor.selectedText();
}

void TerminalWidget::setCurrentCommand(const QString &command) {
  QTextCursor cursor = terminal->textCursor();
  cursor.movePosition(QTextCursor::End);
  cursor.setPosition(promptPosition, QTextCursor::KeepAnchor);
  cursor.insertText(command);
}

void TerminalWidget::appendOutput(const QString &text, const QColor &color) {
  if (color.isValid()) {
    QTextCursor cursor = terminal->textCursor();
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setForeground(color);
    cursor.insertText(text, format);
    terminal->setTextCursor(cursor);
    terminal->ensureCursorVisible();
  } else {
    appendFormattedOutput(text);
  }
}

void TerminalWidget::onReadyReadStandardOutput() {
  QByteArray output = process->readAllStandardOutput();
  appendFormattedOutput(QString::fromLocal8Bit(output));
}

void TerminalWidget::onReadyReadStandardError() {
  QByteArray error = process->readAllStandardError();
  appendFormattedOutput(QString::fromLocal8Bit(error));
}

void TerminalWidget::onProcessFinished(int exitCode,
                                       QProcess::ExitStatus exitStatus) {
  Q_UNUSED(exitCode);
  Q_UNUSED(exitStatus);
  displayPrompt();
}

void TerminalWidget::executeCommand(const QString &command) {
  // Handle built-in commands first
  if (command == "clear" || command == "cls") {
    terminal->clear();
    displayPrompt();
    return;
  }

  if (command.startsWith("cd ") || command == "cd") {
    handleCdCommand(command);
    return;
  }

#ifdef Q_OS_WIN
  process->start("cmd.exe", QStringList() << "/c" << command);
#else
  process->start("/bin/bash", QStringList() << "-c" << command);
#endif
}

void TerminalWidget::handleCdCommand(const QString &command) {
  QString newPath;
  QString arg = command.mid(3).trimmed();

  if (arg.isEmpty() || arg == "~") {
    newPath = QDir::homePath();
  } else if (arg == "-") {
    // cd - to go to previous directory
    newPath = previousWorkingDirectory;
  } else {
    QDir dir(currentWorkingDirectory);
    if (dir.cd(arg)) {
      newPath = dir.absolutePath();
    } else {
      appendOutput("cd: " + arg + ": No such directory\n", QColor("#FF5F5F"));
      displayPrompt();
      return;
    }
  }

  if (!newPath.isEmpty() && QDir(newPath).exists()) {
    previousWorkingDirectory = currentWorkingDirectory;
    setWorkingDirectory(newPath);
    displayPrompt();
  }
}

bool TerminalWidget::eventFilter(QObject *obj, QEvent *event) {
  if (obj == terminal && event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    // Handle control key combinations
    if (keyEvent->modifiers() & Qt::ControlModifier) {
      switch (keyEvent->key()) {
      case Qt::Key_C:
        handleCtrlC();
        return true;

      case Qt::Key_L:
        handleCtrlL();
        return true;

      case Qt::Key_D:
        handleCtrlD();
        return true;
      }
    }

    switch (keyEvent->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
      handleCommandExecution();
      return true;

    case Qt::Key_Up:
    case Qt::Key_Down:
      handleHistoryNavigation(keyEvent->key() == Qt::Key_Up);
      return true;

    case Qt::Key_Tab:
      handleTabCompletion();
      return true;

    case Qt::Key_Backspace:
      if (terminal->textCursor().position() <= promptPosition) {
        return true;
      }
      break;
    }

    if (terminal->textCursor().position() < promptPosition) {
      terminal->moveCursor(QTextCursor::End);
    }
  }
  return QWidget::eventFilter(obj, event);
}

void TerminalWidget::handleTabCompletion() {
  QString currentText = getCurrentCommand();
  QString wordUnderCursor = currentText.section(' ', -1);

  if (wordUnderCursor.isEmpty()) {
    return;
  }

  QStringList completions = getCompletions(wordUnderCursor);

  if (completions.isEmpty()) {
    return;
  } else if (completions.size() == 1) {
    // Single completion - replace the word
    QString newText =
        currentText.left(currentText.length() - wordUnderCursor.length()) +
        completions.first();
    setCurrentCommand(newText);
  } else {
    // Multiple completions - show them
    showCompletions(completions);
  }
}
void TerminalWidget::showCompletions(const QStringList &completions) {
  if (completions.isEmpty())
    return;

  // Store the current command before showing completions
  QString currentCommand = getCurrentCommand();

  // Calculate column width based on longest item
  int maxWidth = 0;
  for (const QString &item : completions) {
    maxWidth = qMax(maxWidth, item.length());
  }
  maxWidth += 2; // Add minimum spacing between columns

  // Get terminal width in characters
  QFontMetrics fm(terminal->font());
  int terminalWidth = terminal->viewport()->width() / fm.horizontalAdvance(' ');
  int numColumns = qMax(1, terminalWidth / maxWidth);

  // Sort completions
  QStringList sortedCompletions = completions;
  std::sort(sortedCompletions.begin(), sortedCompletions.end());

  // Calculate rows needed
  int numItems = sortedCompletions.size();
  int numRows = (numItems + numColumns - 1) / numColumns;

  // Display completions
  appendOutput("\n");

  for (int row = 0; row < numRows; ++row) {
    for (int col = 0; col < numColumns; ++col) {
      int index = col * numRows + row;
      if (index < sortedCompletions.size()) {
        QString item = sortedCompletions[index];
        QString paddedItem = item.leftJustified(maxWidth, ' ');

        // Determine color based on type
        QColor color;
        if (item.endsWith('/')) {
          color = QColor("#5F87FF"); // Blue for directories
        } else if (item.endsWith('*')) {
          color = QColor("#87FF5F"); // Green for executables
        } else {
          color = QColor("#F8F8F2"); // Default color for files
        }

        appendOutput(paddedItem, color);
      }
    }
    appendOutput("\n");
  }

  // Restore prompt with the original command
  appendOutput("\n");
  displayPrompt();
  setCurrentCommand(currentCommand); // Restore the original command
}

QStringList TerminalWidget::getCompletions(const QString &prefix) const {
  QStringList completions;
  QString command = getCurrentCommand();
  QStringList parts = command.split(' ', Qt::SkipEmptyParts);

  if (parts.isEmpty()) {
    return completions;
  }

  // Command-specific completions
  if (parts[0] == "ls" && prefix == "-") {
    // Common ls options
    completions << "-l" << "-a" << "-h" << "-t" << "-r" << "-R" << "--help";
  } else if (parts[0] == "cd") {
    // Only show directories for cd
    QDir dir(currentWorkingDirectory);
    for (const QFileInfo &info :
         dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
      QString name = info.fileName();
      if (name.startsWith(prefix, Qt::CaseInsensitive)) {
        completions << name + "/";
      }
    }
  } else if (parts.size() == 1) {
    // Completing command name
    QDir dir(currentWorkingDirectory);

    // Add executables from current directory
    for (const QFileInfo &info :
         dir.entryInfoList(QDir::Files | QDir::Executable)) {
      QString name = info.fileName();
      if (name.startsWith(prefix, Qt::CaseInsensitive)) {
        completions << name + "*";
      }
    }

    // Add commands from PATH
    QString pathEnv = qgetenv("PATH");
    QStringList paths = pathEnv.split(QDir::listSeparator());
    for (const QString &path : paths) {
      QDir pathDir(path);
      for (const QFileInfo &info :
           pathDir.entryInfoList(QDir::Files | QDir::Executable)) {
        QString name = info.fileName();
        if (name.startsWith(prefix, Qt::CaseInsensitive)) {
          completions << name;
        }
      }
    }
  } else {
    // File completion for arguments
    QDir dir(currentWorkingDirectory);
    for (const QFileInfo &info :
         dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
      QString name = info.fileName();
      if (name.startsWith(prefix, Qt::CaseInsensitive)) {
        if (info.isDir()) {
          completions << name + "/";
        } else if (info.isExecutable()) {
          completions << name + "*";
        } else {
          completions << name;
        }
      }
    }
  }

  completions.removeDuplicates();
  completions.sort(Qt::CaseInsensitive);
  return completions;
}

void TerminalWidget::handleCtrlC() {
  if (process->state() == QProcess::Running) {
    process->kill();
    appendOutput("^C\n");
    displayPrompt();
  } else {
    appendOutput("^C\n");
    displayPrompt();
  }
}

void TerminalWidget::handleCtrlL() {
  terminal->clear();
  displayPrompt();
}

void TerminalWidget::handleCtrlD() {
  // Only exit if no process is running and the current line is empty
  if (process->state() != QProcess::Running && getCurrentCommand().isEmpty()) {
    // Emit a signal to close this terminal instance
    emit closeRequested();
  }
}

void TerminalWidget::setWorkingDirectory(const QString &path) {
  if (path.isEmpty() || !QDir(path).exists()) {
    return;
  }

  QDir dir(path);
  currentWorkingDirectory = dir.absolutePath();
  process->setWorkingDirectory(currentWorkingDirectory);

  // Set the PWD environment variable
  QProcessEnvironment env = process->processEnvironment();
  env.insert("PWD", currentWorkingDirectory);
  process->setProcessEnvironment(env);
}

void TerminalWidget::appendFormattedOutput(const QString &text) {
  QTextCursor cursor = terminal->textCursor();
  cursor.movePosition(QTextCursor::End);

  QRegularExpression regex("\x1B\\[([0-9;]*)([A-Za-z])");
  QRegularExpressionMatchIterator it = regex.globalMatch(text);

  int lastPos = 0;
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    QString before = text.mid(lastPos, match.capturedStart() - lastPos);
    cursor.insertText(before);

    QString codes = match.captured(1);
    QString command = match.captured(2);

    if (command == "m") {
      QStringList codeList = codes.split(';');
      QTextCharFormat format = cursor.charFormat();
      for (const QString &code : codeList) {
        int codeInt = code.toInt();
        switch (codeInt) {
        case 0: // Reset
          format = QTextCharFormat();
          break;
        case 1: // Bold
          format.setFontWeight(QFont::Bold);
          break;
        case 30:
          format.setForeground(QColor("#000000"));
          break; // Black
        case 31:
          format.setForeground(QColor("#FF0000"));
          break; // Red
        case 32:
          format.setForeground(QColor("#00FF00"));
          break; // Green
        case 33:
          format.setForeground(QColor("#FFFF00"));
          break; // Yellow
        case 34:
          format.setForeground(QColor("#0000FF"));
          break; // Blue
        case 35:
          format.setForeground(QColor("#FF00FF"));
          break; // Magenta
        case 36:
          format.setForeground(QColor("#00FFFF"));
          break; // Cyan
        case 37:
          format.setForeground(QColor("#FFFFFF"));
          break; // White
        case 40:
          format.setBackground(QColor("#000000"));
          break; // Black background
        case 41:
          format.setBackground(QColor("#FF0000"));
          break; // Red background
        case 42:
          format.setBackground(QColor("#00FF00"));
          break; // Green background
        case 43:
          format.setBackground(QColor("#FFFF00"));
          break; // Yellow background
        case 44:
          format.setBackground(QColor("#0000FF"));
          break; // Blue background
        case 45:
          format.setBackground(QColor("#FF00FF"));
          break; // Magenta background
        case 46:
          format.setBackground(QColor("#00FFFF"));
          break; // Cyan background
        case 47:
          format.setBackground(QColor("#FFFFFF"));
          break; // White background
        // Add more cases as needed for other ANSI codes
        default:
          break;
        }
      }
      cursor.setCharFormat(format);
    }

    lastPos = match.capturedEnd();
  }

  QString remaining = text.mid(lastPos);
  cursor.insertText(remaining);

  terminal->setTextCursor(cursor);
  terminal->ensureCursorVisible();
}
