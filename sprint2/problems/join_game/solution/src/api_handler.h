#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/io_context.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "logic.h"
#include "model.h"

namespace http_handler {

using namespace std::literals;

namespace fs    = std::filesystem;

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
namespace sys   = boost::system;

namespace json  = boost::json;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;


struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view APP_JSON   = "application/json"sv;
    constexpr static std::string_view TEXT_HTML  = "text/html"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};


struct Response {
    Response() = delete;
    static StringResponse BadRequest(unsigned http_version, bool keep_alive) {
        return Response::MakeResponse(http::status::bad_request, "Bad request"sv, ContentType::TEXT_PLAIN, ""sv, ""sv, http_version, keep_alive);
    }
    static StringResponse FileNotFound(unsigned http_version, bool keep_alive) {
        return Response::MakeResponse(http::status::not_found, "File not found"sv, ContentType::TEXT_PLAIN, ""sv, ""sv, http_version, keep_alive);
    }
    static StringResponse MapNotFound(unsigned http_version, bool keep_alive) {
        json::object result;
        result["code"]    = "mapNotFound"s;
        result["message"] = "Map not found"s;
        return Response::MakeResponse(http::status::not_found, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    //
    static StringResponse MakeResponse(
            http::status     status,
            std::string_view body,
            std::string_view content_type,
            std::string_view cache_control,
            std::string_view allow,
            unsigned         http_version,
            bool             keep_alive) {
        StringResponse response(status, http_version);
        response.keep_alive(keep_alive);
        //
        response.set(http::field::content_type, content_type);
        if ( !cache_control.empty() ) response.set(http::field::cache_control, cache_control);
        if ( !allow.empty() ) response.set(http::field::allow, allow);
        //
        response.body() = body;
        response.content_length(body.size());
        return response;
    }

};


class ApiHandler {
    // types
    constexpr static std::string_view API     = "/api"sv;
    constexpr static std::string_view MAPS    = "/api/v1/maps"sv;
    constexpr static std::string_view JOIN    = "/api/v1/game/join"sv;
    constexpr static std::string_view PLAYERS = "/api/v1/game/players"sv;
    // others
    constexpr static std::string_view BEARER  = "Bearer "sv;
    constexpr static size_t TOKEN_SIZE = 32;
public:
    explicit ApiHandler(model::Game& game, logic::Players& players)
        : game_(game), players_(players) {
    }

    ApiHandler(const ApiHandler&) = delete;
    ApiHandler& operator=(const ApiHandler&) = delete;

    bool CanAccept(const std::string& target) { return target.find(API) == 0; }

    StringResponse Response(const StringRequest& req) {
        unsigned    http_version = req.version();
        bool        keep_alive   = req.keep_alive();
        std::string target(req.target());
        //
        if ( IsMapRequest(target)) { return MapResponse(req);  }
        if ( IsMapsRequest(target) ) { return MapsResponse(req); }
        if ( IsJoinRequest(target) ) { return JoinResponse(req); }
        if ( IsPlayersRequest(target) ) { return PlayersResponse(req); }
        //
        return Response::BadRequest(http_version, keep_alive);
    }

private:
    std::string GetMap(std::string map_id) {
        auto maps  = json::parse(game_.GetConfig()).as_object().at("maps").as_array();
        auto is_id = [map_id](auto map) { return map_id == map.as_object().at("id").as_string(); };
        if (auto it = std::find_if(maps.begin(), maps.end(), is_id); it != maps.end()) {
            return json::serialize(*it);
        }
        return "";
    }

    bool IsMapRequest(std::string target) { return target.find(MAPS) == 0 && target.size() > MAPS.size() + 1; }
    StringResponse MapResponse(const StringRequest& req) {
        http::status status       = http::status::ok;
        unsigned     http_version = req.version();
        bool         keep_alive   = req.keep_alive();
        //
        std::string  target(req.target());
        std::string  map_id = target.substr(13);
        std::string  config = game_.GetConfig();
        std::string  body   = GetMap(map_id);
        //
        if ( body.empty() ) {
            return Response::MapNotFound(http_version, keep_alive);
        }
        return Response::MakeResponse(status, body, ContentType::APP_JSON, ""sv, ""sv, http_version, keep_alive);
    }
    //
    bool IsMapsRequest(std::string target) { return target == MAPS; }
    StringResponse MapsResponse(const StringRequest& req) {
        http::status status       = http::status::ok;
        unsigned     http_version = req.version();
        bool         keep_alive   = req.keep_alive();
        //
        std::string  config = game_.GetConfig();
        std::string  body;
        //
        auto maps = json::parse(config);
        json::array result;
        for (const auto& map : maps.as_object().at("maps").as_array()) {
            json::object id;
            id["id"]   = map.as_object().at("id");
            id["name"] = map.as_object().at("name");
            result.push_back(id);
        }
        body = json::serialize(result);
        //
        return Response::MakeResponse(status, body, ContentType::APP_JSON, ""sv, ""sv, http_version, keep_alive);
    }
    //
    bool IsJoinRequest(std::string target) { return target == JOIN; }
    StringResponse JoinResponse(const StringRequest& req) {
        http::status status       = http::status::ok;
        unsigned     http_version = req.version();
        bool         keep_alive   = req.keep_alive();
        //
        std::string  user_name, map_id;
        std::string  body = req.body();
        // check method
        if ( req.method() != http::verb::post ) {
            status = http::status::method_not_allowed;
            json::object result;
            result["code"]    = "invalidMethod"s;
            result["message"] = "Only POST method is expected"s;
            return Response::MakeResponse(status, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, "POST"sv, http_version, keep_alive);
        }
        // check request json body
        try {
            json::object json = json::parse(body).as_object();
            std::string  user(json.at("userName").as_string());
            std::string  map(json.at("mapId").as_string());
            user_name  = user;
            map_id     = map;
        } catch (...) {
            status = http::status::bad_request;
            json::object result;
            result["code"]    = "invalidArgument"s;
            result["message"] = "Join game request parse error"s;
            return Response::MakeResponse(status, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
        }
//std::cout << "user_name = '" << user_name << "', map_id = '" << map_id << "'" << std::endl;
        // check userName
        if ( user_name.empty() ) {
            status = http::status::bad_request;
            json::object result;
            result["code"]    = "invalidArgument"s;
            result["message"] = "Invalid name"s;
            return Response::MakeResponse(status, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
        }
        // check mapId
        std::string map = GetMap(map_id);
        if ( map.empty() ) {
            return Response::MapNotFound(http_version, keep_alive);
        }
        // add Player
        auto [token, id] = players_.Add(user_name, map_id);
        json::object result;
        result["authToken"] = token;
        result["playerId"]  = id;
        return Response::MakeResponse(status, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    //
    bool IsPlayersRequest(std::string target) { return target == PLAYERS; }
    StringResponse PlayersResponse(const StringRequest& req) {
        http::status status       = http::status::ok;
        unsigned     http_version = req.version();
        bool         keep_alive   = req.keep_alive();
        // check method
        if ( req.method() != http::verb::get && req.method() != http::verb::head ) {
            status = http::status::method_not_allowed;
            json::object result;
            result["code"]    = "invalidMethod"s;
            result["message"] = "Invalid Method"s;
            return Response::MakeResponse(status, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, "GET, HEAD"sv, http_version, keep_alive);
        }
        // get and check Authorization header
        std::string token;
        for (const auto& header : req) {
            if ( "Authorization" == header.name_string() ) {
                token = header.value();
                break;
            }
        }
//std::cout << "token = '" << token << "'" << std::endl;
        //
        bool is_ok = true;
        if ( token.size() == BEARER.size() + TOKEN_SIZE ) {
            if ( token.find(BEARER) == 0 ) {
                token = token.substr(BEARER.size());
                size_t count = std::count_if(token.begin(), token.end(), [](unsigned char c){ return std::isxdigit(c); } );
//std::cout << "count = " << count << std::endl;
                if ( count != TOKEN_SIZE ) {
                    is_ok = false;
                };
            } else {
                is_ok = false;
            }
        } else {
            is_ok = false;
        }
//std::cout << "ok = " << std::boolalpha << is_ok << std::endl;
        //
        if ( !is_ok ) {
//std::cout << "invalidToken" << std::endl;
            status = http::status::unauthorized;
            json::object result;
            result["code"]    = "invalidToken"s;
            result["message"] = "Authorization header is missing"s;
            return Response::MakeResponse(status, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
        }
        // check game has this token
        if ( !players_.HasToken(token) ) {
//std::cout << "unknownToken" << std::endl;
            status = http::status::unauthorized;
            json::object result;
            result["code"]    = "unknownToken"s;
            result["message"] = "Player token has not been found"s;
            return Response::MakeResponse(status, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
        }
        // get players_list
        auto players = players_.GetPlayers();
        json::object obj;
        for (const auto& player : players) {
            json::object sub_obj;
            sub_obj["name"] = player.GetName();
            obj[std::to_string(player.GetId())] = sub_obj;
        }
//std::cout << "status = " << status << std::endl;
        return Response::MakeResponse(status, json::serialize(obj), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }

private:
    model::Game&    game_;
    logic::Players& players_;
};

}  // namespace http_handler
