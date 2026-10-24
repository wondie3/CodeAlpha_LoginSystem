// User accounts, password verification, lockout, activity log and safe file storage.
// No GUI code here, so everything can be unit-tested on its own.
#pragma once

#include <ctime>
#include <map>
#include <string>
#include <vector>

enum class Role { User, Admin };
enum class AuthResult { Ok, BadCredentials, Locked };
enum class EventType { Registered, LoginOk, LoginFailed, Locked, PasswordChanged, Logout };

const char* eventTypeName(EventType t);   // text for people:  "Login failed"
const char* eventTypeKey(EventType t);    // text for the data file: "LOGIN_FAILED"
bool parseEventType(const std::string& s, EventType& out);
const char* roleName(Role r);

// Only a random salt and a salted hash of the password are stored, never the password.
struct User {
    std::string username;        // always lower-case
    std::string salt;            // 32 hex chars
    std::string hash;            // 64 hex chars
    Role role = Role::User;
    std::time_t createdAt = 0;
    std::time_t lastLogin = 0;   // 0 = never
    int failedAttempts = 0;
    std::time_t lockedUntil = 0;

    bool isLocked(std::time_t now) const { return lockedUntil > now; }
};

struct Event {
    std::time_t time = 0;
    std::string username;
    EventType type = EventType::LoginOk;
};

class UserStore {
public:
    static constexpr int kMaxFailedAttempts = 5;   // wrong passwords before a lockout
    static constexpr int kLockSeconds = 300;       // lockout length (5 minutes)
    static constexpr std::size_t kMaxEvents = 1000;

    explicit UserStore(std::string file) : file_(std::move(file)) {}

    // ---- persistence ----
    // True if loaded, or if there is no file yet. On failure the store stays empty
    // and the file is left untouched.
    bool load(std::string& error);
    bool save(std::string& error) const;   // atomic (temp file + rename)
    const std::string& file() const { return file_; }

    // ---- input rules (also used by the GUI) ----
    // Trims, lower-cases and validates a username (3-20 letters, digits or underscores).
    static bool normalizeUsername(std::string& username, std::string& error);
    static bool validatePassword(const std::string& password, const std::string& username, std::string& error);
    // 0 (empty) .. 4 (strong); only for the strength meter.
    static int passwordScore(const std::string& password);

    // ---- accounts ----
    // The very first account becomes the administrator.
    const User* registerUser(std::string username, const std::string& password, std::string& error);
    // The message is safe to show: it is identical for "no such user" and "wrong password".
    AuthResult login(std::string username, const std::string& password, std::string& message);
    void logout(const std::string& username);
    bool changePassword(const std::string& username, const std::string& current,
                        const std::string& next, std::string& error);

    // ---- queries ----
    const User* find(const std::string& username) const;
    std::vector<const User*> users() const;
    const std::vector<Event>& events() const { return events_; }   // oldest first

private:
    bool loadFrom(std::istream& in, std::string& error);
    void addEvent(const std::string& username, EventType type);

    std::string file_;
    std::map<std::string, User> users_;
    std::vector<Event> events_;
};
