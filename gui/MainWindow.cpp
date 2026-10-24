#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "AuthPage.h"
#include "Csv.h"
#include "Dialogs.h"

namespace {

QString qs(const std::string& s) { return QString::fromStdString(s); }

QString dateText(std::time_t t) {
    if (t <= 0) return QObject::tr("Never");
    return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(t)).toString("yyyy-MM-dd HH:mm:ss");
}

QTableWidget* makeTable(QWidget* parent, const QStringList& headers) {
    auto* t = new QTableWidget(0, headers.size(), parent);
    t->setHorizontalHeaderLabels(headers);
    t->verticalHeader()->hide();
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setHighlightSections(false);
    t->verticalHeader()->setDefaultSectionSize(32);
    return t;
}

QTableWidgetItem* cell(const QString& text, const QColor& color = QColor()) {
    auto* item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (color.isValid()) item->setForeground(color);
    return item;
}

const QColor kGreen("#15803d");
const QColor kRed("#b91c1c");

}  // namespace

// =========================================================================
//  Construction
// =========================================================================

MainWindow::MainWindow(UserStore store, QWidget* parent) : QMainWindow(parent), m_store(std::move(store)) {
    setWindowTitle(tr("CodeAlpha Login & Registration System"));
    resize(1000, 680);
    setMinimumSize(900, 620);

    m_stack = new QStackedWidget(this);
    m_auth = new AuthPage(this);
    m_stack->addWidget(m_auth);
    m_stack->addWidget(buildDashboard());
    setCentralWidget(m_stack);
    buildMenus();

    connect(m_auth, &AuthPage::loginRequested, this, &MainWindow::onLogin);
    connect(m_auth, &AuthPage::registerRequested, this, &MainWindow::onRegister);

    statusBar()->showMessage(tr("Data file: %1").arg(QDir::toNativeSeparators(qs(m_store.file()))));
    m_auth->reset();
    if (m_store.users().empty())
        m_auth->showLoginInfo(tr("Welcome! No accounts exist yet. Open the Register tab to create the first "
                                 "one (it becomes the administrator)."));
}

