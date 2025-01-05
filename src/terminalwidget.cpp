#include "terminalwidget.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QFontDatabase>
#include <QDir>
#include <QProcessEnvironment>
#include <QKeyEvent>
#include <QDebug>
#include <QHostInfo>

TerminalWidget::TerminalWidget(QWidget *parent) : QWidget(parent) {
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

void TerminalWidget::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Output display
    outputDisplay = new QPlainTextEdit(this);
    outputDisplay->setReadOnly(true);
    outputDisplay->setMaximumBlockCount(5000);
    outputDisplay->installEventFilter(this);

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
    connect(commandInput, &CommandLineEdit::returnPressed, this, &TerminalWidget::handleReturnPressed);
    connect(commandInput, &CommandLineEdit::tabPressed, this, &TerminalWidget::handleTabCompletion);

    // Show initial prompt
    updatePrompt();
}

void TerminalWidget::setupProcess() {
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    // Set up environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    process->setProcessEnvironment(env);

    // Connect process signals
    connect(process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onReadyReadStandardOutput);
    connect(process, &QProcess::readyReadStandardError, this, &TerminalWidget::onReadyReadStandardError);
    connect(process, &QProcess::finished, this, &TerminalWidget::onProcessFinished);
}

QString TerminalWidget::getColoredPrompt() const {
    QString dirName = QDir(currentWorkingDirectory).dirName();
    if (dirName.isEmpty()) {
        dirName = "/";
    }

    QString prompt;
    prompt += QString("<span style='color:#98C379'>%1@%2</span>").arg(username, hostname);
    prompt += QString("<span style='color:#61AFEF'>:%1</span>").arg(dirName);
    prompt += "<span style='color:#E06C75'>$</span> ";

    return prompt;
}

void TerminalWidget::updatePrompt() {
    appendOutput("\n" + getColoredPrompt(), QColor());
}

void TerminalWidget::handleReturnPressed() {
    QString command = commandInput->text();
    if (command.isEmpty()) {
        updatePrompt();
        return;
    }

    commandInput->addToHistory(command);
    appendOutput(command + "\n");
    executeCommand(command);
    commandInput->clear();
}

void TerminalWidget::executeCommand(const QString &command) {
#ifdef Q_OS_WIN
    process->start("cmd.exe", QStringList() << "/c" << command);
#else
    process->start("/bin/bash", QStringList() << "-c" << command);
#endif
}

void TerminalWidget::setWorkingDirectory(const QString &path) {
    if (path.isEmpty() || !QDir(path).exists()) {
        return;
    }

    currentWorkingDirectory = path;
    process->setWorkingDirectory(path);
}

void TerminalWidget::appendOutput(const QString &text, const QColor &color) {
    QTextCursor cursor = outputDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (color.isValid()) {
        QTextCharFormat format;
        format.setForeground(color);
        cursor.insertText(text, format);
    } else {
        cursor.insertHtml(text);
    }

    outputDisplay->setTextCursor(cursor);
    outputDisplay->ensureCursorVisible();
}

bool TerminalWidget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == outputDisplay && event->type() == QEvent::FocusIn) {
        commandInput->setFocus();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void TerminalWidget::focusInEvent(QFocusEvent *event) {
    QWidget::focusInEvent(event);
    commandInput->setFocus();
}

void TerminalWidget::onReadyReadStandardOutput() {
    QByteArray output = process->readAllStandardOutput();
    appendOutput(QString::fromLocal8Bit(output));
}

void TerminalWidget::onReadyReadStandardError() {
    QByteArray error = process->readAllStandardError();
    appendOutput(QString::fromLocal8Bit(error), QColor("#F44747"));
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    updatePrompt();
}

void TerminalWidget::handleTabCompletion() {
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

QStringList TerminalWidget::getCompletions(const QString &prefix) {
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

void TerminalWidget::showCompletions(const QStringList &completions) {
    QString completionText = "\n" + completions.join("  ") + "\n";
    appendOutput(completionText, QColor("#6A9955"));  // Green color for completions
    appendOutput(getColoredPrompt() + commandInput->text(), QColor());
}
