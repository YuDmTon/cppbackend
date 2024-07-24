#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"

namespace model {

//// Aux ///////////////////////////////////////////////////////////////////////////
enum Direction {
    NORTH,          // U
    SOUTH,          // D
    WEST,           // L
    EAST,           // R
};

std::string DirSign(Direction dir);

struct Position {
    double x = 0;
    double y = 0;
};

struct Speed {
    double sx = 0;
    double sy = 0;
};

double GetRandom(double from, double to);

//// Dog ///////////////////////////////////////////////////////////////////////////
class Dog {
public:
    Dog() = default;
    explicit Dog(std::string name, uint32_t id) : name_(name), id_(id) { 
        pos_.x = GetRandom(0, 100);
        pos_.y = GetRandom(0, 100);
    }
    const std::string GetName() const { return name_; }
    const uint32_t GetId() const { return id_; }
    Position GetPosition() const { return pos_; }
    Speed GetSpeed() const { return speed_; }
    std::string GetDir() const { return DirSign(dir_); }
private:
    std::string name_;
    uint32_t    id_;
    //
    Position    pos_{0, 0};
    Speed       speed_{0, 0};
    Direction   dir_{NORTH};
};
/*
//// GameSession ///////////////////////////////////////////////////////////////////
class Map;

class GameSession {
    explicit GameSession(std::shared_ptr<Map> map) : map_(map) { }
    void AddDog(Dog dog) { dogs_[dog.GetId()] = dog; }
private:
    std::shared_ptr<Map>              map_;
    std::unordered_map<uint32_t, Dog> dogs_;
};
*/
//// Model /////////////////////////////////////////////////////////////////////////
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

class Map {
public:
    using Id        = util::Tagged<std::string, Map>;
    using Roads     = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices   = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
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

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Game {
public:
    using Maps = std::vector<Map>;

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

    // TODO !!!
    void SetConfig(const std::string& config) { config_ = config; }
    std::string GetConfig() { return config_; }
private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    //
    std::string config_; // TODO !!!
//    std::vector<GameSession> sessions_;
};

}  // namespace model
