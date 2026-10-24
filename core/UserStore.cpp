#include "UserStore.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "Crypto.h"
#include "FileUtil.h"

namespace {

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : s) {
        if (c == '|') { parts.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    parts.push_back(cur);
    return parts;
}

bool toLL(const std::string& s, long long& out) {
    if (s.empty()) return false;
    try {
        std::size_t pos = 0;
        out = std::stoll(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

std::string lockedMessage(long long seconds) {
    const long long minutes = std::max<long long>(1, (seconds + 59) / 60);
    return "Too many failed attempts. Try again in " + std::to_string(minutes) +
           (minutes == 1 ? " minute." : " minutes.");
}

const char* const kBadLogin = "Incorrect username or password.";

bool hasClass(const std::string& s, int (*pred)(int)) {
    return std::any_of(s.begin(), s.end(), [&](unsigned char c) { return pred(c) != 0; });
}
int isSymbol(int c) { return std::isgraph(c) && !std::isalnum(c); }

}  // namespace

// =========================================================================
//  Names
// =========================================================================

const char* eventTypeName(EventType t) {
    switch (t) {
        case EventType::Registered: return "Account created";
        case EventType::LoginOk: return "Login succeeded";
        case EventType::LoginFailed: return "Login failed";
        case EventType::Locked: return "Account locked";
        case EventType::PasswordChanged: return "Password changed";
        case EventType::Logout: return "Logged out";
    }
    return "?";
}

const char* eventTypeKey(EventType t) {
    switch (t) {
        case EventType::Registered: return "REGISTERED";
        case EventType::LoginOk: return "LOGIN_OK";
        case EventType::LoginFailed: return "LOGIN_FAILED";
        case EventType::Locked: return "LOCKED";
        case EventType::PasswordChanged: return "PASSWORD_CHANGED";
        case EventType::Logout: return "LOGOUT";
    }
    return "?";
}

bool parseEventType(const std::string& s, EventType& out) {
    for (EventType t : {EventType::Registered, EventType::LoginOk, EventType::LoginFailed, EventType::Locked,
                        EventType::PasswordChanged, EventType::Logout})
        if (s == eventTypeKey(t)) { out = t; return true; }
    return false;
}

const char* roleName(Role r) { return r == Role::Admin ? "Administrator" : "User"; }

// =========================================================================
//  Input rules
// =========================================================================

bool UserStore::normalizeUsername(std::string& username, std::string& error) {
    username = lower(trim(username));
    if (username.size() < 3 || username.size() > 20) {
        error = "Username must be 3-20 characters long.";
        return false;
    }
    for (unsigned char c : username) {
        if (!std::isalnum(c) && c != '_') {
            error = "Username may only contain letters, digits and underscores.";
            return false;
        }
    }
    return true;
}

bool UserStore::validatePassword(const std::string& pw, const std::string& username, std::string& error) {
    if (pw.size() < 8 || pw.size() > 64) {
        error = "Password must be 8-64 characters long.";
        return false;
    }
    for (unsigned char c : pw) {
        if (std::isspace(c)) {
            error = "Password must not contain spaces.";
            return false;
        }
    }
    if (!hasClass(pw, std::isupper) || !hasClass(pw, std::islower) || !hasClass(pw, std::isdigit)) {
        error = "Password needs an uppercase letter, a lowercase letter and a digit.";
        return false;
    }
    if (lower(pw) == lower(username)) {
        error = "Password must not be the same as the username.";
        return false;
    }
    return true;
}

int UserStore::passwordScore(const std::string& pw) {
    if (pw.empty()) return 0;
    const int classes = hasClass(pw, std::islower) + hasClass(pw, std::isupper) +
                        hasClass(pw, std::isdigit) + hasClass(pw, isSymbol);
    int score = 0;
    if (pw.size() >= 8) ++score;
    if (pw.size() >= 12) ++score;
    if (classes >= 3) ++score;
    if (classes == 4 || pw.size() >= 16) ++score;
    return std::max(score, 1);   // anything typed is at least "very weak" on the meter
}

// =========================================================================
//  Accounts
// =========================================================================

void UserStore::addEvent(const std::string& username, EventType type) {
    events_.push_back({std::time(nullptr), username, type});
    if (events_.size() > kMaxEvents) events_.erase(events_.begin(), events_.begin() + (events_.size() - kMaxEvents));
}

const User* UserStore::registerUser(std::string username, const std::string& password, std::string& error) {
    if (!normalizeUsername(username, error)) return nullptr;
    if (users_.count(username)) {
        error = "That username is already taken. Please choose another.";
        return nullptr;
    }
    if (!validatePassword(password, username, error)) return nullptr;

    User u;
    u.username = username;
    u.salt = crypto::randomSalt();
    u.hash = crypto::hashSecret(password, u.salt);
    u.role = users_.empty() ? Role::Admin : Role::User;
    u.createdAt = std::time(nullptr);
    auto it = users_.emplace(username, std::move(u)).first;
    addEvent(username, EventType::Registered);
    return &it->second;
}

AuthResult UserStore::login(std::string username, const std::string& password, std::string& message) {
    std::string err;
    const bool validName = normalizeUsername(username, err);
    auto it = validName ? users_.find(username) : users_.end();

    if (it == users_.end()) {
        // Same amount of work as for a real user, so timing reveals nothing.
        // (Not logged: otherwise anyone could flood the activity log with made-up names.)
        crypto::hashSecret(password, "00000000000000000000000000000000");
        message = kBadLogin;
        return AuthResult::BadCredentials;
    }

    User& u = it->second;
    const std::time_t now = std::time(nullptr);
    if (u.isLocked(now)) {
        message = lockedMessage(u.lockedUntil - now);
        return AuthResult::Locked;
    }

    if (crypto::constantTimeEquals(crypto::hashSecret(password, u.salt), u.hash)) {
        u.failedAttempts = 0;
        u.lockedUntil = 0;
        u.lastLogin = now;
        addEvent(u.username, EventType::LoginOk);
        return AuthResult::Ok;
    }

    addEvent(u.username, EventType::LoginFailed);
    if (++u.failedAttempts >= kMaxFailedAttempts) {
        u.failedAttempts = 0;
        u.lockedUntil = now + kLockSeconds;
        addEvent(u.username, EventType::Locked);
        message = lockedMessage(kLockSeconds);
        return AuthResult::Locked;
    }
    message = kBadLogin;
    return AuthResult::BadCredentials;
}

void UserStore::logout(const std::string& username) {
    if (users_.count(username)) addEvent(username, EventType::Logout);
}

bool UserStore::changePassword(const std::string& username, const std::string& current,
                               const std::string& next, std::string& error) {
    auto it = users_.find(username);
    if (it == users_.end()) { error = "User not found."; return false; }
    User& u = it->second;

    const std::time_t now = std::time(nullptr);
    if (u.isLocked(now)) { error = lockedMessage(u.lockedUntil - now); return false; }
    if (!crypto::constantTimeEquals(crypto::hashSecret(current, u.salt), u.hash)) {
        addEvent(username, EventType::LoginFailed);   // guessing here counts like a failed login
        if (++u.failedAttempts >= kMaxFailedAttempts) {
            u.failedAttempts = 0;
            u.lockedUntil = now + kLockSeconds;
            addEvent(username, EventType::Locked);
            error = lockedMessage(kLockSeconds);
        } else {
            error = "The current password is incorrect.";
        }
        return false;
    }
    if (!validatePassword(next, username, error)) return false;
    if (next == current) {
        error = "The new password must be different from the current one.";
        return false;
    }
    u.salt = crypto::randomSalt();
    u.hash = crypto::hashSecret(next, u.salt);
    u.failedAttempts = 0;
    addEvent(username, EventType::PasswordChanged);
    return true;
}

const User* UserStore::find(const std::string& username) const {
    auto it = users_.find(username);
    return it == users_.end() ? nullptr : &it->second;
}

std::vector<const User*> UserStore::users() const {
    std::vector<const User*> out;
    for (const auto& [name, u] : users_) out.push_back(&u);
    return out;
}

// =========================================================================
//  Persistence
//
//  LOGINDATA 1
//  USER|username|salt|hash|USER or ADMIN|createdAt|lastLogin|failedAttempts|lockedUntil
//  EVENT|time|username|TYPE
// =========================================================================

bool UserStore::save(std::string& error) const {
    std::ostringstream out;
    out << "LOGINDATA 1\n";
    for (const auto& [name, u] : users_)
        out << "USER|" << u.username << '|' << u.salt << '|' << u.hash << '|'
            << (u.role == Role::Admin ? "ADMIN" : "USER") << '|' << static_cast<long long>(u.createdAt) << '|'
            << static_cast<long long>(u.lastLogin) << '|' << u.failedAttempts << '|'
            << static_cast<long long>(u.lockedUntil) << '\n';
    for (const Event& e : events_)
        out << "EVENT|" << static_cast<long long>(e.time) << '|' << e.username << '|' << eventTypeKey(e.type) << '\n';
    return writeFileAtomically(file_, out.str(), error);
}

bool UserStore::load(std::string& error) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(file_, ec)) return true;   // first run
    if (fs::file_size(file_, ec) > 20ULL * 1024 * 1024) {
        error = "The data file is unexpectedly large.";
        return false;
    }
    std::ifstream in(file_);
    if (!in) {
        error = "Cannot open " + file_;
        return false;
    }
    UserStore fresh(file_);
    if (!fresh.loadFrom(in, error)) return false;
    *this = std::move(fresh);   // replace our state only after everything checked out
    return true;
}

bool UserStore::loadFrom(std::istream& in, std::string& error) {
    auto fail = [&](int line, const std::string& msg) {
        error = "Line " + std::to_string(line) + ": " + msg;
        return false;
    };

    std::string line;
    int lineNo = 1;
    if (!std::getline(in, line) || trim(line) != "LOGINDATA 1") {
        error = "Not a valid user data file (bad header).";
        return false;
    }

    while (std::getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty()) continue;
        const auto f = split(line);

        if (f[0] == "USER" && f.size() == 9) {
            User u;
            std::string name = f[1], err;
            long long created = 0, last = 0, failed = 0, locked = 0;
            if (!normalizeUsername(name, err) || name != f[1]) return fail(lineNo, "invalid username.");
            if (users_.count(name)) return fail(lineNo, "duplicate username.");
            if (!crypto::isHex(f[2], 32) || !crypto::isHex(f[3], 64)) return fail(lineNo, "invalid password data.");
            if (f[4] != "USER" && f[4] != "ADMIN") return fail(lineNo, "invalid role.");
            if (!toLL(f[5], created) || !toLL(f[6], last) || !toLL(f[7], failed) || !toLL(f[8], locked) ||
                created < 0 || last < 0 || failed < 0 || failed > 1000 || locked < 0)
                return fail(lineNo, "bad number in USER record.");
            u.username = name;
            u.salt = f[2];
            u.hash = f[3];
            u.role = f[4] == "ADMIN" ? Role::Admin : Role::User;
            u.createdAt = static_cast<std::time_t>(created);
            u.lastLogin = static_cast<std::time_t>(last);
            u.failedAttempts = static_cast<int>(failed);
            u.lockedUntil = static_cast<std::time_t>(locked);
            users_.emplace(name, std::move(u));

        } else if (f[0] == "EVENT" && f.size() == 4) {
            long long t = 0;
            std::string name = f[2], err;
            EventType type;
            if (!toLL(f[1], t) || t < 0 || !normalizeUsername(name, err) || name != f[2] || !parseEventType(f[3], type))
                return fail(lineNo, "bad EVENT record.");
            events_.push_back({static_cast<std::time_t>(t), name, type});

        } else {
            return fail(lineNo, "unrecognised or malformed record.");
        }
    }
    if (events_.size() > kMaxEvents) events_.erase(events_.begin(), events_.begin() + (events_.size() - kMaxEvents));
    return true;
}
