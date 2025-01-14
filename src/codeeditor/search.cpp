#include "search.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextCursor>

SearchDialog::SearchDialog(QPlainTextEdit *editor, QWidget *parent)
    : QDialog(parent), m_editor(editor) {
    setWindowTitle(tr("Find"));
    
    // Create find layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *findInputLayout = new QHBoxLayout;
    m_findLineEdit = new QLineEdit(this);
    findInputLayout->addWidget(new QLabel(tr("Find:")));
    findInputLayout->addWidget(m_findLineEdit);
    mainLayout->addLayout(findInputLayout);

    // Create options layout
    QHBoxLayout *optionsLayout = new QHBoxLayout;
    m_caseSensitiveCheckBox = new QCheckBox(tr("Case Sensitive"), this);
    m_wholeWordsCheckBox = new QCheckBox(tr("Whole Words"), this);
    m_regexCheckBox = new QCheckBox(tr("Regular Expression"), this);
    optionsLayout->addWidget(m_caseSensitiveCheckBox);
    optionsLayout->addWidget(m_wholeWordsCheckBox);
    optionsLayout->addWidget(m_regexCheckBox);
    mainLayout->addLayout(optionsLayout);

    // Create find buttons layout
    QHBoxLayout *findButtonLayout = new QHBoxLayout;
    m_findNextButton = new QPushButton(tr("Find Next"), this);
    m_findPrevButton = new QPushButton(tr("Find Previous"), this);
    QPushButton *findCloseButton = new QPushButton(tr("Close"), this);
    findButtonLayout->addWidget(m_findNextButton);
    findButtonLayout->addWidget(m_findPrevButton);
    findButtonLayout->addWidget(findCloseButton);
    mainLayout->addLayout(findButtonLayout);

    // Create replace layout
    QHBoxLayout *replaceInputLayout = new QHBoxLayout;
    m_replaceLineEdit = new QLineEdit(this);
    replaceInputLayout->addWidget(new QLabel(tr("Replace with:")));
    replaceInputLayout->addWidget(m_replaceLineEdit);
    mainLayout->addLayout(replaceInputLayout);

    // Create replace buttons layout
    QHBoxLayout *replaceButtonLayout = new QHBoxLayout;
    m_replaceButton = new QPushButton(tr("Replace"), this);
    m_replaceAllButton = new QPushButton(tr("Replace All"), this);
    replaceButtonLayout->addWidget(m_replaceButton);
    replaceButtonLayout->addWidget(m_replaceAllButton);
    mainLayout->addLayout(replaceButtonLayout);

    // Connect signals
    connect(m_findNextButton, &QPushButton::clicked, this, &SearchDialog::find);
    connect(m_findPrevButton, &QPushButton::clicked, this, [this]() {
        m_searchFlags |= QTextDocument::FindBackward;
        find();
    });
    connect(findCloseButton, &QPushButton::clicked, this, &QDialog::hide);
    connect(m_replaceButton, &QPushButton::clicked, this, &SearchDialog::replace);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &SearchDialog::replaceAll);
    connect(m_findLineEdit, &QLineEdit::textChanged, this, &SearchDialog::updateSearchHighlight);
}

SearchDialog::~SearchDialog() {
}

void SearchDialog::showFind() {
    show();
    raise();
    activateWindow();
    m_findLineEdit->setFocus();
    m_findLineEdit->selectAll();
}

void SearchDialog::showReplace() {
    show();
    if (!isVisible()) {
        show();
    }
    m_replaceLineEdit->setFocus();
    m_replaceLineEdit->selectAll();
}

void SearchDialog::findNext() {
    if (!m_lastSearchText.isEmpty()) {
        m_searchFlags &= ~QTextDocument::FindBackward;
        findText(m_lastSearchText, m_searchFlags);
    } else {
        showFind();
    }
}

void SearchDialog::findPrevious() {
    if (!m_lastSearchText.isEmpty()) {
        m_searchFlags |= QTextDocument::FindBackward;
        findText(m_lastSearchText, m_searchFlags);
    } else {
        showFind();
    }
}

void SearchDialog::find() {
    QString searchText = m_findLineEdit->text();
    if (searchText.isEmpty()) {
        return;
    }

    m_searchFlags = QTextDocument::FindFlags();
    if (m_caseSensitiveCheckBox->isChecked()) {
        m_searchFlags |= QTextDocument::FindCaseSensitively;
    }
    if (m_wholeWordsCheckBox->isChecked()) {
        m_searchFlags |= QTextDocument::FindWholeWords;
    }

    m_lastSearchText = searchText;
    if (!findText(searchText, m_searchFlags)) {
        QMessageBox::information(this, tr("Find"),
                               tr("No more occurrences found."));
    }
}

