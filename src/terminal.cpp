#include "terminal.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QFontDatabase>
#include <QDir>
#include <QProcessEnvironment>
#include <QKeyEvent>
#include <QDebug>
#include <QHostInfo>

CommandLineEdit::CommandLineEdit(QWidget *parent) : QLineEdit(parent), historyIndex(-1) {
    setFocusPolicy(Qt::StrongFocus);
}

bool CommandLineEdit::event(QEvent *e) {
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Tab) {
            emit tabPressed();
            return true;  // Prevent focus change
        }
    }
    return QLineEdit::event(e);
}

void CommandLineEdit::addToHistory(const QString &command) {
    history.append(command);
    historyIndex = history.size();
}

Terminal::Terminal(QWidget *parent) : QWidget(parent), historyIndex(-1) {
    // Get username and hostname for prompt
    username = qgetenv("USER");
    if (username.isEmpty()) {
        username = qgetenv("USERNAME");
    }
    hostname = QHostInfo::localHostName();

    setupUI();
    setupProcess();
    setWorkingDirectory(QDir::homePath());
}

void Terminal::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Output display
    outputDisplay = new QPlainTextEdit(this);
    outputDisplay->setReadOnly(true);
    outputDisplay->setMaximumBlockCount(5000);
    outputDisplay->installEventFilter(this);  // Install event filter
    
    // Set monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    outputDisplay->setFont(font);
    
    // Set dark theme colors
    QPalette p = outputDisplay->palette();
    p.setColor(QPalette::Base, QColor("#1E1E1E"));
    p.setColor(QPalette::Text, QColor("#D4D4D4"));
    outputDisplay->setPalette(p);
    
    // Command input
    commandInput = new CommandLineEdit(this);
    commandInput->setFont(font);
    commandInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: #1E1E1E;"
        "   color: #D4D4D4;"
        "   border: none;"
        "   padding: 4px;"
        "}"
    );
    
    // Layout
    layout->addWidget(outputDisplay);
    layout->addWidget(commandInput);

    // Connect signals
    connect(commandInput, &CommandLineEdit::returnPressed, this, &Terminal::handleReturnPressed);
    connect(commandInput, &CommandLineEdit::tabPressed, this, &Terminal::handleTabCompletion);
    
    // Show initial prompt
    updatePrompt();
}

QString Terminal::getColoredPrompt() const {
    // Format: username@hostname:directory$
    QString dirName = QDir(currentWorkingDirectory).dirName();
    if (dirName.isEmpty()) {
        dirName = "/";
    }
    
    QString prompt;
    prompt += QString("<span style='color:#98C379'>%1@%2</span>").arg(username, hostname);  // Green
    prompt += QString("<span style='color:#61AFEF'>:%1</span>").arg(dirName);              // Blue
    prompt += "<span style='color:#E06C75'>$</span> ";                                     // Red
    
    return prompt;
}

void Terminal::updatePrompt() {
    appendOutput("\n" + getColoredPrompt(), QColor());  // Empty color for HTML formatting
}

void Terminal::handleReturnPressed() {
    QString command = commandInput->text();
    if (command.isEmpty()) {
        updatePrompt();
        return;
    }

    // Add to history
    commandInput->addToHistory(command);
    
    // Show the command with prompt
    appendOutput(command + "\n");
    
    // Execute the command
    executeCommand(command);
    
    // Clear the input field
    commandInput->clear();
}

