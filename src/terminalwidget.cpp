#include "terminalwidget.h"
#include <QDir>
#include <QFontDatabase>
#include <QHostInfo>
#include <QKeyEvent>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QClipboard>
#include <QApplication>
#include <QMenu>
#include <QInputDialog>
#include <QShortcut>
#include <QWheelEvent>
#include <QContextMenuEvent>

// Define static color constants
const QColor TerminalWidget::DefaultForeground = QColor("#F8F8F2");
const QColor TerminalWidget::DefaultBackground = QColor("#282828");

const QVector<QColor> TerminalWidget::AnsiColors = {
    QColor("#000000"),  // Black
    QColor("#CC0000"),  // Red
    QColor("#4E9A06"),  // Green
    QColor("#C4A000"),  // Yellow
    QColor("#3465A4"),  // Blue
    QColor("#75507B"),  // Magenta
    QColor("#06989A"),  // Cyan
    QColor("#D3D7CF")   // White
};

const QVector<QColor> TerminalWidget::AnsiBrightColors = {
    QColor("#555753"),  // Bright Black
    QColor("#EF2929"),  // Bright Red
    QColor("#8AE234"),  // Bright Green
    QColor("#FCE94F"),  // Bright Yellow
    QColor("#729FCF"),  // Bright Blue
    QColor("#AD7FA8"),  // Bright Magenta
    QColor("#34E2E2"),  // Bright Cyan
    QColor("#EEEEEC")   // Bright White
};

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent), historyIndex(-1), promptPosition(0), baseFontSize(10), ctrlPressed(false) {

  username = qgetenv("USER");
  if (username.isEmpty()) {
    username = qgetenv("USERNAME");
  }
  hostname = QHostInfo::localHostName();

  setupUI();
  setupProcess();
  setupShortcuts();
  setWorkingDirectory(QDir::homePath());
}

void TerminalWidget::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  terminal = new QPlainTextEdit(this);
  terminal->setFrameStyle(QFrame::NoFrame);
  terminal->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  terminal->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  terminal->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  terminal->installEventFilter(this);
  terminal->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(terminal, &QWidget::customContextMenuRequested,
          this, [this](const QPoint &pos) { createContextMenu(pos); });

  // Set terminal font
  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setPointSize(baseFontSize);
  terminal->setFont(font);

  // Set modern terminal colors
  QPalette p = terminal->palette();
  p.setColor(QPalette::Base, DefaultBackground);
  p.setColor(QPalette::Text, DefaultForeground);
  terminal->setPalette(p);

  // Style sheet for better appearance
  terminal->setStyleSheet(QString(
      "QPlainTextEdit {"
      "   background-color: %1;"
      "   color: %2;"
      "   border: none;"
      "   padding: 4px;"
      "}"
      "QPlainTextEdit:focus {"
      "   border: none;"
      "   outline: none;"
      "}"
      "QScrollBar:vertical {"
      "   background-color: #2A2A2A;"
      "   width: 14px;"
      "   margin: 0px;"
      "}"
      "QScrollBar::handle:vertical {"
      "   background-color: #424242;"
      "   min-height: 20px;"
      "   border-radius: 7px;"
      "   margin: 2px;"
      "}"
      "QScrollBar::handle:vertical:hover {"
      "   background-color: #686868;"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "   height: 0px;"
      "}"
      "QScrollBar:horizontal {"
      "   background-color: #2A2A2A;"
      "   height: 14px;"
      "   margin: 0px;"
      "}"
      "QScrollBar::handle:horizontal {"
      "   background-color: #424242;"
      "   min-width: 20px;"
      "   border-radius: 7px;"
      "   margin: 2px;"
      "}"
      "QScrollBar::handle:horizontal:hover {"
      "   background-color: #686868;"
      "}"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "   width: 0px;"
      "}")
      .arg(DefaultBackground.name(), DefaultForeground.name()));

  layout->addWidget(terminal);
  displayPrompt();
}

