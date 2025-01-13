#pragma once
#include <QDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    int getFontSize() const;
    QString getFontFamily() const;
    bool getWordWrap() const;
    bool getIntelligentIndent() const;

private:
    QFontComboBox *fontComboBox;
    QSpinBox *fontSizeSpinBox;
    QCheckBox *wordWrapCheckBox;
    QCheckBox *intelligentIndentCheckBox;
};
