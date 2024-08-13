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
    constexpr const int    MILLISECONDS      = 1000;
    //
    std::string json_config = ReadFile(config_file);
    try {
        json::object config = json::parse(json_config).as_object();

        // try get defaultDogSpeed
        double default_dog_speed = DEFAULT_DOG_SPEED;
        try {
            default_dog_speed = config.at("defaultDogSpeed").as_double();
        } catch (...) { }

        // try get loot generator config
        double period;
        double probability;
        try {
            period      = config.at("lootGeneratorConfig").as_object().at("period").as_double();
            probability = config.at("lootGeneratorConfig").as_object().at("probability").as_double();
        } catch (...) {
            std::string err = "Can't get loot generator config";
            throw std::runtime_error(err);
        }

        // create game
        model::Game game(static_cast<unsigned>(period * MILLISECONDS), probability);

        // try get maps
        for (auto& json_map : config.at("maps").as_array()) {
            // map
            model::Map map = model::Map::FromJson(json_map.as_object(), default_dog_speed);

            // loot
            for (const auto& json_loot_type : json_map.as_object().at("lootTypes").as_array()) {
                map.AddLootType(model::LootType::FromJson(json_loot_type.as_object()));
            }
            
            // roads
            for (const auto& json_road : json_map.as_object().at("roads").as_array()) {
                map.AddRoad(model::Road::FromJson(json_road.as_object()));
            }

            // buildings
            for (const auto& json_building : json_map.as_object().at("buildings").as_array()) {
                map.AddBuilding(model::Building::FromJson(json_building.as_object()));
            }

            // offices
            for (const auto& json_office : json_map.as_object().at("offices").as_array()) {
                map.AddOffice(model::Office::FromJson(json_office.as_object()));
            }

            game.AddMap(map);
        }

        return game;
    } catch (...) {
        throw std::runtime_error("Wrong config json");
    }
}

}  // namespace json_loader
