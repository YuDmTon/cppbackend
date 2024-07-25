#pragma once

#include <boost/json.hpp>

#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.h"

namespace app {

using namespace std::literals;

namespace json  = boost::json;


//// Player ///////////////////////////////////////////////////////////////////////////////////////
class Player {
public:
    Player(uint32_t user_id, std::string user_name, std::string map_id) : user_id_(user_id), user_name_(user_name), map_id_(map_id) { 
        // TODO !!!
        dog_ = model::Dog(user_name, user_id);
    }
    const std::string& GetName()     const { return user_name_; }
    const uint32_t     GetId()       const { return user_id_; }
    const std::string& GetMapId()    const { return map_id_; }
    model::Position    GetPosition() const { return dog_.GetPosition(); }
    model::Speed       GetSpeed()    const { return dog_.GetSpeed(); }
    std::string        GetDir()      const { return dog_.GetDir(); }
    void SetDogSpeed(double speed, model::Direction dir) {  dog_.SetSpeed(speed, dir); }

private:
    uint32_t    user_id_;
    std::string user_name_;
    std::string map_id_;
    // TODO !!!
    model::Dog  dog_;
};  // Player


//// PlayerToken Generator ////////////////////////////////////////////////////////////////////////
class PlayerToken {
    constexpr static size_t TOKEN_SIZE = 32;
public:
    PlayerToken() = default;
    std::string Get() {
        std::ostringstream oss;
        oss << std::hex << generator1_() << generator2_();
        std::string token = oss.str();
        while ( token.size() < TOKEN_SIZE ) {
            token = "0" + token;
        }
        return token;
    }
private:
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
};  // PlayerToken


//// Players //////////////////////////////////////////////////////////////////////////////////////
class Players {
public:
    Players() { }
    std::pair<std::string, uint32_t> Add(std::string user_name, std::string map_id) {
        std::string token = PlayerToken().Get();
        uint32_t dog_id = players_.size();
        Player player(dog_id, user_name, map_id);
        players_.push_back(player);
        ////players_.emplace_back{dog_id, user_name, map_id};
        //
        tokens_[token] = dog_id;
        //
        std::ostringstream ids;
        ids << dog_id;
        ids << ":";
        ids << map_id;
        ids_[ids.str()] = dog_id;
        //
        return {token, dog_id};
    }
    bool HasToken(std::string token) const {
        return tokens_.find(token) != tokens_.end();
    }
    bool GetPlayerByToken(std::string token, std::shared_ptr<Player>& player) const {
        if ( HasToken(token) ) {
            player = std::make_shared<Player>(players_.at(tokens_.at(token)));
            return true;
        }
        return false;
    }
    const std::vector<Player>& GetPlayers() const {
        return players_;
    }
private:
    std::vector<Player> players_;
    std::unordered_map<std::string, size_t> tokens_;
    std::unordered_map<std::string, size_t> ids_;
};  // Players


//// Application //////////////////////////////////////////////////////////////////////////////////
class Application {
public:
    explicit Application(model::Game& game) : game_(game) {
    }
    // фасад для api_handler
    bool GetMap(const std::string& map_id, std::string& res_body);
    bool GetMaps(std::string& res_body);
    bool TryJoin(const std::string& user_name, const std::string& map_id, std::string& res_body);
    bool GetPlayers(const std::string& token, std::string& res_body);
    bool GetState(const std::string& token, std::string& res_body);
    bool CheckDirStr(const std::string& move) { model::Direction dir; return model::StrToDir(move, dir); }
    bool Move(const std::string& token, const std::string& move, std::string& res_body);

private:
    model::Game& game_;
    Players      players_;
};  // Application

}   // namespace app