void MainWindow::buildMenus() {
    auto make = [this](const QString& text, auto slot, const QKeySequence& key = QKeySequence()) {
        auto* a = new QAction(text, this);
        if (!key.isEmpty()) a->setShortcut(key);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };
    m_actExportActivity = make(tr("Export &activity log (CSV)…"), [this] { exportActivity(); },
                               QKeySequence(Qt::CTRL | Qt::Key_E));
    m_actExportUsers = make(tr("Export &users list (CSV)…"), [this] { exportUsers(); },
                            QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    m_actChangePw = make(tr("Change &password…"), [this] { changePassword(); });
    m_actLogout = make(tr("&Log out"), [this] { logout(); }, QKeySequence(Qt::CTRL | Qt::Key_L));
    auto* quit = make(tr("E&xit"), [this] { close(); }, QKeySequence::Quit);
    auto* about = make(tr("&About"), [this] { showAbout(); });

    QMenu* file = menuBar()->addMenu(tr("&File"));
    file->addAction(m_actExportActivity);
    file->addAction(m_actExportUsers);
    file->addSeparator();
    file->addAction(m_actChangePw);
    file->addAction(m_actLogout);
    file->addSeparator();
    file->addAction(quit);
    menuBar()->addMenu(tr("&Help"))->addAction(about);

    for (QAction* a : {m_actExportActivity, m_actExportUsers, m_actChangePw, m_actLogout}) a->setEnabled(false);
}

QWidget* MainWindow::buildDashboard() {
    auto* page = new QWidget(this);
    page->setObjectName("dashboard");
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(20, 16, 20, 14);
    root->setSpacing(12);

    // ---- header ----
    auto* header = new QHBoxLayout;
    m_welcome = new QLabel(page);
    m_welcome->setObjectName("welcome");
    m_badge = new QLabel(page);
    m_badge->setObjectName("badge");
    auto* pwBtn = new QPushButton(tr("Change password"), page);
    auto* logoutBtn = new QPushButton(tr("Log out"), page);
    connect(pwBtn, &QPushButton::clicked, this, &MainWindow::changePassword);
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::logout);
    header->addWidget(m_welcome);
    header->addWidget(m_badge);
    header->addStretch(1);
    header->addWidget(pwBtn);
    header->addWidget(logoutBtn);
    root->addLayout(header);

    auto* body = new QHBoxLayout;
    body->setSpacing(16);

    // ---- profile ----
    auto* profileBox = new QGroupBox(tr("My profile"), page);
    profileBox->setFixedWidth(320);
    auto* form = new QFormLayout(profileBox);
    m_pUser = new QLabel(profileBox);
    m_pRole = new QLabel(profileBox);
    m_pCreated = new QLabel(profileBox);
    m_pSince = new QLabel(profileBox);
    m_pEvents = new QLabel(profileBox);
    form->addRow(tr("Username"), m_pUser);
    form->addRow(tr("Role"), m_pRole);
    form->addRow(tr("Account created"), m_pCreated);
    form->addRow(tr("Logged in since"), m_pSince);
    form->addRow(tr("Activity records"), m_pEvents);
    auto* box = new QVBoxLayout;
    box->addWidget(profileBox);
    box->addStretch(1);
    body->addLayout(box);

    // ---- tabs: activity (everyone) + users (administrator only) ----
    m_tabs = new QTabWidget(page);

    auto* activityTab = new QWidget;
    auto* al = new QVBoxLayout(activityTab);
    auto* top = new QHBoxLayout;
    m_activityHint = new QLabel(activityTab);
    m_activityHint->setObjectName("hint");
    top->addWidget(m_activityHint, 1);
    auto* exportBtn = new QPushButton(tr("Export to CSV (Excel)…"), activityTab);
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportActivity);
    top->addWidget(exportBtn);
    al->addLayout(top);
    m_activityTable = makeTable(activityTab, {tr("Time"), tr("User"), tr("Event")});
    m_activityTable->setColumnWidth(0, 170);
    m_activityTable->setColumnWidth(1, 140);
    al->addWidget(m_activityTable);
    m_tabs->addTab(activityTab, tr("Activity log"));

    auto* usersTab = new QWidget;
    auto* ul = new QVBoxLayout(usersTab);
    auto* utop = new QHBoxLayout;
    auto* uhint = new QLabel(tr("All registered users (administrator only). Passwords are never shown or exported."),
                             usersTab);
    uhint->setObjectName("hint");
    utop->addWidget(uhint, 1);
    m_exportUsersBtn = new QPushButton(tr("Export users to CSV (Excel)…"), usersTab);
    connect(m_exportUsersBtn, &QPushButton::clicked, this, &MainWindow::exportUsers);
    utop->addWidget(m_exportUsersBtn);
    ul->addLayout(utop);
    m_usersTable = makeTable(usersTab, {tr("Username"), tr("Role"), tr("Created"), tr("Last login"), tr("Status")});
    m_usersTable->setColumnWidth(0, 140);
    m_usersTable->setColumnWidth(1, 110);
    m_usersTable->setColumnWidth(2, 160);
    m_usersTable->setColumnWidth(3, 160);
    ul->addWidget(m_usersTable);
    m_tabs->addTab(usersTab, tr("Users"));

    body->addWidget(m_tabs, 1);
    root->addLayout(body, 1);
    return page;
}

// =========================================================================
//  Register / login / logout
// =========================================================================

bool MainWindow::persist() {
    std::string err;
    if (m_store.save(err)) return true;
    QMessageBox::critical(this, tr("Could not save your data"),
                          tr("The change was made, but it could not be written to:\n%1\n\n%2\n\n"
                             "Please check the disk space and folder permissions.")
                              .arg(QDir::toNativeSeparators(qs(m_store.file())), qs(err)));
    return false;
}

bool MainWindow::isAdmin() const {
    const User* u = m_store.find(m_current);
    return u && u->role == Role::Admin;
}

