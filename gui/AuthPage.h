#pragma once

#include <QWidget>

class PasswordEdit;
class QLabel;
class QLineEdit;
class QTabWidget;
class StrengthMeter;

// The sign-in / registration screen: two tabs inside a centred card.
class AuthPage : public QWidget {
    Q_OBJECT
public:
    explicit AuthPage(QWidget* parent = nullptr);

    void showLoginError(const QString& text);
    void showLoginInfo(const QString& text);
    void showRegisterError(const QString& text);
    void clearLoginPassword();
    void switchToLogin(const QString& username);   // after registering: prefill the name
    void reset();                                  // clear everything (after log-out)

signals:
    void loginRequested(const QString& username, const QString& password);
    void registerRequested(const QString& username, const QString& password);

private:
    void submitLogin();
    void submitRegister();

    QTabWidget* m_tabs;
    // login tab
    QLineEdit* m_loginUser;
    PasswordEdit* m_loginPass;
    QLabel* m_loginMessage;
    // register tab
    QLineEdit* m_regUser;
    PasswordEdit* m_regPass;
    PasswordEdit* m_regConfirm;
    StrengthMeter* m_meter;
    QLabel* m_regError;
};
