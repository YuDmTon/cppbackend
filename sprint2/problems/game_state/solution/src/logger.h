#pragma once

#include <boost/json.hpp>

#include <string>

namespace logger {

void LogInit();
void LogStart   (std::string address, unsigned port);
void LogStop    ();
void LogStop    (int code, std::string what);
void LogRequest (std::string ip, std::string_view uri, std::string method);
void LogResponse(int response_time, unsigned code, std::string_view content_type);
void LogNetError(int code, std::string text, std::string_view where);
void LogJson    (std::string_view message, boost::json::object data);

}  // namespace logger
