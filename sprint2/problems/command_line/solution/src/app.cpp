#include "app.h"

namespace app {

//// Player ///////////////////////////////////////////////////////////////////////////////////////


//// Application //////////////////////////////////////////////////////////////////////////////////

// фасад для api_handler
bool Application::GetMap(const std::string& map_id, std::string& res_body) {
    const model::Map* map = game_.FindMap(model::Map::Id(map_id));
    if ( map == nullptr ) {
        return false;
    }
    res_body = map->Serialize();
    return true;
}

bool Application::GetMaps(std::string& res_body) {
    json::array json_maps;
    for (const auto& map : game_.GetMaps()) {
        json::object json_map;
        json_map["id"]   = *map.GetId();
        json_map["name"] =  map.GetName();
        json_maps.push_back(json_map);
    }
    res_body = json::serialize(json_maps);
    return true;
}

bool Application::TryJoin(const std::string& user_name, const std::string& map_id, std::string& res_body) {
    // check map_id
    model::Map::Id id(map_id);
    const model::Map* map = game_.FindMap(id);
    if ( map == nullptr ) {
        return false;
    }

    // get or create GameSession
    model::GameSession* session = game_.FindSession(id);
    if ( session == nullptr ) {
        session = game_.AddSession(map);
    }

    // create dog
    model::Dog* dog = session->AddDog(user_name, DOG_ID++);
    // set dog initial position in random spawn model
    if ( randomize_spawn_ ) {
        auto roads         = map->GetRoads();
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
        dog->SetPosition(random_pos);
//std::cout << "On random road " << random_road << " random initial position = " << random_pos.ToString() << std::endl;
    }

    // create player
    std::string token = players_.Add(dog, session);
    Player* player = players_.FindByToken(token);
    if ( player == nullptr ) {
        std::string err = "Can't get just created player";
        throw std::runtime_error(err);
    }

    // make response
    json::object result;
    result["authToken"] = token;
    result["playerId"]  = player->GetDog()->GetId();
    res_body = json::serialize(result);
    return true;
}

bool Application::GetPlayers(const std::string& token, std::string& res_body) {
    // check game has this token
    if ( !players_.HasToken(token) ) {
        return false;
    }
    // get players_list
    auto players = players_.GetPlayers();
    json::object obj;
    for (auto& player : players) {
        json::object sub_obj;
        sub_obj["name"] = player.GetDog()->GetName();
        obj[std::to_string(player.GetDog()->GetId())] = sub_obj;
    }
    res_body = json::serialize(obj);
    return true;
}

bool Application::GetState(const std::string& token, std::string& res_body) {
    // check game has this token
    if ( !players_.HasToken(token) ) {
        return false;
    }
    // get state
    auto players = players_.GetPlayers();
    json::object obj;
    for (auto& player : players) {
        json::object sub_obj;
        json::array pos;
        model::Dog* dog = player.GetDog();
        pos.push_back(dog->GetPosition().x);
        pos.push_back(dog->GetPosition().y);
        sub_obj["pos"] = pos;
        json::array speed;
        speed.push_back(dog->GetSpeed().sx);
        speed.push_back(dog->GetSpeed().sy);
        sub_obj["speed"] = speed;
        sub_obj["dir"] = dog->GetDir();
        obj[std::to_string(dog->GetId())] = sub_obj;
    }
    json::object result;
    result["players"] = obj;
    res_body = json::serialize(result);
    return true;
}

bool Application::Move(const std::string& token, const std::string& move, std::string& res_body) {
    // try get player
    Player* player = players_.FindByToken(token);
    if ( player == nullptr ) {
        return false;
    }
    // get map speed
    double speed = player->GetSession()->GetMap()->GetDogSpeed();
    // set dog speed
    player->GetDog()->SetSpeed(speed, move);
    res_body = "{}";
    return true;
}

std::string Application::Tick(uint32_t time_delta) {
//std::cout << "Tack !!" << std::endl;
    game_.Tick(time_delta);
    return "{}"s;
}

}   // namespace app