void TerminalWidget::setupShortcuts() {
    // Zoom shortcuts
    new QShortcut(QKeySequence::ZoomIn, this, this, &TerminalWidget::zoomIn);
    new QShortcut(QKeySequence::ZoomOut, this, this, &TerminalWidget::zoomOut);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this, this, &TerminalWidget::resetZoom);
    
    // Find shortcuts
    new QShortcut(QKeySequence::Find, this, this, &TerminalWidget::find);
    new QShortcut(QKeySequence::FindNext, this, this, &TerminalWidget::findNext);
    new QShortcut(QKeySequence::FindPrevious, this, this, &TerminalWidget::findPrevious);
    
    // Copy/Paste shortcuts
    new QShortcut(QKeySequence::Copy, this, this, &TerminalWidget::copySelectedText);
    new QShortcut(QKeySequence::Paste, this, this, &TerminalWidget::pasteClipboard);
}

void TerminalWidget::contextMenuEvent(QContextMenuEvent *event) {
    createContextMenu(event->globalPos());
}

void TerminalWidget::createContextMenu(const QPoint &pos) {
    QMenu *menu = new QMenu(this);
    
    menu->addAction(tr("Copy"), this, &TerminalWidget::copySelectedText);
    menu->addAction(tr("Paste"), this, &TerminalWidget::pasteClipboard);
    menu->addSeparator();
    menu->addAction(tr("Select All"), this, &TerminalWidget::selectAll);
    menu->addSeparator();
    menu->addAction(tr("Find..."), this, &TerminalWidget::find);
    menu->addSeparator();
    menu->addAction(tr("Clear Scrollback"), this, &TerminalWidget::clearScrollback);
    
    menu->exec(terminal->mapToGlobal(pos));
    delete menu;
}

void TerminalWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        handleZoom(event->angleDelta().y());
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void TerminalWidget::handleZoom(int delta) {
    if (delta > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
}

void TerminalWidget::zoomIn() {
    setFontSize(terminal->font().pointSize() + 1);
}

void TerminalWidget::zoomOut() {
    setFontSize(terminal->font().pointSize() - 1);
}

void TerminalWidget::resetZoom() {
    setFontSize(baseFontSize);
}

void TerminalWidget::setFontSize(int size) {
    if (size < 6 || size > 72) return;
    
    QFont font = terminal->font();
    font.setPointSize(size);
    terminal->setFont(font);
    emit fontSizeChanged(size);
}

void TerminalWidget::find() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("Find"),
                                       tr("Search text:"), QLineEdit::Normal,
                                       searchString, &ok);
    if (ok && !text.isEmpty()) {
        searchString = text;
        findNext();
    }
}

void TerminalWidget::findNext() {
    if (searchString.isEmpty()) {
        find();
        return;
    }
    
    QTextDocument *doc = terminal->document();
    QTextCursor cursor = terminal->textCursor();
    
    QTextDocument::FindFlags flags;
    QTextCursor found = doc->find(searchString, cursor, flags);
    
    if (found.isNull()) {
        // Wrap around
        cursor.movePosition(QTextCursor::Start);
        found = doc->find(searchString, cursor, flags);
    }
    
    if (!found.isNull()) {
        terminal->setTextCursor(found);
    }
}

void TerminalWidget::findPrevious() {
    if (searchString.isEmpty()) {
        find();
        return;
    }
    
    QTextDocument *doc = terminal->document();
    QTextCursor cursor = terminal->textCursor();
    
    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    QTextCursor found = doc->find(searchString, cursor, flags);
    
    if (found.isNull()) {
        // Wrap around
        cursor.movePosition(QTextCursor::End);
        found = doc->find(searchString, cursor, flags);
    }
    
    if (!found.isNull()) {
        terminal->setTextCursor(found);
    }
}

void TerminalWidget::copySelectedText() {
    terminal->copy();
}

