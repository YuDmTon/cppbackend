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

bool Application::Move(const std::string& token, const std::string& move, std::string& res_body) {
    // try get player
    std::shared_ptr<Player> player;
    if ( !players_.GetPlayerByToken(token, player) ) {
        return false;
    }
    // get speed
    double speed = GetDogSpeed(player->GetMapId());
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


//private:  ////////////////////////////////////////////////////////////////////////
double Application::GetDogSpeed(const std::string& map_id) {
    std::string  config = game_.GetConfig();
    //
    json::value json_obj = json::parse(config).as_object();
//std::cout << json::serialize(json_obj) << std::endl;
    
    double dog_speed = 1;
    try {
        dog_speed = json_obj.at("defaultDogSpeed").as_double();
    } catch (...) {
//std::cout << "No defaultDogSpeed" << std::endl;
    }
//std::cout << "defaultDogSpeed = " << dog_speed << std::endl;
    
    json::array maps = json_obj.at("maps").as_array();
    for (const auto& map : maps) {
        if ( map.as_object().at("id").as_string() == map_id ) {
//std::cout << "map '" << map_id << "' is found" << std::endl;
            try {
                dog_speed = map.at("dogSpeed").as_double();
            } catch (...) {
//std::cout << "No dogSpeed" << std::endl;
            }
        }
    }
//std::cout << "dogSpeed = " << dog_speed << std::endl;
    return dog_speed;
}

}   // namespace app
