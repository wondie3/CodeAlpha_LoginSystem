// Self-checking tests for the login/registration core (no Qt, no framework needed).
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Crypto.h"
#include "Csv.h"
#include "UserStore.h"

static int failures = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) { ++failures; std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; } \
    } while (0)

static std::string tempFile(const char* name) {
    return (std::filesystem::temp_directory_path() / name).string();
}
static std::string readAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main() {
    std::string err, msg;

    // ---- SHA-256 known answers ----
    CHECK(crypto::toHex(crypto::sha256("abc")) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(crypto::toHex(crypto::sha256("")) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    // ---- username rules ----
    for (const char* bad : {"ab", "a very long username here", "bad name", "bad-name", "na!me", ""}) {
        std::string u = bad;
        CHECK(!UserStore::normalizeUsername(u, err));
    }
    std::string u = "  Alice_01 ";
    CHECK(UserStore::normalizeUsername(u, err) && u == "alice_01");

    // ---- password rules ----
    CHECK(!UserStore::validatePassword("Short1", "bob", err));
    CHECK(!UserStore::validatePassword("alllowercase1", "bob", err));
    CHECK(!UserStore::validatePassword("ALLUPPERCASE1", "bob", err));
    CHECK(!UserStore::validatePassword("NoDigitsHere", "bob", err));
    CHECK(!UserStore::validatePassword("Has Space1x", "bob", err));
    CHECK(!UserStore::validatePassword("Bob12345", "bob12345", err));         // same as username
    CHECK(UserStore::validatePassword("Str0ngPassw0rd", "bob", err));
    CHECK(UserStore::passwordScore("") == 0);
    CHECK(UserStore::passwordScore("abc") == 1);
    CHECK(UserStore::passwordScore("Str0ngPassw0rd!") >= 3);
    CHECK(UserStore::passwordScore("Str0ngPassw0rd!") > UserStore::passwordScore("Passw0rd"));

    // ---- registration ----
    const std::string path = tempFile("login_gui_test_a.db");
    std::filesystem::remove(path);
    UserStore store(path);
    const User* alice = store.registerUser("Alice", "Passw0rdX", err);
    CHECK(alice && alice->username == "alice" && alice->role == Role::Admin);       // first user = admin
    CHECK(alice->hash.size() == 64 && alice->salt.size() == 32 && alice->hash != "Passw0rdX");
    CHECK(store.registerUser("ALICE", "Passw0rdX", err) == nullptr);                // duplicate, any case
    CHECK(err.find("taken") != std::string::npos);
    CHECK(store.registerUser("bob", "weak", err) == nullptr);
    const User* bob = store.registerUser("bob", "Secr3tPass", err);
    CHECK(bob && bob->role == Role::User);
    CHECK(store.users().size() == 2);
    CHECK(store.find("alice")->salt != store.find("bob")->salt);

    // same password, different salt => different hash
    store.registerUser("carol", "Secr3tPass", err);
    CHECK(store.find("carol")->hash != store.find("bob")->hash);

    // ---- login ----
    CHECK(store.login("alice", "Passw0rdX", msg) == AuthResult::Ok);
    CHECK(store.login("  ALICE ", "Passw0rdX", msg) == AuthResult::Ok);            // case-insensitive name
    CHECK(store.find("alice")->lastLogin > 0);
    CHECK(store.login("alice", "wrong", msg) == AuthResult::BadCredentials);
    const std::string badMsg = msg;
    CHECK(store.login("nobody", "Passw0rdX", msg) == AuthResult::BadCredentials && msg == badMsg);
    CHECK(store.login("bad name!", "x", msg) == AuthResult::BadCredentials && msg == badMsg);

    // ---- lockout after 5 wrong passwords, persisted ----
    for (int i = 0; i < UserStore::kMaxFailedAttempts - 1; ++i)
        CHECK(store.login("bob", "nope", msg) == AuthResult::BadCredentials);
    CHECK(store.login("bob", "nope", msg) == AuthResult::Locked);
    CHECK(store.login("bob", "Secr3tPass", msg) == AuthResult::Locked);             // even the right one
    CHECK(msg.find("minute") != std::string::npos);
    CHECK(store.login("alice", "Passw0rdX", msg) == AuthResult::Ok);                // others unaffected

    // ---- change password ----
    CHECK(!store.changePassword("alice", "wrong", "NewPassw0rd1", err));
    CHECK(!store.changePassword("alice", "Passw0rdX", "weak", err));
    CHECK(!store.changePassword("alice", "Passw0rdX", "Passw0rdX", err));
    CHECK(store.changePassword("alice", "Passw0rdX", "NewPassw0rd1", err));
    CHECK(store.login("alice", "Passw0rdX", msg) == AuthResult::BadCredentials);
    CHECK(store.login("alice", "NewPassw0rd1", msg) == AuthResult::Ok);
    store.logout("alice");

    // ---- activity log ----
    int ok = 0, failed = 0, locked = 0, changed = 0, out = 0, reg = 0;
    for (const Event& e : store.events()) {
        ok += e.type == EventType::LoginOk; failed += e.type == EventType::LoginFailed;
        locked += e.type == EventType::Locked; changed += e.type == EventType::PasswordChanged;
        out += e.type == EventType::Logout; reg += e.type == EventType::Registered;
    }
    CHECK(reg == 3 && locked == 1 && changed == 1 && out == 1 && ok >= 3 && failed >= 7);

    // ---- save / load round trip; the password is never written ----
    CHECK(store.save(err));
    const std::string content = readAll(path);
    CHECK(content.rfind("LOGINDATA 1\n", 0) == 0);
    CHECK(content.find("Passw0rd") == std::string::npos && content.find("Secr3t") == std::string::npos);
    UserStore loaded(path);
    CHECK(loaded.load(err));
    CHECK(loaded.users().size() == 3 && loaded.events().size() == store.events().size());
    CHECK(loaded.login("alice", "NewPassw0rd1", msg) == AuthResult::Ok);
    CHECK(loaded.login("bob", "Secr3tPass", msg) == AuthResult::Locked);            // lockout survives restart
    CHECK(loaded.registerUser("dave", "Dav3Passw0rd", err)->role == Role::User);

    // ---- event log is capped ----
    {
        UserStore big(tempFile("login_gui_test_big.db"));
        big.registerUser("zed", "Zed12345x", err);
        for (std::size_t i = 0; i < UserStore::kMaxEvents + 50; ++i) big.logout("zed");
        CHECK(big.events().size() == UserStore::kMaxEvents);
        CHECK(big.events().back().type == EventType::Logout);                          // newest kept
        // guessing names that do not exist adds nothing to the log
        const std::size_t before = big.events().size();
        for (int i = 0; i < 20; ++i) big.login("ghost", "whatever1A", msg);
        CHECK(big.events().size() == before);
    }

    // ---- corrupt / tampered files are rejected, and never replace the loaded state ----
    auto expectReject = [&](const std::string& body, const char* label) {
        const std::string p = tempFile("login_gui_test_b.db");
        { std::ofstream o(p); o << body; }
        UserStore s(p);
        std::string e;
        if (s.load(e) || e.empty() || !s.users().empty()) { ++failures; std::cout << "FAIL: not rejected: " << label << "\n"; }
    };
    expectReject("garbage\n", "bad header");
    expectReject("", "empty file");
    expectReject(content + "NONSENSE\n", "unknown record");
    expectReject(content + "EVENT|1|alice|EXPLODED\n", "unknown event type");
    expectReject(content + "EVENT|abc|alice|LOGIN_OK\n", "bad event time");
    {
        std::string dup = content;
        dup += dup.substr(dup.find("USER|"), dup.find('\n', dup.find("USER|")) - dup.find("USER|") + 1);
        expectReject(dup, "duplicate user");
    }
    {
        std::string t = content;
        t.replace(t.find("USER|alice|") + 11 + 32 + 1, 4, "zzzz");                    // corrupt the salt hex
        expectReject(t, "invalid salt");
    }
    {
        std::string t = content;
        t.replace(t.find("|USER|"), 6, "|ROOT|");
        expectReject(t, "unknown role");
    }
    {   // a bad file must not replace a store that already holds data
        const std::string p = tempFile("login_gui_test_b.db");
        { std::ofstream o(p); o << "garbage\n"; }
        UserStore keep(p);
        std::string e;
        keep.registerUser("keeper", "K33pPassw0rd", e);
        CHECK(!keep.load(e) && keep.find("keeper") != nullptr);
    }
    UserStore fresh(tempFile("login_gui_does_not_exist.db"));
    std::filesystem::remove(tempFile("login_gui_does_not_exist.db"));
    CHECK(fresh.load(err) && fresh.users().empty());                                  // no file = fresh start

    // ---- CSV export ----
    CHECK(csvField("plain") == "plain");
    CHECK(csvField("a,b") == "\"a,b\"");
    CHECK(csvField("say \"hi\"") == "\"say \"\"hi\"\"\"");
    CHECK(csvField("=1+1") == "'=1+1");
    CHECK(csvField("@SUM(A1)") == "'@SUM(A1)");
    const std::string usersCsv = buildUsersCsv(loaded.users(), std::time(nullptr));
    CHECK(usersCsv.rfind("\xEF\xBB\xBF" "Username,Role,Created,Last Login,Status\r\n", 0) == 0);
    CHECK(usersCsv.find("alice,Administrator,") != std::string::npos);
    CHECK(usersCsv.find("bob,User,") != std::string::npos && usersCsv.find(",Locked\r\n") != std::string::npos);
    CHECK(usersCsv.find("Passw0rd") == std::string::npos && usersCsv.find(alice->hash) == std::string::npos);
    const std::string eventsCsv = buildEventsCsv(loaded.events());
    CHECK(eventsCsv.rfind("\xEF\xBB\xBF" "Time,Username,Event\r\n", 0) == 0);
    CHECK(eventsCsv.find(",alice,Login succeeded\r\n") != std::string::npos);
    CHECK(eventsCsv.find(",bob,Account locked\r\n") != std::string::npos);
    const std::string csvPath = tempFile("login_gui_test.csv");
    CHECK(writeCsvFile(csvPath, usersCsv, err) && readAll(csvPath) == usersCsv);
    CHECK(!writeCsvFile("/nonexistent/dir/x.csv", usersCsv, err) && !err.empty());

    for (const char* n : {"login_gui_test_a.db", "login_gui_test_b.db", "login_gui_test_big.db", "login_gui_test.csv"})
        std::filesystem::remove(tempFile(n));
    std::cout << (failures == 0 ? "All tests passed.\n" : "Some checks FAILED.\n");
    return failures == 0 ? 0 : 1;
}
