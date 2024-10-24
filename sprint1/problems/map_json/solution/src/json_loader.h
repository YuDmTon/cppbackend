#pragma once

#include <boost/json.hpp>

#include <filesystem>
#include <fstream>

#include "model.h"

namespace json_loader {

/////////////////////
model::Game LoadGame(const std::filesystem::path& json_path);

////////////////////
std::string NotFound();
std::string BadRequest();
std::string GetIdList(std::string json_str);
std::string GetMap(std::string json_str, std::string id);


}  // namespace json_loader
