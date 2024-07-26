#include <boost/json.hpp>

#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

namespace json = boost::json;

//// Aux ///////////////////////////////////////////////////////////////////////////
std::string DirToStr(Direction dir) {
    switch ( dir ) {
        case NORTH: return "U"s;
        case SOUTH: return "D"s;
        case WEST:  return "L"s;
        case EAST:  return "R"s;
        case STOP:  return ""s;
    }
    return "?"s;
}

bool StrToDir(std::string str, Direction& dir) {
    if ( str.empty() ) {
        dir = STOP;
        return true;
    } if ( str == "U" ) {
        dir = NORTH;
        return true;
    } if ( str == "D" ) {
        dir = SOUTH;
        return true;
    } if ( str == "L" ) {
        dir = WEST;
        return true;
    } if ( str == "R" ) {
        dir = EAST;
        return true;
    }
    //
    return false;
}

double GetRandom(double from, double to) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(from, to);
    return dis(gen);
}


//// Map /////////////////////////////////////////////////////////////////////////
std::string Map::Serialize() const {
    //
    json::object map;
    map["id"]   = *id_;
    map["name"] = name_;
    // roads
    json::array json_roads;
    for (const auto& road : roads_) {
        json::object json_road;
        Point start = road.GetStart();
        Point end   = road.GetEnd();
        json_road["x0"] = start.x;
        json_road["y0"] = start.y;
        if ( road.IsHorizontal() ) {
            json_road["x1"] = end.x;
        }
        if ( road.IsVertical() ) {
            json_road["y1"] = end.y;
        }
        json_roads.push_back(json_road);
    }
    map["roads"] = json_roads;
    // buildings
    json::array json_buildings;
    for (const auto& building : buildings_) {
        json::object json_building;
        Rectangle rect = building.GetBounds();
        json_building["x"] = rect.position.x;
        json_building["y"] = rect.position.y;
        json_building["w"] = rect.size.width;
        json_building["h"] = rect.size.height;
        json_buildings.push_back(json_building);
    }
    map["buildings"] = json_buildings;
    // offices
    json::array json_offices;
    for (const auto& office : offices_) {
        json::object json_office;
        std::string id = *office.GetId();
        Point      pos = office.GetPosition();
        Offset     off = office.GetOffset();
        json_office["id"]      = id;
        json_office["x"]       = pos.x;
        json_office["y"]       = pos.y;
        json_office["offsetX"] = off.dx;
        json_office["offsetY"] = off.dy;
        json_offices.push_back(json_office);
    }
    map["offices"] = json_offices;
    //
    return json::serialize(map);
}



//// Map ///////////////////////////////////////////////////////////////////////////
void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}



//// Game //////////////////////////////////////////////////////////////////////////
void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSession* Game::AddSession(const Map* map) {
    const size_t index = sessions_.size();
    if (auto [it, inserted] = map_id_to_session_.emplace(map->GetId(), index); !inserted) {
        throw std::invalid_argument("GameSession with id "s + *map->GetId() + " already exists"s);
    } else {
        try {

            sessions_.emplace_back(map);
            return &sessions_.at(sessions_.size() - 1);
        } catch (...) {
            map_id_to_session_.erase(it);
            throw;
        }
    }
}

void Game::Tick(uint32_t time_delta) {

}

}  // namespace model
