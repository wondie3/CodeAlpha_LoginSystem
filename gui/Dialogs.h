#pragma once

#include <QDialog>
#include <string>

class PasswordEdit;
class QLabel;
class StrengthMeter;

// Asks for the current password and a new one (with a strength meter).
class ChangePasswordDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChangePasswordDialog(const std::string& username, QWidget* parent = nullptr);
    std::string currentPassword() const { return m_current; }
    std::string newPassword() const { return m_new; }
    void accept() override;

private:
    std::string m_username;
    PasswordEdit *m_currentEdit, *m_newEdit, *m_confirmEdit;
    StrengthMeter* m_meter;
    QLabel* m_error;
    std::string m_current, m_new;
};
