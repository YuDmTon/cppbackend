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
    }
    return "?"s;
}

bool StrToDir(std::string str, Direction& dir) {
    if ( str == "U" ) {
        dir = NORTH;
        return true;
    }
    if ( str == "D" ) {
        dir = SOUTH;
        return true;
    }
    if ( str == "L" ) {
        dir = WEST;
        return true;
    }
    if ( str == "R" ) {
        dir = EAST;
        return true;
    }
    //
    return false;
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




//// Dog ///////////////////////////////////////////////////////////////////////////
bool MoveComparator(const Movement& first, const Movement& second) {
    if ( first.distanse < second.distanse ) {
        return true;
    }
    return (int)first.stop > (int)second.stop;
}

void Dog::Move(uint32_t time_delta, const Map::Roads& roads) {
    if ( IsStopped() )
        return;
    double time = (double)time_delta / 1000;
    std::vector<Movement> real_moves;
    for (const auto& road : roads) {
        real_moves.push_back(MoveOnRoad(time, road));
    }
//std::cout << "Dog::Move() real_moves: ";
//for (auto rm : real_moves) std::cout << rm.ToString() << ", ";
//std::cout << std::endl;
    std::sort(real_moves.begin(), real_moves.end(), MoveComparator);
    Movement do_move = real_moves.back();
//std::cout << "            do move = " << do_move.ToString() << std::endl;
//std::cout << "            before = " << pos_.ToString() << std::endl;
    pos_.x = pos_.x + speed_.sx * do_move.distanse;
    pos_.y = pos_.y + speed_.sy * do_move.distanse;
    if ( do_move.stop ) {
//std::cout << "            stop !" << std::endl;
        Stop();
    }
//std::cout << "            after  x = " << pos_.ToString() << std::endl;
}

Movement Dog::MoveOnRoad(double time, const Road& road) {
    bool must_stop = false;
//std::cout << "MoveOnRoad: " << road.GetStart().ToString() << ", " << road.GetEnd().ToString() << ", vertical = " << std::boolalpha << road.IsVertical() << std::endl;
    if ( road.IsVertical() ) {
        // x
        double cur_x = pos_.x;
        double left  = (double)road.GetStart().x - ROAD_WIDTH / 2;
        double right = (double)road.GetStart().x + ROAD_WIDTH / 2;
        if ( cur_x < left || cur_x > right ) { // пёс не на этой дороге
            return { 0.0, true };
        }
        // y
        double cur_y = pos_.y;
        double beg_y = (double)(road.GetStart().y < road.GetEnd().y ? road.GetStart().y : road.GetEnd().y) - ROAD_WIDTH / 2;
        double end_y = (double)(road.GetEnd().y > road.GetStart().y ? road.GetEnd().y : road.GetStart().y) + ROAD_WIDTH / 2;
        if ( cur_y < beg_y || cur_y > end_y ) { // пёс не на этой дороге
            return { 0.0, true };
        }
        // dir
        double x, y;
        switch ( dir_ ) {
            case NORTH:
                y = cur_y + speed_.sy * time;
                if ( y <= beg_y ) { y = beg_y; must_stop = true; }
                return { std::abs(cur_y - y) / std::abs(speed_.sy), must_stop };
            case SOUTH:
                y = cur_y + speed_.sy * time;
                if ( y >= end_y ) { y = end_y; must_stop = true; }
                return { std::abs(y - cur_y) / std::abs(speed_.sy), must_stop };
            case WEST:
                x = cur_x + speed_.sx * time;
                if ( x <= left ) { x = left; must_stop = true; }
                return { std::abs(cur_x - x) / std::abs(speed_.sx), must_stop };
            case EAST:
                x = cur_x + speed_.sx * time;
//std::cout << "EAST: x = " << x << ", must_stop = " << std::boolalpha << must_stop << std::endl;
                if ( x >= right ) { x = right;  must_stop = true; }
//std::cout << "EAST: x = " << x << ", must_stop = " << std::boolalpha << must_stop << std::endl;
                return { std::abs(x - cur_x) / std::abs(speed_.sx), must_stop };
        }
    } else {
        // y
        double cur_y = pos_.y;
        double down  = (double)road.GetStart().y - ROAD_WIDTH / 2;
        double up    = (double)road.GetStart().y + ROAD_WIDTH / 2; 
        if ( cur_y < down || cur_y > up ) { // пёс не на этой дороге
            return { 0.0, true };
        }
        // x
        double cur_x = pos_.x;
        double beg_x = (double)(road.GetStart().x < road.GetEnd().x ? road.GetStart().x : road.GetEnd().x) - ROAD_WIDTH / 2;
        double end_x = (double)(road.GetEnd().x > road.GetStart().x ? road.GetEnd().x : road.GetStart().x) + ROAD_WIDTH / 2;
        if ( cur_x < beg_x || cur_x > end_x ) { // пёс не на этой дороге
            return { 0.0, true };
        }
        // dir
//std::cout << "cur_x = " << cur_x << ", beg_x = " << beg_x << ", end_x = " << end_x << std::endl;
        double x, y;
        switch ( dir_ ) {
            case NORTH:
                y = cur_y + speed_.sy * time;
                if ( y <= down ) { y = down;  must_stop = true; }
                return { std::abs(cur_y - y) / std::abs(speed_.sy), must_stop };
            case SOUTH:
                y = cur_y + speed_.sy * time;
                if ( y >= up ) { y = up;  must_stop = true; }
                return { std::abs(y - cur_y) / std::abs(speed_.sy), must_stop };
            case WEST:
                x = cur_x + speed_.sx * time;
                if ( x <= beg_x ) { x = beg_x;  must_stop = true; }
                return { std::abs(cur_x - x) / std::abs(speed_.sx), must_stop };
            case EAST:
                x = cur_x + speed_.sx * time;
//std::cout << "EAST: x = " << x << ", must_stop = " << std::boolalpha << must_stop << std::endl;
                if ( x >= end_x ) { x = end_x;  must_stop = true; }
//std::cout << "EAST: x = " << x << ", must_stop = " << std::boolalpha << must_stop << std::endl;
                return { std::abs(x - cur_x) / std::abs(speed_.sx), must_stop };
        }
    }
    return { 0.0, true };
}



//// GameSession ///////////////////////////////////////////////////////////////////
void GameSession::Tick(uint32_t time_delta) {
    const Map::Roads& roads = map_->GetRoads();
    for (auto& dog : dogs_) {
        dog.Move(time_delta, roads);
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
            GameSession session(map);
            sessions_.push_back(session);
            return &sessions_.at(sessions_.size() - 1);
        } catch (...) {
            map_id_to_session_.erase(it);
            throw;
        }
    }
}


}  // namespace model
