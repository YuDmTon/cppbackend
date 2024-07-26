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
//std::cout << "Join 1: Map found with id = '" << *map->GetId() << "'" << std::endl;

    // get or create GameSession
    model::GameSession* session = game_.FindSession(id);
    if ( session == nullptr ) {
        session = game_.AddSession(map);
    }
//std::cout << "Join 2: Session Map id = '" << *map->GetId() << "'" << std::endl;

    // create dog
    model::Dog* dog = session->AddDog(user_name, DOG_ID++);
//std::cout << "Dog '" << dog->GetName() << "' with id " << dog->GetId() << " was added !!! Dog ptr = " << std::hex << dog << ", dog size = " << sizeof(model::Dog) <<  std::endl;

// test
/*
auto dogs = session->GetDogs();
std::cout << "Join T1: Dogs count = " << dogs.size() << std::endl;
size_t i = 0;
for (auto& dg : dogs) {
  std::cout << "         dog #" << i << ": name = '" << dg.GetName() << "', id = " << dg.GetId() << std::endl;
  ++i;
}
auto  p = players_.GetPlayers();
std::cout << "Join T2: players count = " << p.size() << std::endl;
*/

    // create player
    std::string token = players_.Add(dog, session);
    Player* player = players_.FindByToken(token);
    if ( player == nullptr ) {
        std::string err = "Can't get just created player";
        throw std::runtime_error(err);
    }
//std::cout << "Join 4: Player added to map = '" << *player->GetSession()->GetMap()->GetId() << "' with dog = '" << player->GetDog()->GetName() << "'" << std::endl;

    // set dog initial position
/*
    model::Point start = map->GetRoads()[0].GetStart();
    player->GetDog()->SetPosition({(double)start.x, (double)start.y});   // TODO !!! потом убрать ???
//std::cout << "Initial dog pos: x = " << start.x << ", y = " << start.y << std::endl;
*/
    // test
/*
    auto players = players_.GetPlayers();
std::cout << "Join 5: players count = " << players.size() << std::endl;
    for (auto& player : players) {
        model::Dog* dog = player.GetDog();
std::cout << "        dog ptr = " << std::hex << dog << std::endl;
std::cout << "        Dog '" << dog->GetName() << "' at pos = {" << dog->GetPosition().x << ", " << dog->GetPosition().y
                                   << "} with speed = {" << dog->GetSpeed().sx << ", " << dog->GetSpeed().sy
                                      << "} and dir = '" << dog->GetDir() << "'" << std::endl;
    }
*/
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
    // get speed
    double speed = player->GetSession()->GetMap()->GetDogSpeed();
    // set speed
    model::Direction dir;
    if ( !model::StrToDir(move, dir) )
    {
        std::string err = "Wrong move '" + move + "'";
        throw std::runtime_error(err);
    }
    player->GetDog()->SetSpeed(speed, dir);
    res_body = "{}";
    return true;
}

std::string Application::Tick(uint32_t time_delta) {
    game_.Tick(time_delta);
    return "{}"s;
}

}   // namespace app
