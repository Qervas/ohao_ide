#include "terminal.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QFontDatabase>
#include <QDir>
#include <QProcessEnvironment>
#include <QKeyEvent>
#include <QDebug>

CommandLineEdit::CommandLineEdit(QWidget *parent) : QLineEdit(parent), historyIndex(-1) {}

void CommandLineEdit::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Tab:
            emit tabPressed();
            event->accept();  // Prevent event propagation
            return;
        case Qt::Key_Up:
            if (historyIndex > 0) {
                historyIndex--;
                setText(history.at(historyIndex));
            }
            event->accept();
            return;
        case Qt::Key_Down:
            if (historyIndex < history.size() - 1) {
                historyIndex++;
                setText(history.at(historyIndex));
            } else {
                historyIndex = history.size();
                clear();
            }
            event->accept();
            return;
    }
    QLineEdit::keyPressEvent(event);
}

Terminal::Terminal(QWidget *parent) : QWidget(parent), historyIndex(-1)
{
    setupUI();
    setupProcess();
    
    // Set initial working directory to home
    setWorkingDirectory(QDir::homePath());
}

void Terminal::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Output display
    outputDisplay = new QPlainTextEdit(this);
    outputDisplay->setReadOnly(true);
    outputDisplay->setMaximumBlockCount(5000);
    
    // Set monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    outputDisplay->setFont(font);
    
    // Command input
    commandInput = new CommandLineEdit(this);
    commandInput->setFont(font);
    commandInput->setPlaceholderText(tr("Enter command..."));
    
    // Layout
    layout->addWidget(outputDisplay);
    layout->addWidget(commandInput);

    // Connect signals
    connect(commandInput, &CommandLineEdit::returnPressed, this, &Terminal::handleReturnPressed);
    connect(commandInput, &CommandLineEdit::tabPressed, this, &Terminal::handleTabCompletion);
    
    // Initialize with prompt
    appendOutput(QString("%1 $ ").arg(QDir::homePath()), QColor("#569CD6"));
    
    // Make sure the terminal is focusable
    setFocusPolicy(Qt::StrongFocus);
    commandInput->setFocusPolicy(Qt::StrongFocus);
    outputDisplay->setFocusPolicy(Qt::StrongFocus);
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

void Terminal::handleReturnPressed()
{
    QString command = commandInput->text();
    if (command.isEmpty()) {
        appendOutput(QString("\n%1 $ ").arg(QDir(currentWorkingDirectory).dirName()), QColor("#569CD6"));
        return;
    }

    // Add to history
    commandHistory.append(command);
    historyIndex = commandHistory.size();

    // Show the command
    appendOutput(command + "\n");
    
    // Execute the command
    executeCommand(command);
    
    // Clear the input field
    commandInput->clear();
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

void Terminal::appendOutput(const QString &text, const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);
    
    QTextCursor cursor = outputDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);
    outputDisplay->setTextCursor(cursor);
    outputDisplay->ensureCursorVisible();
}

// Override focusInEvent to set focus to command input
void Terminal::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    commandInput->setFocus();
} 