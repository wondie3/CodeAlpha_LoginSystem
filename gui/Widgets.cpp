#include "Widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QStyle>
#include <QToolButton>

#include "UserStore.h"

PasswordEdit::PasswordEdit(const QString& placeholder, QWidget* parent) : QWidget(parent) {
    m_edit = new QLineEdit(this);
    m_edit->setEchoMode(QLineEdit::Password);
    m_edit->setMaxLength(64);
    m_edit->setPlaceholderText(placeholder);

    m_toggle = new QToolButton(this);
    m_toggle->setObjectName("showButton");
    m_toggle->setText(tr("Show"));
    m_toggle->setCheckable(true);
    m_toggle->setCursor(Qt::PointingHandCursor);
    m_toggle->setFocusPolicy(Qt::NoFocus);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(m_edit, 1);
    layout->addWidget(m_toggle);

    connect(m_toggle, &QToolButton::toggled, this, [this](bool show) {
        m_edit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
        m_toggle->setText(show ? tr("Hide") : tr("Show"));
    });
    connect(m_edit, &QLineEdit::textChanged, this, &PasswordEdit::textChanged);
    connect(m_edit, &QLineEdit::returnPressed, this, &PasswordEdit::returnPressed);
}

QString PasswordEdit::text() const { return m_edit->text(); }

void PasswordEdit::clear() { m_edit->clear(); }

void PasswordEdit::setFocus() { m_edit->setFocus(); }

StrengthMeter::StrengthMeter(QWidget* parent) : QWidget(parent) {
    m_bar = new QProgressBar(this);
    m_bar->setRange(0, 4);
    m_bar->setTextVisible(false);
    m_bar->setFixedHeight(8);
    m_label = new QLabel(this);
    m_label->setObjectName("strengthLabel");
    m_label->setMinimumWidth(70);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_bar, 1);
    layout->addWidget(m_label);
    setPassword(QString());
}

void StrengthMeter::setPassword(const QString& password) {
    static const char* names[] = {"", "Weak", "Fair", "Good", "Strong"};
    static const char* colors[] = {"#64748b", "#dc2626", "#ea580c", "#ca8a04", "#16a34a"};
    const int score = UserStore::passwordScore(password.toStdString());
    m_bar->setValue(score);
    m_bar->setProperty("level", score);
    m_bar->style()->unpolish(m_bar);
    m_bar->style()->polish(m_bar);
    m_label->setText(names[score]);
    m_label->setStyleSheet(QString("color: %1;").arg(colors[score]));
}
