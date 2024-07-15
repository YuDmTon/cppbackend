#pragma once

#include <boost/json.hpp>

#include <filesystem>
#include <fstream>

#include <stdexcept>
#include <string>

#include "model.h"

namespace json_loader {

/////////////////////
model::Game LoadGame(const std::filesystem::path& json_path, const std::filesystem::path& root_dir);

//// http //////////
std::string NotFound();
std::string BadRequest();
std::string GetIdList(const std::string& json_str);
std::string GetMap(const std::string& json_str, const std::string& id);

//// Logs /////////
void LogInit();
void LogStart(std::string address, unsigned port);
void LogStop();
void LogStop(int code, std::string what);
void LogRequest(std::string ip, std::string_view uri, std::string method);
void LogResponse(int response_time, unsigned code, std::string_view content_type);
void LogNetError(int code, std::string text, std::string_view where);
void LogJson(std::string_view message, boost::json::object data);

}  // namespace json_loader
