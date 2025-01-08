#pragma once
#include <QDialog>
#include <QSpinBox>
#include <QFontComboBox>

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    int getFontSize() const;
    QString getFontFamily() const;

private:
    QSpinBox *fontSizeSpinner;
    QFontComboBox *fontFamilyCombo;
};
