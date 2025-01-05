#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>

class WelcomeView : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeView(QWidget *parent = nullptr);
    void updateRecentProjects(const QStringList &projects);

signals:
    void openFolder();
    void openFile();
    void openRecentProject(const QString &path);

private:
    void setupUI();
    QLabel *welcomeLabel;
    QWidget *recentProjectsWidget;
    QVBoxLayout *recentLayout;
};