void SearchDialog::replace() {
    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == m_lastSearchText) {
        cursor.insertText(m_replaceLineEdit->text());
        find(); // Find next occurrence
    } else {
        find(); // Find first occurrence
    }
}

void SearchDialog::replaceAll() {
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    int count = 0;
    while (findText(m_lastSearchText, m_searchFlags)) {
        cursor = m_editor->textCursor();
        cursor.insertText(m_replaceLineEdit->text());
        count++;
    }

    QMessageBox::information(this, tr("Replace All"),
                           tr("Replaced %1 occurrence(s).").arg(count));
}

bool SearchDialog::findText(const QString &text, QTextDocument::FindFlags flags) {
    if (m_regexCheckBox->isChecked()) {
        QRegularExpression regex(text);
        if (!regex.isValid()) {
            QMessageBox::warning(
                this, tr("Invalid Regular Expression"),
                tr("The regular expression is invalid: %1").arg(regex.errorString()));
            return false;
        }

        QRegularExpression::PatternOptions options;
        if (!(flags & QTextDocument::FindCaseSensitively)) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }
        regex.setPatternOptions(options);

        QString documentText = m_editor->toPlainText();
        QTextCursor cursor = m_editor->textCursor();
        int searchFrom = cursor.position();

        QRegularExpressionMatch match;
        if (flags & QTextDocument::FindBackward) {
            // For backward search, find all matches up to cursor and take the last one
            int endPos = cursor.position();
            QRegularExpressionMatchIterator it =
                regex.globalMatch(documentText.left(endPos));
            QRegularExpressionMatch lastMatch;
            while (it.hasNext()) {
                lastMatch = it.next();
            }
            match = lastMatch;
        } else {
            match = regex.match(documentText, searchFrom);
        }

        if (match.hasMatch()) {
            cursor.setPosition(match.capturedStart());
            cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
            m_editor->setTextCursor(cursor);
            return true;
        }
    } else {
        return m_editor->find(text, flags);
    }

    return false;
}

void SearchDialog::updateSearchHighlight() {
    clearSearchHighlights();

    QString searchText = m_findLineEdit->text();
    if (searchText.isEmpty()) {
        return;
    }

    QTextCharFormat format;
    format.setBackground(QColor(255, 255, 0, 100)); // Light yellow highlight

    QTextCursor cursor = m_editor->textCursor();
    int originalPosition = cursor.position();
    cursor.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    QTextDocument::FindFlags flags = QTextDocument::FindFlags();
    if (m_caseSensitiveCheckBox->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (m_wholeWordsCheckBox->isChecked()) {
        flags |= QTextDocument::FindWholeWords;
    }

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (m_regexCheckBox->isChecked()) {
        QRegularExpression regex(searchText);
        if (!regex.isValid()) {
            return;
        }

        QRegularExpression::PatternOptions options =
            QRegularExpression::NoPatternOption;
        if (!(flags & QTextDocument::FindCaseSensitively)) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }
        regex.setPatternOptions(options);

        QString documentText = m_editor->toPlainText();
        QRegularExpressionMatchIterator it = regex.globalMatch(documentText);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QTextCursor matchCursor = m_editor->textCursor();
            matchCursor.setPosition(match.capturedStart());
            matchCursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);

            QTextEdit::ExtraSelection selection;
            selection.format = format;
            selection.cursor = matchCursor;
            extraSelections.append(selection);
        }
    } else {
        // Store original flags to restore after highlighting
        QTextDocument::FindFlags originalFlags = m_searchFlags;
        m_searchFlags = flags; // Use current flags for highlighting

        while (m_editor->find(searchText, flags)) {
            QTextEdit::ExtraSelection selection;
            selection.format = format;
            selection.cursor = m_editor->textCursor();
            extraSelections.append(selection);
        }

        m_searchFlags = originalFlags; // Restore original flags
    }

    // Restore original cursor position
    cursor.setPosition(originalPosition);
    m_editor->setTextCursor(cursor);
    m_editor->setExtraSelections(extraSelections);
}

void SearchDialog::clearSearchHighlights() {
    QList<QTextEdit::ExtraSelection> selections = m_editor->extraSelections();
    selections.removeIf([](const QTextEdit::ExtraSelection &selection) {
        return selection.format.background() == QColor(255, 255, 0, 100);
    });
    m_editor->setExtraSelections(selections);
} 