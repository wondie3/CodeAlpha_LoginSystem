// CSV export that opens cleanly in Microsoft Excel.
#pragma once

#include <string>
#include <vector>

#include "UserStore.h"

// One CSV cell: quoted when needed, and neutralised against "formula injection"
// (Excel runs cells that start with = + - @, so such text gets a leading apostrophe).
std::string csvField(const std::string& text);

// Complete file content: UTF-8 BOM + header + rows, CRLF line endings.
std::string buildUsersCsv(const std::vector<const User*>& users, std::time_t now);
std::string buildEventsCsv(const std::vector<Event>& events);   // oldest first as given

bool writeCsvFile(const std::string& path, const std::string& content, std::string& error);