void TerminalWidget::pasteClipboard() {
    const QClipboard *clipboard = QApplication::clipboard();
    QString text = clipboard->text();
    if (!text.isEmpty()) {
        QStringList lines = text.split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            setCurrentCommand(lines[i]);
            if (i < lines.size() - 1) {
                handleCommandExecution();
            }
        }
    }
}

void TerminalWidget::selectAll() {
    terminal->selectAll();
}

void TerminalWidget::clearScrollback() {
    terminal->clear();
    displayPrompt();
}

QColor TerminalWidget::getAnsiColor(int colorCode, bool bright) {
    if (colorCode < 0 || colorCode > 7) {
        return bright ? DefaultForeground : DefaultBackground;
    }
    return bright ? AnsiBrightColors[colorCode] : AnsiColors[colorCode];
}

QString TerminalWidget::getAnsiColorTable() {
    return QString(
        "\033[0m\033[K  Default                     "
        "\033[1m  Bold\n\033[0m"
        "\033[30m  Black                       "
        "\033[1;30m  Bright Black\n\033[0m"
        "\033[31m  Red                         "
        "\033[1;31m  Bright Red\n\033[0m"
        "\033[32m  Green                       "
        "\033[1;32m  Bright Green\n\033[0m"
        "\033[33m  Yellow                      "
        "\033[1;33m  Bright Yellow\n\033[0m"
        "\033[34m  Blue                        "
        "\033[1;34m  Bright Blue\n\033[0m"
        "\033[35m  Magenta                     "
        "\033[1;35m  Bright Magenta\n\033[0m"
        "\033[36m  Cyan                        "
        "\033[1;36m  Bright Cyan\n\033[0m"
        "\033[37m  White                       "
        "\033[1;37m  Bright White\n\033[0m");
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
    QTextCharFormat currentFormat;
    currentFormat.setForeground(DefaultForeground);
    currentFormat.setBackground(DefaultBackground);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString before = text.mid(lastPos, match.capturedStart() - lastPos);
        cursor.insertText(before, currentFormat);

        QString codes = match.captured(1);
        QString command = match.captured(2);

        if (command == "m") {
            QStringList codeList = codes.split(';');
            for (const QString &code : codeList) {
                int codeInt = code.toInt();
                
                if (codeInt == 0) {
                    currentFormat = QTextCharFormat();
                    currentFormat.setForeground(DefaultForeground);
                    currentFormat.setBackground(DefaultBackground);
                }
                else if (codeInt == 1) {
                    QFont font = currentFormat.font();
                    font.setBold(true);
                    currentFormat.setFont(font);
                }
                else if (codeInt == 2) {
                    QFont font = currentFormat.font();
                    font.setBold(false);
                    currentFormat.setFont(font);
                }
                else if (codeInt == 3) {
                    QFont font = currentFormat.font();
                    font.setItalic(true);
                    currentFormat.setFont(font);
                }
                else if (codeInt == 4) {
                    QFont font = currentFormat.font();
                    font.setUnderline(true);
                    currentFormat.setFont(font);
                }
                else if (codeInt >= 30 && codeInt <= 37) {
                    currentFormat.setForeground(getAnsiColor(codeInt - 30, false));
                }
                else if (codeInt >= 90 && codeInt <= 97) {
                    currentFormat.setForeground(getAnsiColor(codeInt - 90, true));
                }
                else if (codeInt >= 40 && codeInt <= 47) {
                    currentFormat.setBackground(getAnsiColor(codeInt - 40, false));
                }
                else if (codeInt >= 100 && codeInt <= 107) {
                    currentFormat.setBackground(getAnsiColor(codeInt - 100, true));
                }
            }
        }

        lastPos = match.capturedEnd();
    }

    QString remaining = text.mid(lastPos);
    cursor.insertText(remaining, currentFormat);

    terminal->setTextCursor(cursor);
    terminal->ensureCursorVisible();
}
