#pragma once

#include <QDialog>
#include <QTextDocument>
#include <QPlainTextEdit>

class QLineEdit;
class QCheckBox;
class QPushButton;

class SearchDialog : public QDialog {
    Q_OBJECT

public:
    explicit SearchDialog(QPlainTextEdit *editor, QWidget *parent = nullptr);
    ~SearchDialog();

    void showFind();
    void showReplace();
    void findNext();
    void findPrevious();

private slots:
    void find();
    void replace();
    void replaceAll();
    void updateSearchHighlight();
    void clearSearchHighlights();

private:
    bool findText(const QString &text, QTextDocument::FindFlags flags);

    QPlainTextEdit *m_editor;
    QLineEdit *m_findLineEdit;
    QLineEdit *m_replaceLineEdit;
    QCheckBox *m_caseSensitiveCheckBox;
    QCheckBox *m_wholeWordsCheckBox;
    QCheckBox *m_regexCheckBox;
    QPushButton *m_findNextButton;
    QPushButton *m_findPrevButton;
    QPushButton *m_replaceButton;
    QPushButton *m_replaceAllButton;

    QString m_lastSearchText;
    QTextDocument::FindFlags m_searchFlags;
}; 