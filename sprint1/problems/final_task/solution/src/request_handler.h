#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "json_loader.h"

#include "http_server.h"
#include "model.h"

namespace http_handler {

using namespace std::literals;
namespace beast = boost::beast;
namespace http  = beast::http;

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
        // Обработать запрос request и отправить ответ, используя send
        const auto text_response = [&req, this](http::status status, std::string_view text) {
            return this->MakeStringResponse(status, text, req.version(), req.keep_alive());
        };
        //
        std::string  body;
        std::string  target(req.target());
        http::status status = http::status::ok;
        auto pos = target.find("/api/v1/maps");
        if ( pos == std::string::npos || pos != 0 || target.size() == 13 ) {
            status = http::status::bad_request;
            body   = json_loader::BadRequest();
            send(text_response(status, body));
            return;
        }
        if ( target == "/api/v1/maps" ) {
            body   = json_loader::GetIdList(game_.json_str);
            send(text_response(status, body));
            return;
        }
        body = json_loader::GetMap(game_.json_str, target.substr(13));
        if ( body.empty() ) {
            status = http::status::not_found;
            body = json_loader::NotFound();
        }
        send(text_response(status, body));
    }

private:
    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                    bool keep_alive,
                                    std::string_view content_type = ContentType::APP_JSON) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    model::Game& game_;
};

}  // namespace http_handler
