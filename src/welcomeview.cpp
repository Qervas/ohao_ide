#include "welcomeview.h"
#include <QDir>
#include <QFont>

WelcomeView::WelcomeView(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void WelcomeView::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    // Welcome header
    welcomeLabel = new QLabel(tr("Modern C++ IDE"));
    QFont headerFont = welcomeLabel->font();
    headerFont.setPointSize(24);
    welcomeLabel->setFont(headerFont);
    welcomeLabel->setAlignment(Qt::AlignCenter);

    // Quick actions section
    QLabel* actionsLabel = new QLabel(tr("Quick Actions"));
    QFont sectionFont = actionsLabel->font();
    sectionFont.setBold(true);
    actionsLabel->setFont(sectionFont);

    // Action buttons
    QPushButton* openFolderBtn = new QPushButton(tr("Open Folder..."));
    QPushButton* openFileBtn = new QPushButton(tr("Open File..."));

    QString buttonStyle =
        "QPushButton {"
        "    background-color: #0E639C;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1177BB;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #0D5789;"
        "}";

    openFolderBtn->setStyleSheet(buttonStyle);
    openFileBtn->setStyleSheet(buttonStyle);

    // Recent projects section
    QLabel* recentLabel = new QLabel(tr("Recent Projects"));
    recentLabel->setFont(sectionFont);

    // Recent projects list
    recentProjectsWidget = new QWidget;
    recentLayout = new QVBoxLayout(recentProjectsWidget);
    recentLayout->setSpacing(4);
    recentLayout->setContentsMargins(0, 0, 0, 0);

    // Add all widgets to main layout
    layout->addWidget(welcomeLabel);
    layout->addSpacing(20);
    layout->addWidget(actionsLabel);
    layout->addWidget(openFolderBtn);
    layout->addWidget(openFileBtn);
    layout->addSpacing(20);
    layout->addWidget(recentLabel);
    layout->addWidget(recentProjectsWidget);
    layout->addStretch();

    // Connect buttons
    connect(openFolderBtn, &QPushButton::clicked, this, &WelcomeView::openFolder);
    connect(openFileBtn, &QPushButton::clicked, this, &WelcomeView::openFile);
}

void WelcomeView::updateRecentProjects(const QStringList &projects) {
    // Clear existing items
    QLayoutItem *item;
    while ((item = recentLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // Add new items
    for (const QString& project : projects) {
        QPushButton* projectBtn = new QPushButton(QDir(project).dirName());
        projectBtn->setToolTip(project);
        projectBtn->setFlat(true);
        projectBtn->setCursor(Qt::PointingHandCursor);
        projectBtn->setStyleSheet(
            "QPushButton {"
            "    text-align: left;"
            "    padding: 4px 8px;"
            "    color: #0E639C;"
            "}"
            "QPushButton:hover {"
            "    background-color: #2A2D2E;"
            "}"
        );

        connect(projectBtn, &QPushButton::clicked, this,
                [this, project]() { emit openRecentProject(project); });

        recentLayout->addWidget(projectBtn);
    }
    recentLayout->addStretch();
}
