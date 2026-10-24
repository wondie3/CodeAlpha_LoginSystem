#include "Dialogs.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "UserStore.h"
#include "Widgets.h"

ChangePasswordDialog::ChangePasswordDialog(const std::string& username, QWidget* parent)
    : QDialog(parent), m_username(username) {
    setWindowTitle(tr("Change password"));
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    m_currentEdit = new PasswordEdit(QString(), this);
    m_newEdit = new PasswordEdit(tr("at least 8 characters"), this);
    m_meter = new StrengthMeter(this);
    m_confirmEdit = new PasswordEdit(tr("repeat the new password"), this);
    form->addRow(tr("Current password"), m_currentEdit);
    form->addRow(tr("New password"), m_newEdit);
    form->addRow(QString(), m_meter);
    form->addRow(tr("Confirm new password"), m_confirmEdit);
    layout->addLayout(form);

    m_error = new QLabel(this);
    m_error->setObjectName("errorLabel");
    m_error->setWordWrap(true);
    m_error->hide();
    layout->addWidget(m_error);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Change password"));
    buttons->button(QDialogButtonBox::Ok)->setObjectName("primaryButton");
    connect(buttons, &QDialogButtonBox::accepted, this, &ChangePasswordDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_newEdit, &PasswordEdit::textChanged, m_meter, &StrengthMeter::setPassword);
}

void ChangePasswordDialog::accept() {
    auto fail = [this](const QString& text) {
        m_error->setText(text);
        m_error->show();
    };
    const std::string current = m_currentEdit->text().toStdString();
    const std::string next = m_newEdit->text().toStdString();
    std::string err;

    if (current.empty()) return fail(tr("Enter your current password."));
    if (!UserStore::validatePassword(next, m_username, err)) return fail(QString::fromStdString(err));
    if (m_confirmEdit->text().toStdString() != next) return fail(tr("The two new passwords do not match."));
    m_current = current;
    m_new = next;
    QDialog::accept();
}
