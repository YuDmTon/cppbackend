#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

//// Aux ///////////////////////////////////////////////////////////////////////////
int GetRandom(int from, int to) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(from, to);
    return dis(gen);
}

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



//// LootType //////////////////////////////////////////////////////////////////////
json::object LootType::ToJson() const {
    json::object json_loot_type;
    json_loot_type["name"]  = name_;
    json_loot_type["file"]  = file_;
    json_loot_type["type"]  = type_;
    json_loot_type["scale"] = scale_;
    if ( rotation_ != ROTATION_DEFAULT ) {
        json_loot_type["rotation"] = rotation_;
    }
    if ( color_ != COLOR_DEFAULT ) {
        json_loot_type["color"] = color_;
    }
    if ( value_ != 0 ) {
        json_loot_type["value"] = value_;
    }
    return json_loot_type;
}

LootType LootType::FromJson(json::object json_loot_type) {
    std::string name     = json::value_to< std::string >(json_loot_type.at("name"));
    std::string file     = json::value_to< std::string >(json_loot_type.at("file"));
    std::string type     = json::value_to< std::string >(json_loot_type.at("type"));
    double      scale    = json::value_to< double >(json_loot_type.at("scale"));
    int         rotation = ROTATION_DEFAULT;
    unsigned    value    = 0;
    std::string color(COLOR_DEFAULT);
    try {
        rotation = json::value_to< int >(json_loot_type.at("rotation"));
    } catch (...) { }
    try {
        color = json::value_to< std::string >(json_loot_type.at("color"));
    } catch (...) { }
    try {
        value = json::value_to< unsigned >(json_loot_type.at("value"));
    } catch (...) { }
    return LootType(name, file, type, scale, rotation, color, value);
}



//// LostObject ////////////////////////////////////////////////////////////////////
unsigned LostObject::CURR_ID = 0;

json::object LostObject::ToJson() const {
    json::object json_lost_object;
    json_lost_object["type"] = type_;
    json::array pos;
    pos.push_back(position_.x);
    pos.push_back(position_.y);
    json_lost_object["pos"] = pos;
    return json_lost_object;
}



//// Road //////////////////////////////////////////////////////////////////////////
json::object Road::ToJson() const {
    json::object json_road;
    json_road["x0"] = start_.x;
    json_road["y0"] = start_.y;
    if ( IsHorizontal() ) {
        json_road["x1"] = end_.x;
    }
    if ( IsVertical() ) {
        json_road["y1"] = end_.y;
    }
    return json_road;
}

Road Road::FromJson(json::object json_road) {
    int x0 = json::value_to< int >(json_road.at("x0"));
    int y0 = json::value_to< int >(json_road.at("y0"));
    try {
        int x1 = json::value_to< int >(json_road.at("x1"));
        return Road(model::Road::HORIZONTAL, {x0, y0}, x1);
    } catch (...) { }
    try {
        int y1 = json::value_to< int >(json_road.at("y1"));
        return Road(model::Road::VERTICAL, {x0, y0}, y1);
    } catch (...) { }
    //
    throw std::runtime_error("Unknown road orientation");
}



//// Building //////////////////////////////////////////////////////////////////////
json::object Building::ToJson() const {
    json::object json_building;
    json_building["x"] = bounds_.position.x;
    json_building["y"] = bounds_.position.y;
    json_building["w"] = bounds_.size.width;
    json_building["h"] = bounds_.size.height;
    return json_building;
}

Building Building::FromJson(json::object json_building) {
    int x = json::value_to< int >(json_building.at("x"));
    int y = json::value_to< int >(json_building.at("y"));
    int w = json::value_to< int >(json_building.at("w"));
    int h = json::value_to< int >(json_building.at("h"));
    return Building(Rectangle({x, y}, {w, h}));
}



//// Office ////////////////////////////////////////////////////////////////////////
json::object Office::ToJson() const {
    json::object json_office;
    json_office["id"]      = *id_;
    json_office["x"]       = position_.x;
    json_office["y"]       = position_.y;
    json_office["offsetX"] = offset_.dx;
    json_office["offsetY"] = offset_.dy;
    return json_office;
}

Office Office::FromJson(json::object json_office) {
    std::string id = json::value_to< std::string >(json_office.at("id"));
    int x          = json::value_to< int >(json_office.at("x"));
    int y          = json::value_to< int >(json_office.at("y"));
    int offset_x   = json::value_to< int >(json_office.at("offsetX"));
    int offset_y   = json::value_to< int >(json_office.at("offsetY"));
    return Office(Office::Id(id), {x, y}, {offset_x, offset_y});
}