bool Terminal::eventFilter(QObject *obj, QEvent *event) {
    if (obj == outputDisplay && event->type() == QEvent::FocusIn) {
        commandInput->setFocus();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void Terminal::appendOutput(const QString &text, const QColor &color) {
    QTextCursor cursor = outputDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    
    if (color.isValid()) {
        QTextCharFormat format;
        format.setForeground(color);
        cursor.insertText(text, format);
    } else {
        // If no color specified, treat as HTML
        cursor.insertHtml(text);
    }
    
    outputDisplay->setTextCursor(cursor);
    outputDisplay->ensureCursorVisible();
}

void Terminal::setupProcess()
{
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    
    // Set up environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    process->setProcessEnvironment(env);
    
    // Connect process signals
    connect(process, &QProcess::readyReadStandardOutput, this, &Terminal::onReadyReadStandardOutput);
    connect(process, &QProcess::readyReadStandardError, this, &Terminal::onReadyReadStandardError);
    connect(process, &QProcess::finished, this, &Terminal::onProcessFinished);
}

QString Terminal::getShell() {
#ifdef Q_OS_WIN
    return "cmd.exe";
#else
    return "/bin/bash";
#endif
}

void Terminal::handleTabCompletion() {
    QString currentText = commandInput->text();
    QString wordUnderCursor = currentText.section(' ', -1);
    
    if (wordUnderCursor.isEmpty()) {
        return;
    }

    QStringList completions = getCompletions(wordUnderCursor);
    
    if (completions.isEmpty()) {
        return;
    } else if (completions.size() == 1) {
        // Single completion - replace the word
        QString newText = currentText.left(currentText.length() - wordUnderCursor.length()) + completions.first();
        commandInput->setText(newText);
    } else {
        // Multiple completions - show them
        showCompletions(completions);
    }
}

QStringList Terminal::getCompletions(const QString &prefix) {
    QStringList completions;
    QDir dir(currentWorkingDirectory);
    
    // Add file completions
    for (const QFileInfo &info : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        QString name = info.fileName();
        if (name.startsWith(prefix)) {
            if (info.isDir()) {
                completions << name + "/";
            } else if (info.isExecutable()) {
                completions << name + "*";
            } else {
                completions << name;
            }
        }
    }
    
    // Add command completions from PATH
    if (!prefix.contains('/')) {
        QString pathEnv = qgetenv("PATH");
        QStringList paths = pathEnv.split(QDir::listSeparator());
        for (const QString &path : paths) {
            QDir pathDir(path);
            for (const QFileInfo &info : pathDir.entryInfoList(QDir::Files | QDir::Executable)) {
                QString name = info.fileName();
                if (name.startsWith(prefix)) {
                    completions << name;
                }
            }
        }
    }
    
    return completions;
}

void Terminal::showCompletions(const QStringList &completions) {
    QString completionText = "\n" + completions.join("  ") + "\n";
    appendOutput(completionText, QColor("#6A9955"));
    appendOutput(QString("%1 $ %2").arg(QDir(currentWorkingDirectory).dirName(), commandInput->text()), QColor("#569CD6"));
}

void Terminal::executeCommand(const QString &command)
{
#ifdef Q_OS_WIN
    process->start("cmd.exe", QStringList() << "/c" << command);
#else
    process->start("/bin/bash", QStringList() << "-c" << command);
#endif
}

void Terminal::setWorkingDirectory(const QString &path)
{
    if (path.isEmpty() || !QDir(path).exists()) {
        return;
    }

    currentWorkingDirectory = path;
    process->setWorkingDirectory(path);
    
    // Show current directory in prompt
    QString dirName = QDir(path).dirName();
    appendOutput(QString("\n%1 $ ").arg(dirName), QColor("#569CD6"));
}

void Terminal::onReadyReadStandardOutput()
{
    QByteArray output = process->readAllStandardOutput();
    appendOutput(QString::fromLocal8Bit(output));
}

void Terminal::onReadyReadStandardError()
{
    QByteArray error = process->readAllStandardError();
    appendOutput(QString::fromLocal8Bit(error), QColor("#F44747"));
}

void Terminal::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    appendOutput("\n$ ", QColor("#569CD6"));
}

// Override focusInEvent to set focus to command input
void Terminal::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    commandInput->setFocus();
}

void CommandLineEdit::keyPressEvent(QKeyEvent *event) {
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