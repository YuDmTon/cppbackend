#pragma once

#include <iostream>
#include <list>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

namespace model {

//// Model's Aux ///////////////////////////////////////////////////////////////////
using Dimension = int;
using Coord     = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};



//// Road //////////////////////////////////////////////////////////////////////////
class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};



//// Building //////////////////////////////////////////////////////////////////////
class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};



//// Office ////////////////////////////////////////////////////////////////////////
class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};



//// Map ///////////////////////////////////////////////////////////////////////////
class Map {
public:
    using Id        = util::Tagged<std::string, Map>;
    using Roads     = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices   = std::vector<Office>;

    Map(Id id, std::string name, double dog_speed) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(dog_speed) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    double GetDogSpeed() const noexcept {
        return dog_speed_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    std::string Serialize() const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    double dog_speed_;
};



//// Dog ///////////////////////////////////////////////////////////////////////////
enum Direction {
    NORTH,          // U
    SOUTH,          // D
    WEST,           // L
    EAST,           // R
    //
    STOP
};

std::string DirToStr(Direction dir);
bool StrToDir(std::string str, Direction& dir);

struct Position {
    double x = 0;
    double y = 0;
};

struct Speed {
    double sx = 0;
    double sy = 0;
};

double GetRandom(double from, double to);

class Dog {
public:
    Dog(std::string name, uint32_t id) : name_(name), id_(id) {
/*
std::cout << "Dog '" << name_ << "' with id " << id_
<< " was  created at pos = {" << pos_.x << ", " << pos_.y
<< "} with speed = {" << speed_.sx << ", " << speed_.sy
<< "} and dir = '" << DirToStr(dir_) << "'" << std::endl;
*/
    }
    //
    const std::string GetName()     const { return name_; }
    const uint32_t    GetId()       const { return id_; }
    Position          GetPosition() const { return pos_; }
    Speed             GetSpeed()    const { return speed_; }
    std::string       GetDir()      const { return DirToStr(dir_); }
    //
    void SetPosition(Position pos) { pos_ = pos; }
    void SetSpeed(double speed, Direction dir) {
        dir_ = dir;
        switch ( dir_ ) {
            case NORTH: speed_ = { 0,     -speed };
            case SOUTH: speed_ = { 0,     speed  };
            case WEST:  speed_ = {-speed, 0 };
            case EAST:  speed_ = { speed, 0 };
            case STOP:  speed_ = { 0,     0 };
        }
    }
private:
    std::string name_;
    uint32_t    id_;
    //
    Position    pos_{0, 0};
    Speed       speed_{0, 0};
    Direction   dir_{NORTH};
};



//// GameSession ///////////////////////////////////////////////////////////////////
class GameSession {
public:
    explicit GameSession(const Map* map) : map_(map) { }
    Dog* AddDog(std::string name, uint32_t id) { 
        dogs_.emplace_back(name, id);
        return &dogs_.back();//at(dogs_.size() - 1);
    }
    //
    const Map* GetMap() const { return map_; }
    const std::list<Dog>& GetDogs() const { return dogs_; }

private:
    const Map*       map_;
    std::list<Dog> dogs_;
};




//// Game //////////////////////////////////////////////////////////////////////////
class Game {
public:
    using Maps     = std::vector<Map>;
    using Sessions = std::vector<GameSession>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    //
    GameSession* AddSession(const Map* map);

    const Sessions& GetSessions() const noexcept {
        return sessions_;
    }

    GameSession* FindSession(const Map::Id& id) noexcept {
        if (auto it = map_id_to_session_.find(id); it != map_id_to_index_.end()) {
            return &sessions_.at(it->second);
        }
        return nullptr;
    }

    void Tick(uint32_t time_delta);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    //
    Maps         maps_;
    MapIdToIndex map_id_to_index_;
    //
    Sessions     sessions_;
    MapIdToIndex map_id_to_session_;

};

}  // namespace model
