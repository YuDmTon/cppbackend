#pragma once

#include "app.h"
#include "model.h"
#include "response.h"

namespace http_handler {


class ApiHandler {
    // requesta to api
    constexpr static std::string_view API     = "/api"sv;
    constexpr static std::string_view MAPS    = "/api/v1/maps"sv;
    constexpr static std::string_view JOIN    = "/api/v1/game/join"sv;
    constexpr static std::string_view PLAYERS = "/api/v1/game/players"sv;
    constexpr static std::string_view STATE   = "/api/v1/game/state"sv;
    // others
    constexpr static std::string_view BEARER  = "Bearer "sv;
    constexpr static size_t TOKEN_SIZE = 32;
public:
    explicit ApiHandler(app::Application& app)
        : app_(app) {
    }

    ApiHandler(const ApiHandler&) = delete;
    ApiHandler& operator=(const ApiHandler&) = delete;

    bool CanAccept(const std::string& target) { return target.find(API) == 0; }

    StringResponse Response(const StringRequest& req) {
        unsigned    http_version = req.version();
        bool        keep_alive   = req.keep_alive();
        std::string target(req.target());
        //
        if ( IsMapRequest(target))      { return MapResponse(req);  }
        if ( IsMapsRequest(target) )    { return MapsResponse(req); }
        if ( IsJoinRequest(target) )    { return JoinResponse(req); }
        if ( IsPlayersRequest(target) ) { return PlayersResponse(req); }
        if ( IsStateRequest(target) )   { return StateResponse(req); }
        //
        return Response::BadRequest(http_version, keep_alive);
    }

private:
    std::string MethodToString(http::verb verb) {
        switch ( verb ) {
            case http::verb::get:  return "GET";
            case http::verb::head: return "HEAD";
            case http::verb::post: return "POST";
            case http::verb::put:  return "PUT";
        }
        return "UNKNOWN";
    }

    bool CheckToken(const StringRequest& req, std::string& token) {
        for (const auto& header : req) {
            if ( "Authorization" == header.name_string() || "authorization" == header.name_string() ) {
                token = header.value();
                break;
            }
        }
        //
        bool is_ok = true;
        if ( token.size() == BEARER.size() + TOKEN_SIZE ) {
            if ( token.find(BEARER) == 0 ) {
                token = token.substr(BEARER.size());
                size_t count = std::count_if(token.begin(), token.end(), [](unsigned char c){ return std::isxdigit(c); } );
/*
                if ( count != TOKEN_SIZE ) {
                    is_ok = false;

                };
*/
            } else {
                is_ok = false;
            }
        } else {
            is_ok = false;
        }
        return is_ok;
    }

private:
    bool IsMapRequest(std::string target) { return target.find(MAPS) == 0 && target.size() > MAPS.size() + 1; }
    StringResponse MapResponse(const StringRequest& req) {
        unsigned    http_version = req.version();
        bool        keep_alive   = req.keep_alive();
        std::string target(req.target());
        std::string map_id       = target.substr(13);
        //
        std::string res_body;
        if ( app_.GetMap(map_id, res_body) ) {
            return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, ""sv, ""sv, http_version, keep_alive);
        } else {
            return Response::NotFound("mapNotFound"s, "map not found"s, http_version, keep_alive);
        }
    }
    //
    bool IsMapsRequest(std::string target) { return target == MAPS; }
    StringResponse MapsResponse(const StringRequest& req) {
        unsigned http_version = req.version();
        bool     keep_alive   = req.keep_alive();
        //
        std::string res_body;
        app_.GetMaps(res_body);
        return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, ""sv, ""sv, http_version, keep_alive);
    }
    //
    bool IsJoinRequest(std::string target) { return target == JOIN; }
    StringResponse JoinResponse(const StringRequest& req) {
        unsigned    http_version = req.version();
        bool        keep_alive   = req.keep_alive();
        //
        std::string user_name, map_id;
        std::string req_body = req.body();
        // check method
        if ( req.method() != http::verb::post ) {
            return Response::InvalidMethod("invalidMethod"s, "Only POST method is expected"s, "POST"s, http_version, keep_alive);
        }
        // try parse request json body
        try {
            json::object json = json::parse(req_body).as_object();
            std::string  user(json.at("userName").as_string());
            std::string  map(json.at("mapId").as_string());
            user_name  = user;
            map_id     = map;
        } catch (...) {
            return Response::BadRequest("invalidArgument"s, "Join game request parse error"s, http_version, keep_alive);
        }
//std::cout << "user_name = '" << user_name << "', map_id = '" << map_id << "'" << std::endl;
        // check userName
        if ( user_name.empty() ) {
            return Response::BadRequest("invalidArgument"s, "Invalid name"s, http_version, keep_alive);
        }
        // try join to game
        std::string res_body;
        if ( app_.TryJoin(user_name, map_id, res_body) ) {
            return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
        } else {
            return Response::NotFound("mapNotFound", "Map not found", http_version, keep_alive);
        }
    }
    //
    bool IsPlayersRequest(std::string target) { return target == PLAYERS; }
    StringResponse PlayersResponse(const StringRequest& req) {
        unsigned     http_version = req.version();
        bool         keep_alive   = req.keep_alive();
        // check method
        if ( req.method() != http::verb::get && req.method() != http::verb::head ) {
            return Response::InvalidMethod("invalidMethod"s, "Only GET & HEAD methods are expected"s, "GET, HEAD"s, http_version, keep_alive);
        }
        // get and check Authorization header
        std::string token;
        if ( !CheckToken(req, token) ) {
            return Response::Unauthorized("invalidToken"s, "Authorization header is missing or wrong"s, http_version, keep_alive);
        }
        std::string res_body;
        if ( app_.GetPlayers(token, res_body) ) {
            return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
        } else {
            return Response::Unauthorized("unknownToken"s, "Player token has not been found"s, http_version, keep_alive);
        }
    }
    //
    bool IsStateRequest(std::string target) { return target == STATE; }
    StringResponse StateResponse(const StringRequest& req) {
        unsigned     http_version = req.version();
        bool         keep_alive   = req.keep_alive();
        // check method
//std::cout << "Method = '" << MethodToString(req.method()) << "'" << std::endl;
        if ( req.method() != http::verb::get && req.method() != http::verb::head ) {
            return Response::InvalidMethod("invalidMethod"s, "Only GET & HEAD methods are expected"s, "GET, HEAD"s, http_version, keep_alive);
        }
        // get and check Authorization header
        std::string token;
        if ( !CheckToken(req, token) ) {
            return Response::Unauthorized("invalidToken"s, "Authorization header is missing or wrong"s, http_version, keep_alive);
        }
//std::cout << "token = '" << token << "'" << std::endl;
        std::string res_body;
        if ( app_.GetState(token, res_body) ) {
            return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
        } else {
            return Response::Unauthorized("unknownToken"s, "Player token has not been found"s, http_version, keep_alive);
        }
    }

private:
    app::Application& app_;
};

}  // namespace http_handler
