#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "app.h"
#include "model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, Position& obj, [[maybe_unused]] const unsigned version) {
    ar & obj.x;
    ar & obj.y;
}

template <typename Archive>
void serialize(Archive& ar, Speed& obj, [[maybe_unused]] const unsigned version) {
    ar & obj.sx;
    ar & obj.sy;
}

template <typename Archive>
void serialize(Archive& ar, BagItem& obj, [[maybe_unused]] const unsigned version) {
    ar & obj.id_;
    ar & obj.type_;
}

template <typename Archive>
void serialize(Archive& ar, LostObject& obj, [[maybe_unused]] const unsigned version) {
    ar & obj.id_;
    ar & obj.type_;
    ar & obj.position_;
    ar & obj.CURR_ID;
}

}  // namespace model

namespace app {

}  // namespace app




namespace serialization {

//// model !!! //////////////////////////////////////////////////////////
// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : name_(dog.GetName())
        , id_(dog.GetId())
        //
        , pos_(dog.GetPosition())
        , start_pos_(dog.GetStartPos())
        , speed_(dog.GetSpeed())
        , dir_(dog.GetDirection())
        //
        , bag_(dog.GetBag())
        , bag_capacity_(dog.GetBagCapacity())
        , score_(dog.GetScore())
        , value_(dog.GetValue())
    { }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{name_, id_};
        //
        dog.SetPosition(pos_);
        dog.SetStartPos(start_pos_);
        dog.SetSpeed(speed_);
        dog.SetDirection(dir_);
        //
        dog.SetBagCapacity(bag_capacity_);
        for (const auto& bag_item : bag_) {
            if (!dog.PushIntoBag(bag_item, 0)) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        //
        dog.SetScore(score_);
        dog.SetValue(value_);
        //
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & name_;
        ar & id_;
        //
        ar & pos_;
        ar & start_pos_;
        ar & speed_;
        ar & dir_;
        //
        ar & bag_;
        ar & bag_capacity_;
        ar & score_;
        ar & value_;
    }

    std::string GetName() const { return name_; }
    unsigned GetId() const { return id_; }

private:
    std::string      name_;
    unsigned         id_ = 0;
    //
    model::Position  pos_;
    model::Position  start_pos_;
    model::Speed     speed_;
    model::Direction dir_ = model::Direction::NORTH;
    model::Bag       bag_;
    size_t           bag_capacity_ = 0;
    unsigned         score_ = 0;
    unsigned         value_ = 0;
};
//////


// GameSessionRepr (GameSessionRepresentation) - сериализованное представление класса GameSession
class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& session)
        : lost_objects_(session.GetLostObjects())
        , map_id_(session.GetMapId())
    {
        for (const auto& dog : session.GetDogs()) {
            dogs_repr_.push_back(DogRepr(dog));
        }
    }

    void Restore(model::GameSession* session) const {
        for (const auto& dog_repr : dogs_repr_) {
            session->AddDog(dog_repr.Restore());
        }
        for (const auto& lost_object : lost_objects_) {
            session->AddLostObject(lost_object);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & dogs_repr_;
        ar & lost_objects_;
        ar & *map_id_;
    }

    model::Map::Id GetSessionId() const noexcept { return map_id_; }

private:
    std::vector<DogRepr>           dogs_repr_;
    std::vector<model::LostObject> lost_objects_;
    model::Map::Id                 map_id_ = model::Map::Id{""};
};


//// app !!! ////////////////////////////////////////////////////////////
// PlayerRepr (PlayerRepresentation) - сериализованное представление класса Player
class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const app::Player& player)
        : dog_id_(player.GetDogId())
        , session_id_(player.GetSessionId())
    { }

    [[nodiscard]] app::Player Restore() const {
        app::Player player{nullptr, nullptr};
        //
        player.SetDogId(dog_id_);
        player.SetSessionId(session_id_);
        return player;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & dog_id_;
        ar & *session_id_;
    }

    uint32_t GetDogId() const noexcept { return dog_id_; }
    model::Map::Id GetSessionId() const noexcept { return session_id_; }

private:
    uint32_t       dog_id_;
    model::Map::Id session_id_ = model::Map::Id{""};
};
//////

// AppRepr (AppRepresentation) - сериализованное представление класса Application
class AppRepr {
public:
    AppRepr() = default;

    explicit AppRepr(const app::Application& app) : dog_id_(app.GetDogId()) {
        for (const auto& session : app.GetGame().GetSessions()) {
            sessions_repr_.push_back(GameSessionRepr(session));
        }
        for (const auto& player : app.GetPlayers().GetPlayers()) {
            players_repr_.push_back(PlayerRepr(player));
        }
        for (const auto [token, index] : app.GetPlayers().GetPlayersTokenToIndex()) {
            token_to_index_[token] = index;
        }
    }

    void Restore(app::Application& app) const {
        model::Game& game = app.GetGame();
        app.SetDogId(dog_id_);
        for (const auto& session_repr : sessions_repr_) {
            const model::Map* map = game.FindMap(session_repr.GetSessionId());
            if ( map == nullptr ) {
                std::string err = "Session: can't find map with id " + *session_repr.GetSessionId();
                throw std::runtime_error(err);
            }
            model::GameSession* session = game.AddSession(map);
            session_repr.Restore(session);
        }
        for (const auto& player_repr : players_repr_) {
            model::Dog* dog = game.FindDog(player_repr.GetDogId());
            if ( dog == nullptr ) {
                std::string err = "Player: can't find dog with id " + *player_repr.GetSessionId();
                throw std::runtime_error(err);
            }
            model::GameSession* session = game.FindSession(player_repr.GetSessionId());
            if ( session == nullptr ) {
                std::string err = "Player: can't find session with id " + *player_repr.GetSessionId();
                throw std::runtime_error(err);
            }
            app::Player player(dog, session);
            player.SetDogId(player_repr.GetDogId());
            player.SetSessionId(player_repr.GetSessionId());
            app.AddPlayer(player);
        }
        for (const auto [token, index] : token_to_index_) {
            app.AddPlayersTokenAndIndex(token, index);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & sessions_repr_;
        ar & players_repr_;
        ar & token_to_index_;
        ar & dog_id_;
    }

private:
    std::vector<GameSessionRepr>            sessions_repr_;
    std::vector<PlayerRepr>                 players_repr_;
    std::unordered_map<std::string, size_t> token_to_index_;
    uint32_t                                dog_id_;
};
//////


void SaveApp(app::Application& app);
void RestoreApp(app::Application& app);

void TestVectorDogsReps(std::ostream& os, std::string file_name);
void TestPlayersReps(const app::Application& app, std::ostream& os, const std::string file_name);

}  // namespace serialization
