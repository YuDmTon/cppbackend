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

model::Game LoadGame(const fs::path& config_file) {
    constexpr const double DEFAULT_DOG_SPEED = 1.0;
    //
    model::Game game;
    // read & parse
    json::object config = json::parse(ReadFile(config_file)).as_object();
    // try get defaultDogSpeed
    double default_dog_speed = DEFAULT_DOG_SPEED;
    try {
        default_dog_speed = config.at("defaultDogSpeed").as_double();
    } catch (...) { }
    // try get maps
    try {
        for (auto& json_map : config.at("maps").as_array()) {
            // map
            std::string id   = json::value_to< std::string >(json_map.as_object().at("id"));
            std::string name = json::value_to< std::string >(json_map.as_object().at("name"));
            double dog_speed = default_dog_speed;
            try {
                dog_speed = json_map.at("dogSpeed").as_double();
            } catch (...) { }
            model::Map map{model::Map::Id(id), name, dog_speed};
            // roads
            json::array roads = json_map.as_object().at("roads").as_array();
            for (const auto& road : roads) {
                int x0 = json::value_to< int >(road.as_object().at("x0"));
                int y0 = json::value_to< int >(road.as_object().at("y0"));
                try {
                    int x1 = json::value_to< int >(road.as_object().at("x1"));
                    model::Road road(model::Road::HORIZONTAL, {x0, y0}, x1);
                    map.AddRoad(road);
                } catch (...) { }
                try {
                    int y1 = json::value_to< int >(road.as_object().at("y1"));
                    model::Road road(model::Road::VERTICAL, {x0, y0}, y1);
                    map.AddRoad(road);
                } catch (...) { }
            }
            // buildings
            json::array buildings = json_map.as_object().at("buildings").as_array();
            for (const auto& building : buildings) {
                int x = json::value_to< int >(building.as_object().at("x"));
                int y = json::value_to< int >(building.as_object().at("y"));
                int w = json::value_to< int >(building.as_object().at("w"));
                int h = json::value_to< int >(building.as_object().at("h"));
                model::Building build(model::Rectangle({x, y}, {w, h}));
                map.AddBuilding(build);
            }
            // offices
            json::array offices = json_map.as_object().at("offices").as_array();
            for (const auto& office : offices) {
                std::string id = json::value_to< std::string >(office.as_object().at("id"));
                int x = json::value_to< int >(office.as_object().at("x"));
                int y = json::value_to< int >(office.as_object().at("y"));
                int offset_x = json::value_to< int >(office.as_object().at("offsetX"));
                int offset_y = json::value_to< int >(office.as_object().at("offsetY"));
                model::Office offc(model::Office::Id(id), {x, y}, {offset_x, offset_y});
                map.AddOffice(offc);
            }
            game.AddMap(map);
        }
    } catch (...) {
        throw std::runtime_error("Wrong config json");
    }

    return game;
}

}  // namespace json_loader