void MainWindow::onRegister(const QString& username, const QString& password) {
    std::string err;
    const User* u = m_store.registerUser(username.toStdString(), password.toStdString(), err);
    if (!u) {
        m_auth->showRegisterError(qs(err));
        return;
    }
    const QString name = qs(u->username);
    const bool admin = u->role == Role::Admin;
    persist();
    m_auth->switchToLogin(name);
    m_auth->showLoginInfo(tr("Account \"%1\" created%2. You can log in now.")
                              .arg(name, admin ? tr(" - you are the administrator") : QString()));
    statusBar()->showMessage(tr("Account \"%1\" created.").arg(name));
}

void MainWindow::onLogin(const QString& username, const QString& password) {
    std::string msg;
    const AuthResult r = m_store.login(username.toStdString(), password.toStdString(), msg);
    std::string ignored;
    m_store.save(ignored);   // keep the failed-attempt counter on disk, so a restart can't reset it

    if (r != AuthResult::Ok) {
        m_auth->showLoginError(qs(msg));
        m_auth->clearLoginPassword();
        return;
    }
    std::string name = username.toStdString(), e;
    UserStore::normalizeUsername(name, e);
    enterDashboard(name);
}

void MainWindow::enterDashboard(const std::string& username) {
    m_current = username;
    m_loginTime = std::time(nullptr);
    m_auth->reset();
    m_stack->setCurrentIndex(1);

    const bool admin = isAdmin();
    m_welcome->setText(tr("Welcome, %1").arg(qs(username)));
    m_badge->setText(admin ? tr("Administrator") : tr("User"));
    m_tabs->setTabVisible(1, admin);
    m_tabs->setCurrentIndex(0);
    m_actExportUsers->setEnabled(admin);
    for (QAction* a : {m_actExportActivity, m_actChangePw, m_actLogout}) a->setEnabled(true);
    refreshAll();
    statusBar()->showMessage(tr("Logged in as %1.").arg(qs(username)));
}

void MainWindow::logout() {
    if (m_current.empty()) return;
    m_store.logout(m_current);
    persist();
    m_current.clear();
    m_activityTable->setRowCount(0);
    m_usersTable->setRowCount(0);
    for (QAction* a : {m_actExportActivity, m_actExportUsers, m_actChangePw, m_actLogout}) a->setEnabled(false);
    m_stack->setCurrentIndex(0);
    m_auth->reset();
    statusBar()->showMessage(tr("You have been logged out."));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_current.empty()) {
        m_store.logout(m_current);
        std::string ignored;
        m_store.save(ignored);
    }
    event->accept();
}

void MainWindow::changePassword() {
    ChangePasswordDialog dlg(m_current, this);
    if (dlg.exec() != QDialog::Accepted) return;

    std::string err;
    const bool ok = m_store.changePassword(m_current, dlg.currentPassword(), dlg.newPassword(), err);
    persist();   // wrong guesses are counted, so save either way
    if (!ok) {
        QMessageBox::warning(this, tr("Password not changed"), qs(err));
        const User* u = m_store.find(m_current);
        if (u && u->isLocked(std::time(nullptr))) {   // too many wrong guesses: end the session
            const QString lockMsg = qs(err);
            m_current.clear();
            for (QAction* a : {m_actExportActivity, m_actExportUsers, m_actChangePw, m_actLogout})
                a->setEnabled(false);
            m_stack->setCurrentIndex(0);
            m_auth->reset();
            m_auth->showLoginError(lockMsg);
        }
        return;
    }
    refreshAll();
    QMessageBox::information(this, tr("Password changed"), tr("Your password has been changed."));
}

// =========================================================================
//  Showing data
// =========================================================================

void MainWindow::refreshAll() {
    refreshProfile();
    refreshActivity();
    if (isAdmin()) refreshUsers();
}

void MainWindow::refreshProfile() {
    const User* u = m_store.find(m_current);
    if (!u) return;
    m_pUser->setText(qs(u->username));
    m_pRole->setText(roleName(u->role));
    m_pCreated->setText(dateText(u->createdAt));
    m_pSince->setText(dateText(m_loginTime));
}

