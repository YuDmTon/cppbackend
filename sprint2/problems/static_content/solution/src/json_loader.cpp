#include <iostream>

#include "json_loader.h"

namespace json_loader {

using namespace std::literals;

namespace json = boost::json;
namespace fs   = std::filesystem;

///////////////////////////////////////////
std::string ReadFile(const fs::path& path) {
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if ( !f ) {
        std::string err = "Can't open config file '" + path.string() + "'";
        throw std::runtime_error(err);
    }
    const auto sz = fs::file_size(path);
    std::string result(sz, '\0');
    f.read(result.data(), sz);
    return result;
}

model::Game LoadGame(const fs::path& json_path, const fs::path& root_dir) {
    model::Game game;
    game.SetJsonStr(ReadFile(json_path));
    //
    fs::path server_root = fs::current_path();
    server_root += "/";
    std::string root_str = root_dir.string();
    if ( root_str.size() != 0 && root_str[root_str.size() - 1] == '/' ) {
        root_str = root_str.substr(0, root_str.size() - 1);
    }
    server_root += root_str;
    server_root = fs::weakly_canonical(server_root);
//std::cout << "Server root = '" << server_root.string() << "'" << std::endl;
    if ( !fs::is_directory(server_root) ) {
        std::string err = "Directory '" + server_root.string() + "' doesn't exist";
        throw std::runtime_error(err);
    }
    game.SetRootDir(server_root.string());
    return game;
}

/////////////////////////////////////////
std::string NotFound() {
    json::object result;
    result["code"] = "mapNotFound"s;
    result["name"] = "Map not found"s;
    return json::serialize(result);
}

std::string BadRequest() {
    json::object result;
    result["code"] = "badRequest"s;
    result["name"] = "Bad request"s;
    return json::serialize(result);
}

std::string GetIdList(const std::string& json_str) {
    auto maps = json::parse(json_str);
    json::array result;
    for (const auto& map : maps.as_object().at("maps").as_array()) {
        json::object id;
        id["id"]   = map.as_object().at("id");
        id["name"] = map.as_object().at("name");
        result.push_back(id);
    }
    return json::serialize(result);
}

std::string GetMap(const std::string& json_str, const std::string& id) {
    auto maps  = json::parse(json_str).as_object().at("maps").as_array();
    auto is_id = [id](auto map) { return id == map.as_object().at("id").as_string(); };
    if (auto it = std::find_if(maps.begin(), maps.end(), is_id); it != maps.end()) {
        return json::serialize(*it);
    }
    return ""s;
}

}  // namespace json_loader
