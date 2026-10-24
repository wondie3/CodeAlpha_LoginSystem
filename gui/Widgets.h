// Small reusable widgets: a password box with a Show/Hide button, and a strength meter.
#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QProgressBar;
class QToolButton;

class PasswordEdit : public QWidget {
    Q_OBJECT
public:
    explicit PasswordEdit(const QString& placeholder = QString(), QWidget* parent = nullptr);
    QString text() const;
    void clear();
    void setFocus();
    QLineEdit* lineEdit() const { return m_edit; }

signals:
    void textChanged(const QString& text);
    void returnPressed();

private:
    QLineEdit* m_edit;
    QToolButton* m_toggle;
};

class StrengthMeter : public QWidget {
    Q_OBJECT
public:
    explicit StrengthMeter(QWidget* parent = nullptr);
    void setPassword(const QString& password);

private:
    QProgressBar* m_bar;
    QLabel* m_label;
};
