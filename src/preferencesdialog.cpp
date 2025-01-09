#include "preferencesdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QFontComboBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QGridLayout>
#include <QCheckBox>

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Preferences"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Font settings group
    QGroupBox *fontGroup = new QGroupBox(tr("Editor Font"), this);
    QGridLayout *fontLayout = new QGridLayout(fontGroup);

    // Font family
    fontFamilyCombo = new QFontComboBox(this);
    fontLayout->addWidget(new QLabel(tr("Font:")), 0, 0);
    fontLayout->addWidget(fontFamilyCombo, 0, 1);

    // Font size
    fontSizeSpinner = new QSpinBox(this);
    fontSizeSpinner->setRange(8, 72);
    fontLayout->addWidget(new QLabel(tr("Size:")), 1, 0);
    fontLayout->addWidget(fontSizeSpinner, 1, 1);

    // Word wrap option
    wordWrapCheckBox = new QCheckBox(tr("Enable Word Wrap"), this);
    fontLayout->addWidget(wordWrapCheckBox, 2, 0, 1, 2);

    mainLayout->addWidget(fontGroup);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"), this);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Load current settings
    QSettings settings;
    fontFamilyCombo->setCurrentFont(QFont(settings.value("editor/fontFamily", "Monospace").toString()));
    fontSizeSpinner->setValue(settings.value("editor/fontSize", 11).toInt());
    wordWrapCheckBox->setChecked(settings.value("editor/wordWrap", true).toBool());

    // Connect buttons
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

int PreferencesDialog::getFontSize() const {
    return fontSizeSpinner->value();
}

QString PreferencesDialog::getFontFamily() const {
    return fontFamilyCombo->currentFont().family();
}
