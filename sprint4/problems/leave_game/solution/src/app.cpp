#include "app.h"
#include "serializer.h"

namespace app {

//// Player ///////////////////////////////////////////////////////////////////////////////////////
std::string Player::ToString(std::string offs) const {
    std::ostringstream oss;
    oss << offs << "dog_ptr     = "  << dog_ << "\n";
    oss << offs << "dog_id_     = "  << dog_id_ << "\n";
    if ( dog_ != nullptr ) {
        oss << offs << "dog name_   = '" << dog_->GetName() << "' (" << dog_->GetId() << ")" << "\n";
    } else {
        oss << offs << "dog name_   = 'nullptr'\n";
    }
    oss << offs << "session_ptr = "  << session_ << "\n";
    oss << offs << "session_id_ = "  << *session_id_ << "\n";
    if ( session_ != nullptr ) {
        oss << offs << "map name    = '" << session_->GetMap()->GetName() << "' (" << *session_->GetMap()->GetId() << ")" << "\n";
    } else {
        oss << offs << "map name    = 'nullptr'\n";
    }
    return oss.str();
}



//// Players //////////////////////////////////////////////////////////////////////////////////////
std::vector<std::tuple<std::string, int, int>> Players::GetRetiredPlayers() {
    // try find retired dogs
    std::vector<RetiredPlayer> retired_players;
    for (size_t idx = 0; idx < players_.size(); ++idx) {
        model::Dog* dog = players_[idx].GetDog();
        auto play_time = dog->GetPlayTime();
        if ( play_time.has_value() ) {
            RetiredPlayer retired_player{dog->GetName(), static_cast<int>(dog->GetScore()), static_cast<int>(play_time.value()), idx};
            retired_players.push_back(retired_player);
        }
    }
    // process found retired dogs
    std::vector<std::tuple<std::string, int, int>> result;
    for (const auto& retired_player : retired_players) {
        result.emplace_back(retired_player.name, retired_player.score, retired_player.play_time_ms);
        // remove player
        players_.erase(players_.begin() + retired_player.idx);
        // find & remove player token
        std::string retired_token;
        for (const auto& [token, idx] : token_to_index_) {
            if ( idx == retired_player.idx ) {
                retired_token = token;
                break;
            }
        }
        if ( !retired_token.empty() ) {
            token_to_index_.erase(retired_token);
        }
    }
    return result;
}

std::string Players::ToString() const {
    std::ostringstream oss;
    oss << "--- Players:\n";
    oss << "\tcounter = "  << players_.size() << "(" << token_to_index_.size() << ")\n";
    for (const auto& [token, idx] : token_to_index_) {
        oss << "\ttoken   = '" << token << "'" << "\n";
        oss << "\tidx     = "  << idx << "\n";
//        oss << "\t---player:\n = " << players_[idx].ToString("\t\t") << "\n";
    }
    for (const auto& player : players_) {
        oss << "\t---player:\n" << player.ToString("\t\t") << "\n";
    }
    oss << "\n";
    return oss.str();
}


//// Application //////////////////////////////////////////////////////////////////////////////////
//// фасад для api_handler
std::string Application::ToString() const {
    std::ostringstream oss;
    oss << "--- Application ---\n";
    oss << game_.ToString();
    oss << players_.ToString();
    oss << "---\n";
    oss << "debug_mode_      = " << std::boolalpha << debug_mode_ << "\n";
    oss << "randomize_spawn_ = " << std::boolalpha << randomize_spawn_ << "\n";
    oss << "state_file_      = " << state_file_ << "\n";
    oss << "save_period_     = " << save_period_ << "\n";
    oss << "dog_id_          = " << dog_id_ << "\n";
    oss << "curr_time_       = " << curr_time_ << "\n";
    oss << "save_time_       = " << save_time_ << "\n";
    oss << "\n";
    return oss.str();
}

std::optional<std::string> Application::GetMap(const std::string& map_id) {
    const model::Map* map = game_.FindMap(model::Map::Id(map_id));
    if ( map == nullptr ) {
        return std::nullopt;
    }
    return { map->Serialize() };
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
    model::Dog* dog = session->AddDog(user_name, dog_id_++, curr_time_);
    dog->SetBagCapacity(map->GetBagCapacity());
    // set dog initial position in random spawn model
    if ( randomize_spawn_ ) {
        auto roads         = map->GetRoads();
        int random_road    = model::GetRandom(0, roads.size() - 1);
        model::Point start = roads[random_road].GetStart();
        model::Point end   = roads[random_road].GetEnd();
        model::Position random_pos;
        if ( roads[random_road].IsVertical() ) {
            random_pos.x = start.x;
            random_pos.y = model::GetRandom(std::min(start.y, end.y), std::max(start.y, end.y));
        } else {
            random_pos.y = start.y;
            random_pos.x = model::GetRandom(std::min(start.x, end.x), std::max(start.x, end.x));
        }
        dog->SetPosition(random_pos);
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
    auto player  = players_.FindByToken(token);
    if ( player == nullptr ) {
        return false;
    }
    // get this player map
    const model::Map* map = player->GetSession()->GetMap();
    assert(map);

    // get state of only this player map
    // all dogs on the player map
    json::object dogs;
    auto players = players_.GetPlayers();
    for (auto& player : players) {
        if ( player.GetSession()->GetMap() != map ) {
            continue;
        }
        json::object json_dog;
        //
        json::array json_pos;
        model::Dog* dog = player.GetDog();
        assert(dog);
        json_pos.push_back(dog->GetPosition().x);
        json_pos.push_back(dog->GetPosition().y);
        json_dog["pos"] = json_pos;
        //
        json::array json_speed;
        json_speed.push_back(dog->GetSpeed().sx);
        json_speed.push_back(dog->GetSpeed().sy);
        json_dog["speed"] = json_speed;
        //
        json_dog["dir"] = dog->GetDir();
        //
        json::array  json_bag;
        for (const auto& bag_item : dog->GetBag()) {
            json::object json_bag_item;
            json_bag_item["id"]   = bag_item.id_;
            json_bag_item["type"] = bag_item.type_;
            json_bag.push_back(json_bag_item);
        }
        json_dog["bag"]   = json_bag;
        //
        json_dog["score"] = dog->GetScore();
        //
        dogs[std::to_string(dog->GetId())] = json_dog;
    }
    // all lost objects on the player map (session)
    const model::GameSession* session = player->GetSession();
    json::object json_losts;
    for (auto& lost : session->GetLostObjects()) {
        json_losts[std::to_string(lost.id_)] = lost.ToJson();
    }

    // return json
    json::object result;
    result["players"]  = dogs;
    if ( session->GetLostsCount() != 0 ) {
        result["lostObjects"] = json_losts;
    }
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
    game_.Tick(curr_time_, time_delta);
    // --- is time to save state ?
    curr_time_ += time_delta;
    if ( curr_time_ - save_time_ > save_period_ ) {
        save_time_ = curr_time_;
        serialization::SaveApp(*this);
    }
    // --- try find new retired dogs & save if found
    db_.SaveRetiredPlayers(players_.GetRetiredPlayers());
    // --- response
    return "{}"s;
}

bool Application::GetRecords(std::string& res_body) {
    auto retired_players = db_.ReadRetiredPlayers();
    json::array arr;
    for (auto& retired_player : retired_players) {
        json::object item;
        item["name"]     = std::get<0>(retired_player);
        item["score"]    = std::get<1>(retired_player);
        item["playTime"] = std::get<2>(retired_player);
        arr.push_back(item);
    }
    res_body = json::serialize(arr);
    return true;
}

}   // namespace app
