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
    // Возвращает true, если каталог path содержится внутри base.
    bool IsSubPath(fs::path path);

    std::string GetLowCaseExtention(fs::path path);

    std::string GetMime(fs::path path);

    void DumpRequest(const StringRequest& req);
    void DumpResponse(const StringResponse& res);

private:
    Strand          api_strand_;
    ApiHandler      api_;
    const fs::path& root_;
};

}  // namespace http_handler
