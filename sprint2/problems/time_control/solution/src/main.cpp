#include "sdk.h"
//

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <filesystem>
#include <iostream>
#include <thread>

#include "app.h"
#include "json_loader.h"
#include "logger.h"
#include "request_handler.h"

using namespace std::literals;

namespace fs  = std::filesystem;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Получает, преобразует при необходимости и проверяет пути 
fs::path GetAndCheckPath(const std::string& path_str, bool is_dir) {
    //
    fs::path srvr_path = fs::current_path();
    srvr_path += "/";
    //
    std::string path(path_str);
    if ( path[path.size() - 1] == '/' ) {
        path = path.substr(0, path.size() - 1);
    }
    fs::path game_path = path;
    //
    if ( game_path.string()[0] != '/' ) {
        game_path  = srvr_path;
        game_path += path_str;
        game_path  = fs::weakly_canonical(game_path);
    }
    //
    if ( !fs::exists(game_path) ) {
        std::string err = "Path '" + game_path.string() + "' doen't exist";
        throw std::runtime_error(err);
    }
    if ( is_dir && !fs::is_directory(game_path) ) {
        std::string err = "Path '" + game_path.string() + "' isn't directory";
        throw std::runtime_error(err);
    }
    //
    return game_path;
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <game-static-content>"sv << std::endl;
        return EXIT_FAILURE;
    }
    //
    logger::LogInit();
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(GetAndCheckPath(argv[1], false));
        fs::path    root = GetAndCheckPath(argv[2], true);
        app::Application app(game);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto api_strand = net::make_strand(ioc);
        http_handler::RequestHandler handler{api_strand, app, root};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
//        std::cout << "Server has started..."sv << std::endl;
        logger::LogStart(address.to_string(), port); 

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        logger::LogStop(); 
    } catch (const std::exception& ex) {
//        std::cerr << "game_server excehtion: " << ex.what() << std::endl;
        logger::LogStop(EXIT_FAILURE, ex.what()); 
        return EXIT_FAILURE;
    }
}
