#pragma once

#include "api_handler.h"
#include "app.h"
#include "http_server.h"

namespace http_handler {

class RequestHandler : public std::enable_shared_from_this<RequestHandler>  {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    explicit RequestHandler(Strand api_strand, app::Application& app, const fs::path& root, bool debug_mode)
            : api_strand_(api_strand)
            , api_(app)
            , root_(root) {
        api_.SetDebugMode(debug_mode);
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        unsigned    http_version = req.version();
        bool        keep_alive   = req.keep_alive();
        std::string target(req.target());
        //
        StringResponse response;
        if ( api_.CanAccept(target) ) { // request to API /////////////////////////////////
            response = api_.Response(req);  // without strand
        } else {                        // get static content /////////////////////////////
            std::string wanted_file = root_.string() + target;
            if ( target == "/" ) {
                wanted_file += "index.html";
            }
            //
            if ( !IsSubPath(wanted_file) ) {
                response = Response::BadRequest(http_version, keep_alive);
            } else if ( !std::filesystem::exists(wanted_file) ) {
                response = Response::FileNotFound(http_version, keep_alive);
            } else {
                http::response<http::file_body> response;
                response.version(11);  // HTTP/1.1
                response.result(http::status::ok);
                response.insert(http::field::content_type, GetMime(wanted_file));

                http::file_body::value_type file;
                if (sys::error_code ec; file.open(wanted_file.c_str(), beast::file_mode::read, ec), ec) {
                    std::string err = "Can't open file '" + wanted_file + "'";
                    throw std::runtime_error(err);
                }

                response.body() = std::move(file);
                // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
                // в зависимости от свойств тела сообщения
                response.prepare_payload();
                send(response);
                return;
            }
        }
        //
        send(response);
    }

private:
    ////
    StringResponse HandleApiRequest(const StringRequest& req) {
        StringResponse response(http::status::ok, 11);
        return response;
    }

    StringResponse ReportServerError(unsigned version, bool keep_alive) {
        StringResponse response(http::status::ok, 11);
        return response;
    }
    ////

    // Возвращает true, если каталог path содержится внутри base.
    bool IsSubPath(fs::path path) {
        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = root_.begin(), p = path.begin(); b != root_.end(); ++b, ++p) {
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
        // Выводим заголовки запроса
        for (const auto& header : res) {
            std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
        }
        std::cout << res.body() << std::endl;
        std::cout << std::endl;
    }

private:
    Strand          api_strand_;
    ApiHandler      api_;
    const fs::path& root_;
};

}  // namespace http_handler
