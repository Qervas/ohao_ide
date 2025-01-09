#pragma once
#include <QDialog>
#include <QSpinBox>
#include <QFontComboBox>
#include <QCheckBox>

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    int getFontSize() const;
    QString getFontFamily() const;
    bool getWordWrap() const { return wordWrapCheckBox->isChecked(); }

private:
    QSpinBox *fontSizeSpinner;
    QFontComboBox *fontFamilyCombo;
    QCheckBox *wordWrapCheckBox;
};