//// Map /////////////////////////////////////////////////////////////////////////
std::string Map::Serialize() const {
    //
    json::object map;
    map["id"]   = *id_;
    map["name"] = name_;
    // loots
    json::array json_loot_types;
    for (const auto& loot_type : loot_types_) {
        json_loot_types.push_back(loot_type.ToJson());
    }
    map["lootTypes"] = json_loot_types;
    // roads
    json::array json_roads;
    for (const auto& road : roads_) {
        json_roads.push_back(road.ToJson());
    }
    map["roads"] = json_roads;
    // buildings
    json::array json_buildings;
    for (const auto& building : buildings_) {
        json_buildings.push_back(building.ToJson());
    }
    map["buildings"] = json_buildings;
    // offices
    json::array json_offices;
    for (const auto& office : offices_) {
        json_offices.push_back(office.ToJson());
    }
    map["offices"] = json_offices;
    //
    return json::serialize(map);
}

Map Map::FromJson(json::object json_map, double default_dog_speed, unsigned default_bag_capacity) {
    std::string id   = json::value_to< std::string >(json_map.at("id"));
    std::string name = json::value_to< std::string >(json_map.at("name"));
    //
    double dog_speed = default_dog_speed;
    try {
        dog_speed = json_map.at("dogSpeed").as_double();
    } catch (...) { }
    //
    unsigned bag_capacity = default_bag_capacity;
    try {
        bag_capacity = json_map.at("bagCapacity").as_int64();
    } catch (...) { }
    //
    return Map{Map::Id(id), name, dog_speed, bag_capacity};
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
    double time = static_cast<double>(time_delta) / 1000;
    std::vector<Movement> real_moves;
    for (const auto& road : roads) {
        real_moves.push_back(MoveOnRoad(time, road));
    }
    std::sort(real_moves.begin(), real_moves.end(), MoveComparator);
    Movement do_move = real_moves.back();
    pos_.x = pos_.x + speed_.sx * do_move.distanse;
    pos_.y = pos_.y + speed_.sy * do_move.distanse;
    if ( do_move.stop ) {
        Stop();
    }
}

Movement Dog::MoveOnRoad(double time, const Road& road) {
    bool must_stop = false;
    start_pos_ = pos_;
    if ( road.IsVertical() ) {
        // x
        double cur_x = pos_.x;
        double left  = static_cast<double>(road.GetStart().x) - ROAD_WIDTH / 2;
        double right = static_cast<double>(road.GetStart().x) + ROAD_WIDTH / 2;
        if ( cur_x < left || cur_x > right ) { // пёс не на этой дороге
            return { 0.0, true };
        }
        // y
        double cur_y = pos_.y;
        double beg_y = static_cast<double>(road.GetStart().y < road.GetEnd().y ? road.GetStart().y : road.GetEnd().y) - ROAD_WIDTH / 2;
        double end_y = static_cast<double>(road.GetEnd().y > road.GetStart().y ? road.GetEnd().y : road.GetStart().y) + ROAD_WIDTH / 2;
        if ( cur_y < beg_y || cur_y > end_y ) { // пёс не на этой дороге
            return { 0.0, true };
        }
        // dir
        double x, y;
        switch ( dir_ ) {
            case NORTH:
                y = cur_y + speed_.sy * time;
                if ( y <= beg_y ) {
                    y = beg_y;
                    must_stop = true;
                }
                return { std::abs(cur_y - y) / std::abs(speed_.sy), must_stop };
            case SOUTH:
                y = cur_y + speed_.sy * time;
                if ( y >= end_y ) {
                    y = end_y;
                    must_stop = true;
                }
                return { std::abs(y - cur_y) / std::abs(speed_.sy), must_stop };
            case WEST:
                x = cur_x + speed_.sx * time;
                if ( x <= left ) {
                    x = left;
                    must_stop = true;
                }
                return { std::abs(cur_x - x) / std::abs(speed_.sx), must_stop };
            case EAST:
                x = cur_x + speed_.sx * time;
                if ( x >= right ) {
                    x = right;
                    must_stop = true;
                }
                return { std::abs(x - cur_x) / std::abs(speed_.sx), must_stop };
        }
    } else {
        // y
        double cur_y = pos_.y;
        double down  = static_cast<double>(road.GetStart().y) - ROAD_WIDTH / 2;
        double up    = static_cast<double>(road.GetStart().y) + ROAD_WIDTH / 2;
        if ( cur_y < down || cur_y > up ) { // пёс не на этой дороге
            return { 0.0, true };
        }
        // x
        double cur_x = pos_.x;
        double beg_x = static_cast<double>(road.GetStart().x < road.GetEnd().x ? road.GetStart().x : road.GetEnd().x) - ROAD_WIDTH / 2;
        double end_x = static_cast<double>(road.GetEnd().x > road.GetStart().x ? road.GetEnd().x : road.GetStart().x) + ROAD_WIDTH / 2;
        if ( cur_x < beg_x || cur_x > end_x ) { // пёс не на этой дороге
            return { 0.0, true };
        }
        // dir
        double x, y;
        switch ( dir_ ) {
            case NORTH:
                y = cur_y + speed_.sy * time;
                if ( y <= down ) {
                    y = down;
                    must_stop = true;
                }
                return { std::abs(cur_y - y) / std::abs(speed_.sy), must_stop };
            case SOUTH:
                y = cur_y + speed_.sy * time;
                if ( y >= up ) {
                    y = up;
                    must_stop = true;
                }
                return { std::abs(y - cur_y) / std::abs(speed_.sy), must_stop };
            case WEST:
                x = cur_x + speed_.sx * time;
                if ( x <= beg_x ) {
                    x = beg_x;
                    must_stop = true;
                }
                return { std::abs(cur_x - x) / std::abs(speed_.sx), must_stop };
            case EAST:
                x = cur_x + speed_.sx * time;
                if ( x >= end_x ) {
                    x = end_x;
                    must_stop = true;
                }
                return { std::abs(x - cur_x) / std::abs(speed_.sx), must_stop };
        }
    }
    return { 0.0, true };
}

