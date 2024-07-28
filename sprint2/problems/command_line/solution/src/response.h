#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/io_context.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>

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
        json::object result;
        result["code"]    = "badRequest";
        result["message"] = "Bad request";
        return Response::MakeResponse(http::status::bad_request, json::serialize(result), ContentType::APP_JSON, ""sv, ""sv, http_version, keep_alive);
    }
    static StringResponse FileNotFound(unsigned http_version, bool keep_alive) {
        return Response::MakeResponse(http::status::not_found, "File not found"sv, ContentType::TEXT_PLAIN, ""sv, ""sv, http_version, keep_alive);
    }
    //
    static StringResponse NotFound(std::string code, std::string message, unsigned http_version, bool keep_alive) {
        json::object result;
        result["code"]    = code;
        result["message"] = message;
        return Response::MakeResponse(http::status::not_found, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    //
    static StringResponse InvalidMethod(std::string code, std::string message, std::string allowed, unsigned http_version, bool keep_alive) {
        json::object result;
        result["code"]    = code;
        result["message"] = message;
        return Response::MakeResponse(http::status::method_not_allowed, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, allowed, http_version, keep_alive);
    }
    //
    static StringResponse BadRequest(std::string code, std::string message, unsigned http_version, bool keep_alive) {
        json::object result;
        result["code"]    = code;
        result["message"] = message;
        return Response::MakeResponse(http::status::bad_request, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
    }
    static StringResponse Unauthorized(std::string code, std::string message, unsigned http_version, bool keep_alive) {
        json::object result;
        result["code"]    = code;
        result["message"] = message;
        return Response::MakeResponse(http::status::unauthorized, json::serialize(result), ContentType::APP_JSON, "no-cache"sv, ""sv, http_version, keep_alive);
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

}  // namespace http_handler
