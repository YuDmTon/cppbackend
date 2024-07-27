#pragma once

#include "sdk.h"

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/date_time.hpp>

#include <iostream>

#include "logger.h"

namespace http_server {

using namespace std::literals;

namespace net   = boost::asio;
namespace sys   = boost::system;
using     tcp   = net::ip::tcp;
namespace beast = boost::beast;
namespace http  = beast::http;

void ReportError(beast::error_code ec, std::string_view what);


//// SessionBase ///////////////////////////////////////////////////////////////////////////////////
class SessionBase {
public:
    // Запрещаем копирование и присваивание объектов SessionBase и его наследников
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;
    // асинхронный запуск сессии
    void Run() {
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));    
    }

protected:
    using HttpRequest = http::request<http::string_body>;

    explicit SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) { }
    ~SessionBase() = default;

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
//        std::cout << "--- Write() ---" << std::endl;
        std::string_view content_type = "null"s;
        for (const auto& header : response) {
//            std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
            if ( header.name_string() == "Content-Type" ) {
                content_type = header.value();
                break;
            }
        }
        // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
    
        //
        boost::posix_time::ptime res_time = boost::posix_time::microsec_clock::local_time();
        logger::LogResponse((res_time - req_time_).total_milliseconds(), 
                                  safe_response->result_int(),
                                  content_type);
        //
        
        auto self = GetSharedThis();
        http::async_write(stream_, *safe_response,
                        [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                            self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                        });
    }

private:
    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);
    // асинхронное чтение запроса
    void Read();
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
    void Close();
    // Обработку запроса делегируем подклассу
    virtual void HandleRequest(HttpRequest&& request) = 0;
    // Получение указателя на себя делегируем подклассу
    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

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
/*
    template <typename Body, typename Fields>
    std::string_view GetContentType(http::response<Body, Fields> response) {
        std::cout << "--- GetContentType() ---" << std::endl;
        for (const auto& header : response) {
            std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
            if ( header.name_string() == "Content-Type" ) {
                return header.value();
            }
        }
        return "null"sv;
    }
*/
    
private:
    beast::tcp_stream  stream_; // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
    beast::flat_buffer buffer_;
    HttpRequest        request_;
    boost::posix_time::ptime req_time_;

};


//// Session ///////////////////////////////////////////////////////////////////////////////////////
template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : SessionBase(std::move(socket))
        , request_handler_(std::forward<Handler>(request_handler)) {
    }

private:
    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    } 

    void HandleRequest(HttpRequest&& request) override {
        request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
            self->Write(std::move(response));
        });
    }    

private:
    RequestHandler request_handler_;
};


//// Listener //////////////////////////////////////////////////////////////////////////////////////
template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
            : ioc_(ioc)
            , acceptor_(net::make_strand(ioc))      // Обработчики асинхронных операций acceptor_ будут вызываться в своём strand
            , request_handler_(std::forward<Handler>(request_handler)) {
        acceptor_.open(endpoint.protocol());        // Открываем acceptor, используя протокол (IPv4 или IPv6), указанный в endpoint
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);                   // Привязываем acceptor к адресу и порту endpoint
        acceptor_.listen(net::socket_base::max_listen_connections);
    }

    void Run() {
        DoAccept();
    }

private:
    void AsyncRunSession(tcp::socket&& socket) {
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
    }
    
    void DoAccept() {
        acceptor_.async_accept(
            net::make_strand(ioc_), // это последовательный (strand !!!) испонитель
            // вместо лямбды [](beast::error_code ec, tcp::socket socket) {} напишем bind_front_handler()
            // вызовет self->OnAccept(ec, socket)
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()) 
        );
    }

    // Метод socket::async_accept создаст сокет и передаст его в OnAccept
    void OnAccept(sys::error_code ec, tcp::socket socket) {
        using namespace std::literals;

        if (ec) {
//            return ReportError(ec, "accept"sv);
            return logger::LogNetError(ec.value(), ec.message(), "accept"sv);
        }

        // Асинхронно обрабатываем сессию
        AsyncRunSession(std::move(socket));

        // Принимаем новое соединение
        DoAccept();
    }

private:
    net::io_context& ioc_;
    tcp::acceptor    acceptor_;
    RequestHandler   request_handler_;
};


//// ServeHttp /////////////////////////////////////////////////////////////////////////////////////
template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;
    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}

}  // namespace http_server
