#pragma once

#include <boost/json.hpp>

#include <filesystem>
#include <fstream>

#include <stdexcept>
#include <string>

#include "model.h"

namespace json_loader {

//// game ///////////
model::Game LoadGame(const std::filesystem::path& config);

}  // namespace json_loader
