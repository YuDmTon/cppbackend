#pragma once

#include <boost/json.hpp>

#include <algorithm>
#include <iostream>
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
    Player(model::Dog* dog, const model::GameSession* session) : dog_(dog), session_(session) { }
    model::Dog* GetDog() { return dog_; }
    const model::GameSession* GetSession() const { return session_; }

private:
    model::Dog* dog_;
    const model::GameSession* session_;
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
    Players() = default;
    std::string Add(model::Dog* dog, const model::GameSession* session) {
        std::string  token = PlayerToken().Get();
        const size_t index = players_.size();
        Player player(dog, session);
        players_.push_back(player);
        //
        token_to_index_[token] = index;
        //
        return token;
    }
    bool HasToken(std::string token) const {
        return token_to_index_.find(token) != token_to_index_.end();
    }
    Player* FindByToken(std::string token) {
        if ( HasToken(token) ) {
            return &players_.at(token_to_index_.at(token));
        }
        return nullptr;
    }
    const std::vector<Player>& GetPlayers() const { return players_; }

private:
    model::Dog*               dog_;
    const model::GameSession* session_;
    //
    std::vector<Player> players_;
    std::unordered_map<std::string, size_t> token_to_index_;
};  // Players


//// Application //////////////////////////////////////////////////////////////////////////////////
class Application {
    uint32_t dog_id_;
public:
    explicit Application(model::Game& game, bool debug_mode, bool randomize_spawn) 
            : game_(game)
            , debug_mode_(debug_mode)
            , randomize_spawn_(randomize_spawn) { 
        dog_id_ = 0;
    }
    
    // unauthorized
    bool GetMap(const std::string& map_id, std::string& res_body);
    bool GetMaps(std::string& res_body);
    bool TryJoin(const std::string& user_name, const std::string& map_id, std::string& res_body);
    // authorized
    bool GetPlayers(const std::string& token, std::string& res_body);
    bool GetState(const std::string& token, std::string& res_body);
    bool Move(const std::string& token, const std::string& move, std::string& res_body);
    // debug (unauthorized)
    std::string Tick(uint32_t time_delta);

private:
/*
    int GetRandom(int from, int to) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(from, to);
        return dis(gen);
    }
*/
private:
    model::Game& game_;
    Players      players_;
    bool         debug_mode_;
    bool         randomize_spawn_;
};  // Application

}   // namespace app
