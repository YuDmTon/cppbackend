#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast.hpp>
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

#include "json_loader.h"

#include "http_server.h"
#include "model.h"

namespace http_handler {

using namespace std::literals;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace sys   = boost::system;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APP_JSON  = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};


class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        //
        //DumpRequest(req);
        //
        http::status status       = http::status::ok;
        std::string  body;
        unsigned     http_version = req.version();
        bool         keep_alive   = req.keep_alive();
        std::string_view  content_type = ContentType::APP_JSON;
        //
        std::string  target(req.target());
//std::cout << "Target = '" << target << "'" << std::endl;            
        auto pos = target.find("/api");
        if ( pos == 0 ) {   // request to API /////////////////////////////////
            if ( target == "/api/v1/maps" ) {
                body   = json_loader::GetIdList(game_.GetJsonStr());
            } else {
                pos = target.find("/api/v1/maps");
                if ( pos == 0 && target.size() > 13 ) {
                    body = json_loader::GetMap(game_.GetJsonStr(), target.substr(13));
                    if ( body.empty() ) {
                        status = http::status::not_found;
                        body = json_loader::NotFound();
                    }
                }
                else {
                    status = http::status::bad_request;
                    body   = json_loader::BadRequest();
                }
            }
        } else {            // GET static content /////////////////////////////
            std::string wanted_file = game_.GetRootDir() + target;
            if ( target == "/" ) {
                wanted_file += "index.html";
            }
//std::cout << "Wanted file = '" << wanted_file << "'" << std::endl;
            if ( !IsSubPath(wanted_file, game_.GetRootDir()) ) {
                status = http::status::bad_request;
                body   = json_loader::BadRequest();
            } else if ( !std::filesystem::exists(wanted_file) ) {
                status = http::status::not_found;
                body = json_loader::NotFound();
            } else {
                content_type = GetMime(wanted_file);
                body         = "?!";
                /////
                http::response<http::file_body> res;
                res.version(11);  // HTTP/1.1
                res.result(http::status::ok);
                res.insert(http::field::content_type, GetMime(wanted_file));
            
                http::file_body::value_type file;
                if (sys::error_code ec; file.open(wanted_file.c_str(), beast::file_mode::read, ec), ec) {
                    std::string err = "Can't open file '" + wanted_file + "'";
                    throw std::runtime_error(err);
                }
            
                res.body() = std::move(file);
                // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
                // в зависимости от свойств тела сообщения
                res.prepare_payload();
                send(res);
                return;
            }
        }
        StringResponse response = MakeStringResponse(status, body, http_version, keep_alive, content_type);
        //
        //DumpResponse(response);
        //
        send(response);
    }

private:
    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status     status, 
                                      std::string_view body, 
                                      unsigned         http_version,
                                      bool             keep_alive,
                                      std::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    // Возвращает true, если каталог path содержится внутри base.
    bool IsSubPath(fs::path path, fs::path base) {
    //    std::cout << "base before: " << base.string() << std::endl;
    //    std::cout << "path before: " << path.string() << std::endl;
    
        // Приводим оба пути к каноничному виду (без . и ..)
        base = fs::weakly_canonical(base);
        path = fs::weakly_canonical(path);
    
    //    std::cout << "base after:  " << base.string() << std::endl;
    //    std::cout << "path after:  " << path.string() << std::endl;
    
        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

    std::string GetLowCaseExtention(fs::path path) {
        std::string ext = path.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
        return ext;
    }
    
    std::string GetMime(fs::path path) {
        std::string ext = GetLowCaseExtention(path);
        //
        if ( ext == ".htm" || ext == ".html" ) return "text/html";
        if ( ext == ".css" )  return "text/css";
        if ( ext == ".txt" )  return "text/plain";
        if ( ext == ".js" )   return "text/javascript";
        if ( ext == ".json" ) return "application/json";
        if ( ext == ".xml" )  return "application/xml";
        if ( ext == ".png" )  return "image/png";
        if ( ext == ".jpg" || ext == ".jpe" || ext == ".jpeg" ) return "image/jpeg";
        if ( ext == ".gif" )  return "image/gif";
        if ( ext == ".bmp" )  return "image/bmp";
        if ( ext == ".ico" )  return "image/vnd.microsoft.icon";
        if ( ext == ".tif" || ext == ".tiff" ) return "image/tiff";
        if ( ext == ".svg" || ext == ".svgz" ) return "image/svg+xml";
        if ( ext == ".mp3" )  return "audio/mpeg";
        //
        return "application/octet-stream";
    }

    void DumpRequest(const StringRequest& req) {
        std::cout << ">>> DumpRequest():" << std::endl;
        std::cout << req.method_string() << ' ' << req.target() << std::endl;
        // Выводим заголовки запроса
        for (const auto& header : req) {
            std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
        }
        std::cout << std::endl;
    }
    void DumpResponse(const StringResponse& res) {
        std::cout << ">>> DumpResponse():" << std::endl;
    //    std::cout << res.method_string() << ' ' << res.target() << std::endl;
        // Выводим заголовки запроса
        for (const auto& header : res) {
            std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
        }
        std::cout << res.body();
        std::cout << std::endl;
    }

private:
    model::Game& game_;
};

}  // namespace http_handler
