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
    fontComboBox = new QFontComboBox(this);
    fontComboBox->setFontFilters(QFontComboBox::MonospacedFonts);
    fontLayout->addWidget(new QLabel(tr("Font:")), 0, 0);
    fontLayout->addWidget(fontComboBox, 0, 1);

    // Font size
    fontSizeSpinBox = new QSpinBox(this);
    fontSizeSpinBox->setRange(6, 72);
    fontLayout->addWidget(new QLabel(tr("Size:")), 1, 0);
    fontLayout->addWidget(fontSizeSpinBox, 1, 1);

    mainLayout->addWidget(fontGroup);

    // Editor settings group
    QGroupBox *editorGroup = new QGroupBox(tr("Editor Settings"), this);
    QVBoxLayout *editorLayout = new QVBoxLayout(editorGroup);

    // Word wrap option
    wordWrapCheckBox = new QCheckBox(tr("Enable Word Wrap"), this);
    editorLayout->addWidget(wordWrapCheckBox);

    // Intelligent indent setting
    intelligentIndentCheckBox = new QCheckBox(tr("Enable intelligent indent"), this);
    editorLayout->addWidget(intelligentIndentCheckBox);

    // Syntax highlighting setting
    syntaxHighlightingCheckBox = new QCheckBox(tr("Enable syntax highlighting"), this);
    editorLayout->addWidget(syntaxHighlightingCheckBox);

    mainLayout->addWidget(editorGroup);

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
    fontComboBox->setCurrentFont(QFont(settings.value("editor/fontFamily", "Monospace").toString()));
    fontSizeSpinBox->setValue(settings.value("editor/fontSize", 11).toInt());
    wordWrapCheckBox->setChecked(settings.value("editor/wordWrap", true).toBool());
    intelligentIndentCheckBox->setChecked(settings.value("editor/intelligentIndent", true).toBool());
    syntaxHighlightingCheckBox->setChecked(settings.value("editor/syntaxHighlighting", true).toBool());

    // Connect buttons
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

int PreferencesDialog::getFontSize() const {
    return fontSizeSpinBox->value();
}

QString PreferencesDialog::getFontFamily() const {
    return fontComboBox->currentFont().family();
}

bool PreferencesDialog::getWordWrap() const {
    return wordWrapCheckBox->isChecked();
}

bool PreferencesDialog::getIntelligentIndent() const {
    return intelligentIndentCheckBox->isChecked();
}

bool PreferencesDialog::getSyntaxHighlighting() const {
    return syntaxHighlightingCheckBox->isChecked();
}
