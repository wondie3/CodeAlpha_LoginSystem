#include "Csv.h"

#include "FileUtil.h"

namespace {

std::string timeText(std::time_t t) {
    if (t <= 0) return "";
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

const char* const kBom = "\xEF\xBB\xBF";   // UTF-8 byte order mark: Excel detects the encoding

}  // namespace

std::string csvField(const std::string& text) {
    std::string s = text;
    if (!s.empty() && (s[0] == '=' || s[0] == '+' || s[0] == '-' || s[0] == '@' || s[0] == '\t' || s[0] == '\r'))
        s.insert(s.begin(), '\'');
    if (s.find_first_of(",\"\r\n") == std::string::npos) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

std::string buildUsersCsv(const std::vector<const User*>& users, std::time_t now) {
    std::string csv = kBom;
    csv += "Username,Role,Created,Last Login,Status\r\n";
    for (const User* u : users)
        csv += csvField(u->username) + ',' + roleName(u->role) + ',' + timeText(u->createdAt) + ',' +
               timeText(u->lastLogin) + ',' + (u->isLocked(now) ? "Locked" : "Active") + "\r\n";
    return csv;
}

std::string buildEventsCsv(const std::vector<Event>& events) {
    std::string csv = kBom;
    csv += "Time,Username,Event\r\n";
    for (const Event& e : events)
        csv += timeText(e.time) + ',' + csvField(e.username) + ',' + eventTypeName(e.type) + "\r\n";
    return csv;
}

bool writeCsvFile(const std::string& path, const std::string& content, std::string& error) {
    return writeFileAtomically(path, content, error);
}