bool Dog::PushIntoBag(BagItem bag_item, unsigned value) {
    if ( bag_.size() < bag_capacity_ ) {
        bag_.push_back(bag_item);
        value_ += value;
        return true;
    }
    return false;
}

void Dog::EmptyBag() {
    bag_.clear();
    score_ += value_;
    value_ = 0;
}



//// GameSession ///////////////////////////////////////////////////////////////////
void GameSession::Tick(uint32_t time_delta, unsigned lost_count) {
    // create loot
    const Map::Roads& roads = map_->GetRoads();
    for (unsigned i = 0; i < lost_count; ++i) {
        int random_loot    = GetRandom(0, map_->GetLootsCount() - 1);
        int random_road    = GetRandom(0, roads.size() - 1);
        model::Point start = roads[random_road].GetStart();
        model::Point end   = roads[random_road].GetEnd();
        model::Position random_pos;
        if ( roads[random_road].IsVertical() ) {
            random_pos.x = start.x;
            random_pos.y = GetRandom(std::min(start.y, end.y), std::max(start.y, end.y));
        } else {
            random_pos.y = start.y;
            random_pos.x = GetRandom(std::min(start.x, end.x), std::max(start.x, end.x));
        }
        LostObject lost(random_loot, random_pos);
        AddLostObject(lost);
    }
    // move dog
    for (auto& dog : dogs_) {
        dog.Move(time_delta, roads);
    }
    // prepare to gather
    GameProvider gp;
    for (const auto& lost_object : lost_objects_) {
        gp.PushItem({ { lost_object.position_.x, lost_object.position_.y }, LOOT_WIDTHS / 2, false } );
    }
    for (const auto& office : map_->GetOffices()) {
        gp.PushItem({ { static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y) }, OFFICE_WIDTHS / 2, true } );
    }
    for (const auto& dog : dogs_) {
        gp.PushGatherer( { { dog.GetStartPos().x,  dog.GetStartPos().y }, { dog.GetPosition().x,  dog.GetPosition().y }, DOG_WIDTHS / 2} );
    }
    // detect collisions
    std::vector<collision_detector::GatheringEvent> events = FindGatherEvents(gp);
    std::vector<size_t> takens;
    unsigned bag_capacity = GetMap()->GetBagCapacity();
    for (const auto& event : events) {
        LostObject lost_object = lost_objects_.at(event.item_id);
        unsigned   value       = map_->GetLootTypes()[lost_object.type_].value_;
        Dog        dog         = dogs_.at(event.gatherer_id);
        BagItem    bag_item{lost_object.id_, lost_object.type_};
        if ( gp.GetItem(event.item_id).is_office ) {
            dog.EmptyBag();
        } else if ( dog.PushIntoBag(bag_item, value) ) {
            lost_objects_.erase(lost_objects_.begin() + event.item_id);
        }
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
