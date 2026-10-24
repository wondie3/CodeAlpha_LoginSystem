#pragma once

#include <QMainWindow>
#include <string>

#include "UserStore.h"

class AuthPage;
class QCloseEvent;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(UserStore store, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onLogin(const QString& username, const QString& password);
    void onRegister(const QString& username, const QString& password);
    void logout();
    void changePassword();
    void exportActivity();
    void exportUsers();
    void showAbout();

private:
    QWidget* buildDashboard();
    void buildMenus();
    void enterDashboard(const std::string& username);
    void refreshAll();
    void refreshProfile();
    void refreshActivity();
    void refreshUsers();
    bool isAdmin() const;
    bool persist();   // saves; shows an error dialog on failure
    void saveCsvWithDialog(const QString& defaultName, const std::string& content);

    UserStore m_store;
    std::string m_current;          // empty = nobody logged in
    std::time_t m_loginTime = 0;

    QStackedWidget* m_stack = nullptr;
    AuthPage* m_auth = nullptr;
    QLabel* m_welcome = nullptr;
    QLabel* m_badge = nullptr;
    QLabel *m_pUser = nullptr, *m_pRole = nullptr, *m_pCreated = nullptr, *m_pSince = nullptr,
           *m_pEvents = nullptr;
    QTabWidget* m_tabs = nullptr;
    QTableWidget* m_activityTable = nullptr;
    QTableWidget* m_usersTable = nullptr;
    QLabel* m_activityHint = nullptr;
    QPushButton* m_exportUsersBtn = nullptr;
    QAction *m_actExportActivity = nullptr, *m_actExportUsers = nullptr, *m_actChangePw = nullptr,
            *m_actLogout = nullptr;
};
