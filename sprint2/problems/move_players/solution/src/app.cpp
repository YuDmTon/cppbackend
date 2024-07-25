#include "app.h"

namespace app {

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
    std::string map;
    if ( !GetMap(map_id, map) ) {
        return false;
    }
    // add user_name & map_id
    auto [token, id] = players_.Add(user_name, map_id);
    json::object result;
    result["authToken"] = token;
    result["playerId"]  = id;
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
    for (const auto& player : players) {
        json::object sub_obj;
        sub_obj["name"] = player.GetName();
        obj[std::to_string(player.GetId())] = sub_obj;
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
    for (const auto& player : players) {
        json::object sub_obj;
        json::array pos;
        pos.push_back(player.GetPosition().x);
        pos.push_back(player.GetPosition().y);
        sub_obj["pos"] = pos;
        json::array speed;
        speed.push_back(player.GetSpeed().sx);
        speed.push_back(player.GetSpeed().sy);
        sub_obj["speed"] = speed;
        sub_obj["dir"] = player.GetDir();
        obj[std::to_string(player.GetId())] = sub_obj;
    }
    json::object result;
    result["players"] = obj;
    res_body = json::serialize(result);
    return true;
}

bool Application::Move(const std::string& token, const std::string& move, std::string& res_body) {
    // try get player
    std::shared_ptr<Player> player;
    if ( !players_.GetPlayerByToken(token, player) ) {
        return false;
    }
    // get speed
    const model::Map* map = game_.FindMap(model::Map::Id(player->GetMapId()));
    if ( map == nullptr ) {
        std::string err = "Can't find player's map";
        throw std::runtime_error(err);
    }
    double speed = map->GetDogSpeed();
    // set speed
    model::Direction dir;
    if ( !model::StrToDir(move, dir) )
    {
        std::string err = "Wrong move '" + move + "'";
        throw std::runtime_error(err);
    }
    player->SetDogSpeed(speed, dir);
    res_body = "{}";
    return true;
}

}   // namespace app
