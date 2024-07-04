#pragma once

#include <boost/json.hpp>

#include <filesystem>
#include <fstream>

#include <stdexcept>
#include <string>

#include "model.h"

namespace json_loader {

/////////////////////
model::Game LoadGame(const std::filesystem::path& json_path);

////////////////////
std::string NotFound();
std::string BadRequest();
std::string GetIdList(const std::string& json_str);
std::string GetMap(const std::string& json_str, const std::string& id);


}  // namespace json_loader
