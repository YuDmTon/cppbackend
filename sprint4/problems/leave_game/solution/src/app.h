#pragma once

#include <boost/json.hpp>

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "model.h"
#include "postgres.h"

namespace app {

using namespace std::literals;

namespace json = boost::json;
namespace fs   = std::filesystem;


//// Player ///////////////////////////////////////////////////////////////////////////////////////
class Player {
public:
    Player(model::Dog* dog, const model::GameSession* session)
        : dog_(dog)
        , session_(session)
        , dog_id_(0)
        , session_id_("")
    {
        if ( dog_ != nullptr ) {
            dog_id_ = dog->GetId();
        }
        if ( session_ != nullptr ) {
            session_id_ = session->GetMapId();
        }
    }
    model::Dog* GetDog() const noexcept { return dog_; }
    const model::GameSession* GetSession() const noexcept { return session_; }
    // for deserialization only
    uint32_t GetDogId() const noexcept { return dog_id_; }
    model::Map::Id GetSessionId() const noexcept { return session_id_; }
    void SetDogId(uint32_t dog_id) { dog_id_ = dog_id; }
    void SetSessionId(model::Map::Id session_id) { session_id_ = session_id; }
    //
    std::string ToString(std::string offs) const;

private:
    model::Dog*               dog_;
    const model::GameSession* session_;
    // for deserialization only
    uint32_t                  dog_id_;
    model::Map::Id            session_id_;
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
struct RetiredPlayer {
    std::string name;
    int         score;
    int         play_time_ms;
    //
    size_t      idx;
};

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
    bool HasToken(std::string token) const noexcept {
        return token_to_index_.find(token) != token_to_index_.end();
    }
    Player* FindByToken(std::string token) noexcept {
        if ( HasToken(token) ) {
            return &players_.at(token_to_index_.at(token));
        }
        return nullptr;
    }
    const std::vector<Player>& GetPlayers() const noexcept { return players_; }
    // for deserialization only
    const std::unordered_map<std::string, size_t>  GetPlayersTokenToIndex() const noexcept { return token_to_index_; }
    Player* AddPlayer(Player player) {
        players_.push_back(player);
        return &players_.back();
    }
    void AddPlayersTokenAndIndex(std::string token, size_t index) { token_to_index_[token] = index; }
    // retiring
    std::vector<std::tuple<std::string, int, int>> GetRetiredPlayers();
    //
    std::string ToString() const;

private:
    std::string DebugToken() {
        const int   TOKEN_SIZE = 32;
        std::string debug_token(TOKEN_SIZE, static_cast<char>(0x30 + players_.size()));
        return debug_token;
    }

private:
    std::vector<Player> players_;
    std::unordered_map<std::string, size_t> token_to_index_;
};  // Players


//// Application //////////////////////////////////////////////////////////////////////////////////
class Application {
public:
    explicit Application(postgres::Db& db, model::Game& game, bool debug_mode, bool randomize_spawn, std::string state_file, uint32_t save_period)
            : db_(db)
            , game_(game)
            , debug_mode_(debug_mode)
            , randomize_spawn_(randomize_spawn)
            , state_file_(state_file)
            , save_period_(save_period)
            , dog_id_(0)
            , curr_time_(0)
            , save_time_(0) {
    }

    // unauthorized
    std::optional<std::string> GetMap(const std::string& map_id);
    bool GetMaps(std::string& res_body);
    bool TryJoin(const std::string& user_name, const std::string& map_id, std::string& res_body);
    bool GetRecords(std::string& res_body);
    // authorized
    bool GetPlayers(const std::string& token, std::string& res_body);
    bool GetState(const std::string& token, std::string& res_body);
    bool Move(const std::string& token, const std::string& move, std::string& res_body);
    // debug (unauthorized)
    std::string Tick(uint32_t time_delta);
    // for deserialization only
    std::string GetStateFile() const noexcept { return state_file_; }
    //
    model::Game& GetGame() const noexcept { return game_; }
    const Players& GetPlayers() const noexcept { return players_; }
    uint32_t GetDogId() const noexcept { return dog_id_; }
    //
    Player* AddPlayer(Player player) { return players_.AddPlayer(player); }
    void AddPlayersTokenAndIndex(std::string token, size_t index) { players_.AddPlayersTokenAndIndex(token, index); }
    void SetDogId(uint32_t dog_id) { dog_id_ = dog_id; }
    //
    std::string ToString() const;

private:
    // components
    postgres::Db& db_;
    model::Game&  game_;
    Players       players_;
    // command line arguements
    bool          debug_mode_;
    bool          randomize_spawn_;
    std::string   state_file_;
    uint32_t      save_period_;
    // work variables
    uint32_t      dog_id_;
    uint64_t      curr_time_;
    uint64_t      save_time_;
};  // Application

}   // namespace app
