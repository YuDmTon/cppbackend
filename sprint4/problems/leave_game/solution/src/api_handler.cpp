#include "api_handler.h"

#include <algorithm>

namespace http_handler {

std::string MethodToString(http::verb verb) {
    switch ( verb ) {
        case http::verb::get:  return "GET";
        case http::verb::head: return "HEAD";
        case http::verb::post: return "POST";
        case http::verb::put:  return "PUT";
    }
    assert(false);
    return "UNKNOWN";
}


StringResponse ApiHandler::Response(const StringRequest& req) {
    unsigned    http_version = req.version();
    bool        keep_alive   = req.keep_alive();
    std::string target(req.target());
    //
    if ( IsMapRequest(target))      { return MapResponse(req);     }
    if ( IsMapsRequest(target) )    { return MapsResponse(req);    }
    if ( IsJoinRequest(target) )    { return JoinResponse(req);    }
    if ( IsPlayersRequest(target) ) { return PlayersResponse(req); }
    if ( IsStateRequest(target) )   { return StateResponse(req);   }
    if ( IsMoveRequest(target) )    { return MoveResponse(req);    }
    if ( IsTickRequest(target) )    { return TickResponse(req);    }
    if ( IsRecordsRequest(target) ) { return RecordsResponse(req); }
    //
    return Response::BadRequest("badRequest"s, "Unknown API target"s, http_version, keep_alive);
}

bool ApiHandler::CheckToken(const StringRequest& req, std::string& token) {
    auto header = std::find_if(req.begin(), req.end(), [](const auto& header){ return "Authorization" == header.name_string() || "authorization" == header.name_string(); } );
    if ( header != req.end() ) {
        token = header->value();
    }
    //
    bool is_ok = true;
    if ( token.size() == BEARER.size() + TOKEN_SIZE ) {
        if ( token.starts_with(BEARER) ) {
            token = token.substr(BEARER.size());
            auto xdigits_count = std::count_if(token.begin(), token.end(), [](unsigned char c){ return std::isxdigit(c); } );
            if ( xdigits_count != TOKEN_SIZE ) {
                is_ok = false;
            }
        } else {
            is_ok = false;
        }
    } else {
        is_ok = false;
    }
    return is_ok;
}

// --- Map by Id
StringResponse ApiHandler::MapResponse(const StringRequest& req) {
    unsigned    http_version = req.version();
    bool        keep_alive   = req.keep_alive();
    // check method
    if ( req.method() != http::verb::get && req.method() != http::verb::head ) {
        return Response::InvalidMethod("invalidMethod"s, "Only GET method is expected"s, "GET, HEAD"s, http_version, keep_alive);
    }
    //
    std::string target(req.target());
    std::string map_id       = target.substr(13);
    // do
    std::optional<std::string> res_body = app_.GetMap(map_id);
    if ( res_body ) {
        return Response::MakeResponse(http::status::ok, *res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    return Response::NotFound("mapNotFound"s, "map not found"s, http_version, keep_alive);
}
// --- All maps
StringResponse ApiHandler::MapsResponse(const StringRequest& req) {
    unsigned http_version = req.version();
    bool     keep_alive   = req.keep_alive();
    // do
    std::string res_body;
    app_.GetMaps(res_body);
    return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
}
// --- Join
StringResponse ApiHandler::JoinResponse(const StringRequest& req) {
    unsigned    http_version = req.version();
    bool        keep_alive   = req.keep_alive();
    // check method
    if ( req.method() != http::verb::post ) {
        return Response::InvalidMethod("invalidMethod"s, "Only POST method is expected"s, "POST"s, http_version, keep_alive);
    }
    // try parse request json body
    std::string user_name, map_id;
    std::string req_body = req.body();
    try {
        json::object json = json::parse(req_body).as_object();
        std::string  user(json.at("userName").as_string());
        std::string  map(json.at("mapId").as_string());
        user_name  = user;
        map_id     = map;
    } catch ( std::exception& ) {
        return Response::BadRequest("invalidArgument"s, "Join game request parse error"s, http_version, keep_alive);
    }
    // check userName
    if ( user_name.empty() ) {
        return Response::BadRequest("invalidArgument"s, "Invalid name"s, http_version, keep_alive);
    }
    // try join to game
    std::string res_body;
    if ( app_.TryJoin(user_name, map_id, res_body) ) {
        return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    return Response::NotFound("mapNotFound", "Map not found", http_version, keep_alive);
}
// --- Players
StringResponse ApiHandler::PlayersResponse(const StringRequest& req) {
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
    // do
    std::string res_body;
    if ( app_.GetPlayers(token, res_body) ) {
        return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    return Response::Unauthorized("unknownToken"s, "Player token has not been found"s, http_version, keep_alive);
}
// --- State
StringResponse ApiHandler::StateResponse(const StringRequest& req) {
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
    // do
    std::string res_body;
    if ( app_.GetState(token, res_body) ) {
        return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    return Response::Unauthorized("unknownToken"s, "Player token has not been found"s, http_version, keep_alive);
}
// --- Move
StringResponse ApiHandler::MoveResponse(const StringRequest& req) {
    unsigned     http_version = req.version();
    bool         keep_alive   = req.keep_alive();
    // check method
    if ( req.method() != http::verb::post ) {
        return Response::InvalidMethod("invalidMethod"s, "Only POST method is expected"s, "GET, HEAD"s, http_version, keep_alive);
    }
    // try parse request json body
    std::string move;
    std::string req_body = req.body();
    try {
        json::object json = json::parse(req_body).as_object();
        std::string  str(json.at("move").as_string());
        move = str;
    } catch ( std::exception& ) {
        return Response::BadRequest("invalidArgument"s, "No move feild"s, http_version, keep_alive);
    }
    if ( move != "U"s && move != "D"s && move != "R"s && move != "L"s && move != ""s  ) {
        return Response::BadRequest("invalidArgument"s, "Unknown move feild"s, http_version, keep_alive);
    }
    // get and check Authorization header
    std::string token;
    if ( !CheckToken(req, token) ) {
        return Response::Unauthorized("invalidToken"s, "Authorization header is missing or wrong"s, http_version, keep_alive);
    }
    // do
    std::string res_body;
    if ( app_.Move(token, move, res_body) ) {
        return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    return Response::Unauthorized("unknownToken"s, "Player token has not been found"s, http_version, keep_alive);
}
// --- Tick
StringResponse ApiHandler::TickResponse(const StringRequest& req) {
    unsigned     http_version = req.version();
    bool         keep_alive   = req.keep_alive();
    // check debug model
    if ( !debug_mode_ ) {
        return Response::BadRequest("badRequest"s, "Server isn't in debug mode"s, http_version, keep_alive);
    }
    // check method
    if ( req.method() != http::verb::post ) {
        return Response::InvalidMethod("invalidMethod"s, "Only POST method is expected"s, "POST"s, http_version, keep_alive);
    }
    // try parse request json body
    uint32_t time_delta = 0;
    std::string req_body = req.body();
    try {
        json::object json = json::parse(req_body).as_object();
        time_delta = json.at("timeDelta").as_int64();
    } catch ( std::exception& ) {
        return Response::BadRequest("invalidArgument"s, "No timeDelta feild"s, http_version, keep_alive);
    }
    // do
    return Response::MakeResponse(http::status::ok, app_.Tick(time_delta), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
}
// --- Results
StringResponse ApiHandler::RecordsResponse(const StringRequest& req) {
    unsigned     http_version = req.version();
    bool         keep_alive   = req.keep_alive();
    // check method
    if ( req.method() != http::verb::get ) {
        return Response::InvalidMethod("invalidMethod"s, "Only GET method is expected"s, "GET"s, http_version, keep_alive);
    }
    // do
    std::string res_body;
    app_.GetRecords(res_body);
    return Response::MakeResponse(http::status::ok, res_body, ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
}

}  // namespace http_handler
