#include "app.h"

namespace app {

//// Application //////////////////////////////////////////////////////////////////////////////////

// фасад для api_handler
bool Application::GetMap(const std::string& map_id, std::string& res_body) {
    std::string config = game_.GetConfig();
    //
    auto maps  = json::parse(config).as_object().at("maps").as_array();
    auto is_id = [map_id](auto map) { return map_id == map.as_object().at("id").as_string(); };
    if (auto it = std::find_if(maps.begin(), maps.end(), is_id); it != maps.end()) {
        res_body = json::serialize(*it);
        return true;
    }
    return false;
}

bool Application::GetMaps(std::string& res_body) {
    std::string  config = game_.GetConfig();
    //
    auto maps = json::parse(config);
    json::array result;
    for (const auto& map : maps.as_object().at("maps").as_array()) {
        json::object id;
        id["id"]   = map.as_object().at("id");
        id["name"] = map.as_object().at("name");
        result.push_back(id);
    }
    res_body = json::serialize(result);
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

}   // namespace app