void MainWindow::refreshActivity() {
    const bool admin = isAdmin();
    std::vector<Event> shown;
    for (const Event& e : m_store.events())
        if (admin || e.username == m_current) shown.push_back(e);

    m_activityTable->setRowCount(static_cast<int>(shown.size()));
    for (int row = 0; row < static_cast<int>(shown.size()); ++row) {
        const Event& e = shown[shown.size() - 1 - row];   // newest first
        QColor color;
        if (e.type == EventType::LoginFailed || e.type == EventType::Locked) color = kRed;
        else if (e.type == EventType::LoginOk || e.type == EventType::Registered) color = kGreen;
        m_activityTable->setItem(row, 0, cell(dateText(e.time)));
        m_activityTable->setItem(row, 1, cell(qs(e.username)));
        m_activityTable->setItem(row, 2, cell(eventTypeName(e.type), color));
    }
    m_pEvents->setText(QString::number(shown.size()));
    m_activityHint->setText(admin ? tr("Activity of all users, newest first.")
                                  : tr("Your account activity, newest first."));
}

void MainWindow::refreshUsers() {
    const auto users = m_store.users();
    const std::time_t now = std::time(nullptr);
    m_usersTable->setRowCount(static_cast<int>(users.size()));
    for (int row = 0; row < static_cast<int>(users.size()); ++row) {
        const User* u = users[row];
        const bool locked = u->isLocked(now);
        m_usersTable->setItem(row, 0, cell(qs(u->username)));
        m_usersTable->setItem(row, 1, cell(roleName(u->role)));
        m_usersTable->setItem(row, 2, cell(dateText(u->createdAt)));
        m_usersTable->setItem(row, 3, cell(dateText(u->lastLogin)));
        m_usersTable->setItem(row, 4, cell(locked ? tr("Locked") : tr("Active"), locked ? kRed : kGreen));
    }
}

// =========================================================================
//  CSV export
// =========================================================================

void MainWindow::saveCsvWithDialog(const QString& defaultName, const std::string& content) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString path = QFileDialog::getSaveFileName(this, tr("Export to CSV"), dir + "/" + defaultName,
                                                tr("CSV files for Excel (*.csv);;All files (*)"));
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += ".csv";

    std::string err;
    if (!writeCsvFile(path.toStdString(), content, err)) {
        QMessageBox::critical(this, tr("Could not export"),
                              tr("The CSV file could not be written to\n%1\n\n%2")
                                  .arg(QDir::toNativeSeparators(path), qs(err)));
        return;
    }
    statusBar()->showMessage(tr("Exported to %1").arg(QDir::toNativeSeparators(path)));

    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Export complete"));
    box.setText(tr("The file was exported to:\n%1").arg(QDir::toNativeSeparators(path)));
    QPushButton* openBtn = box.addButton(tr("Open file"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Close);
    box.exec();
    if (box.clickedButton() == openBtn) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::exportActivity() {
    if (m_current.empty()) return;
    const bool admin = isAdmin();
    std::vector<Event> rows;   // oldest first, exactly what the table shows
    for (const Event& e : m_store.events())
        if (admin || e.username == m_current) rows.push_back(e);
    saveCsvWithDialog(admin ? "activity_log_all_users.csv" : QString("activity_%1.csv").arg(qs(m_current)),
                      buildEventsCsv(rows));
}

void MainWindow::exportUsers() {
    if (m_current.empty() || !isAdmin()) return;   // administrators only
    saveCsvWithDialog("users.csv", buildUsersCsv(m_store.users(), std::time(nullptr)));
}

void MainWindow::showAbout() {
    QMessageBox::about(
        this, tr("About CodeAlpha Login & Registration System"),
        tr("<h3>CodeAlpha Login &amp; Registration System</h3>"
           "<p>Qt 6 Widgets GUI. Passwords are stored only as salted, repeatedly hashed values. "
           "After %1 wrong passwords an account is locked for %2 minutes.</p>"
           "<p>The first account created becomes the administrator.</p>"
           "<p>Data file:<br><code>%3</code></p>")
            .arg(UserStore::kMaxFailedAttempts)
            .arg(UserStore::kLockSeconds / 60)
            .arg(QDir::toNativeSeparators(qs(m_store.file())).toHtmlEscaped()));
}
