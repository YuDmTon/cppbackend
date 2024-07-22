#pragma once

#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>


namespace logic {

class Player {
public:
    Player(uint32_t user_id, std::string user_name, std::string map_id) : user_id_(user_id), user_name_(user_name), map_id_(map_id) { }
    const std::string& GetName() const { return user_name_; }
    const uint32_t GetId() const { return user_id_; }
private:
    uint32_t    user_id_;
    std::string user_name_;
    std::string map_id_;
};


class PlayerToken {
public:
    PlayerToken() = default;
    std::string Get() {
        std::ostringstream oss;
        oss << std::hex << generator1_() << generator2_();
        return oss.str();
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
};


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
    const std::vector<Player>& GetPlayers() const {
        return players_;
    }
    
private:
    std::vector<Player> players_;
    std::unordered_map<std::string, size_t> tokens_;
    std::unordered_map<std::string, size_t> ids_;
};

}   // namespace logic
