#include "AuthPage.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>

#include "Widgets.h"

namespace {

QLabel* makeMessageLabel(QWidget* parent) {
    auto* l = new QLabel(parent);
    l->setWordWrap(true);
    l->hide();
    return l;
}

void showMessage(QLabel* label, const QString& text, const char* objectName) {
    label->setObjectName(objectName);
    label->style()->unpolish(label);
    label->style()->polish(label);
    label->setText(text);
    label->setVisible(!text.isEmpty());
}

}  // namespace

AuthPage::AuthPage(QWidget* parent) : QWidget(parent) {
    setObjectName("authPage");

    auto* card = new QFrame(this);
    card->setObjectName("card");
    card->setFixedWidth(440);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(30, 28, 30, 24);
    cardLayout->setSpacing(6);

    auto* title = new QLabel(tr("CodeAlpha Secure Login"), card);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    auto* subtitle = new QLabel(tr("Log in, or create a new account"), card);
    subtitle->setObjectName("subtitle");
    subtitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);
    cardLayout->addWidget(subtitle);
    cardLayout->addSpacing(8);

    m_tabs = new QTabWidget(card);
    cardLayout->addWidget(m_tabs);

    // ------------------------------ Login tab ------------------------------
    auto* loginTab = new QWidget;
    auto* ll = new QVBoxLayout(loginTab);
    ll->setSpacing(8);
    ll->addSpacing(8);
    ll->addWidget(new QLabel(tr("Username"), loginTab));
    m_loginUser = new QLineEdit(loginTab);
    m_loginUser->setMaxLength(20);
    m_loginUser->setPlaceholderText(tr("your username"));
    ll->addWidget(m_loginUser);
    ll->addWidget(new QLabel(tr("Password"), loginTab));
    m_loginPass = new PasswordEdit(tr("your password"), loginTab);
    ll->addWidget(m_loginPass);
    m_loginMessage = makeMessageLabel(loginTab);
    ll->addWidget(m_loginMessage);
    auto* loginBtn = new QPushButton(tr("Log in"), loginTab);
    loginBtn->setObjectName("primaryButton");
    loginBtn->setDefault(true);
    loginBtn->setCursor(Qt::PointingHandCursor);
    ll->addSpacing(4);
    ll->addWidget(loginBtn);
    ll->addStretch(1);
    m_tabs->addTab(loginTab, tr("Log in"));

    // ---------------------------- Register tab -----------------------------
    auto* regTab = new QWidget;
    auto* rl = new QVBoxLayout(regTab);
    rl->setSpacing(8);
    rl->addSpacing(8);
    rl->addWidget(new QLabel(tr("Choose a username"), regTab));
    m_regUser = new QLineEdit(regTab);
    m_regUser->setMaxLength(20);
    m_regUser->setPlaceholderText(tr("3-20 letters, digits or _"));
    rl->addWidget(m_regUser);
    rl->addWidget(new QLabel(tr("Choose a password"), regTab));
    m_regPass = new PasswordEdit(tr("at least 8 characters"), regTab);
    rl->addWidget(m_regPass);
    m_meter = new StrengthMeter(regTab);
    rl->addWidget(m_meter);
    auto* rules = new QLabel(tr("8-64 characters with an uppercase letter, a lowercase letter and a digit; "
                                "no spaces."), regTab);
    rules->setObjectName("hint");
    rules->setWordWrap(true);
    rl->addWidget(rules);
    rl->addWidget(new QLabel(tr("Confirm password"), regTab));
    m_regConfirm = new PasswordEdit(tr("repeat the password"), regTab);
    rl->addWidget(m_regConfirm);
    m_regError = makeMessageLabel(regTab);
    rl->addWidget(m_regError);
    auto* regBtn = new QPushButton(tr("Create account"), regTab);
    regBtn->setObjectName("primaryButton");
    regBtn->setCursor(Qt::PointingHandCursor);
    rl->addSpacing(4);
    rl->addWidget(regBtn);
    m_tabs->addTab(regTab, tr("Register"));

    // ------------------------------ wiring ---------------------------------
    connect(loginBtn, &QPushButton::clicked, this, &AuthPage::submitLogin);
    connect(m_loginPass, &PasswordEdit::returnPressed, this, &AuthPage::submitLogin);
    connect(m_loginUser, &QLineEdit::returnPressed, m_loginPass, &PasswordEdit::setFocus);
    connect(regBtn, &QPushButton::clicked, this, &AuthPage::submitRegister);
    connect(m_regConfirm, &PasswordEdit::returnPressed, this, &AuthPage::submitRegister);
    connect(m_regPass, &PasswordEdit::textChanged, m_meter, &StrengthMeter::setPassword);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this] {
        showMessage(m_loginMessage, QString(), "errorLabel");
        showMessage(m_regError, QString(), "errorLabel");
    });

    // centre the card
    auto* outer = new QHBoxLayout(this);
    outer->addStretch(1);
    auto* column = new QVBoxLayout;
    column->addStretch(1);
    column->addWidget(card);
    column->addStretch(1);
    outer->addLayout(column);
    outer->addStretch(1);
}

void AuthPage::submitLogin() {
    showMessage(m_loginMessage, QString(), "errorLabel");
    if (m_loginUser->text().trimmed().isEmpty()) {
        showLoginError(tr("Enter your username."));
        m_loginUser->setFocus();
        return;
    }
    if (m_loginPass->text().isEmpty()) {
        showLoginError(tr("Enter your password."));
        m_loginPass->setFocus();
        return;
    }
    emit loginRequested(m_loginUser->text(), m_loginPass->text());
}

void AuthPage::submitRegister() {
    showMessage(m_regError, QString(), "errorLabel");
    if (m_regPass->text() != m_regConfirm->text()) {
        showRegisterError(tr("The two passwords do not match."));
        m_regConfirm->setFocus();
        return;
    }
    emit registerRequested(m_regUser->text(), m_regPass->text());
}

void AuthPage::showLoginError(const QString& text) { showMessage(m_loginMessage, text, "errorLabel"); }

void AuthPage::showLoginInfo(const QString& text) { showMessage(m_loginMessage, text, "successLabel"); }

void AuthPage::showRegisterError(const QString& text) { showMessage(m_regError, text, "errorLabel"); }

void AuthPage::clearLoginPassword() {
    m_loginPass->clear();
    m_loginPass->setFocus();
}

void AuthPage::switchToLogin(const QString& username) {
    m_regUser->clear();
    m_regPass->clear();
    m_regConfirm->clear();
    m_tabs->setCurrentIndex(0);
    m_loginUser->setText(username);
    m_loginPass->clear();
    m_loginPass->setFocus();
}

void AuthPage::reset() {
    m_loginUser->clear();
    m_loginPass->clear();
    m_regUser->clear();
    m_regPass->clear();
    m_regConfirm->clear();
    showMessage(m_loginMessage, QString(), "errorLabel");
    showMessage(m_regError, QString(), "errorLabel");
    m_tabs->setCurrentIndex(0);
    m_loginUser->setFocus();
}
