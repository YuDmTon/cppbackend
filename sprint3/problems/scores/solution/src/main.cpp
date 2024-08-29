#include "sdk.h"
//
#include <boost/program_options.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
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

// часы сервера
class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
        : strand_{strand}
        , period_{period}
        , handler_{std::move(handler)} {
    }

    void Start() {
        net::dispatch(strand_, [this, self = shared_from_this()] {
            last_tick_ = Clock::now();
            self->ScheduleTick();
        });
    }

private:
    void ScheduleTick() {
        assert(strand_.running_in_this_thread());
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
        });
    }

    void OnTick(sys::error_code ec) {
        using namespace std::chrono;
        assert(strand_.running_in_this_thread());

        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            try {
                handler_(delta);
//std::cout << "Tick !" << std::endl;
            } catch (...) {
            }
            ScheduleTick();
        }
    }

    using Clock = std::chrono::steady_clock;

    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};

// Параметры программы
struct Args {
    uint32_t    time_delta;
    std::string config_file;
    std::string www_root;
    bool        randomize;
};

// Парсим командную строку
[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"All options"s};

    // Готовим описание всех параметров программы
    Args args;
    desc.add_options()
        ("help,h",        "produce help message")
        ("tick-period,t", po::value(&args.time_delta)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file)->value_name("file"s),   "set config file path")
        ("www-root,w",    po::value(&args.www_root)->value_name("dir"s),      "set static files root")
        ("randomize-spawn-points,r", po::value(&args.randomize),     "spawn dogs at random positions");

    // Парсим командную строку
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Проверяем найденные параметры
    if ( vm.contains("help"s) ) {
        std::cout << desc;
        return std::nullopt;
    }

    if ( !vm.contains("config-file"s) ) {
        throw std::runtime_error("config-file is not specified"s);
    }
    if ( !vm.contains("www-root"s) ) {
        throw std::runtime_error("www-root is not specified"s);
    }

    if ( !vm.contains("tick-period"s) ) {
        std::cout << "Debug mode"s << std::endl;
        args.time_delta = 0;
    }

    if ( !vm.contains("randomize-spawn-points"s) ) {
        std::cout << "Not randomize spawn mode"s << std::endl;
        args.randomize = false;
    }

    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}



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
    //
    logger::LogInit();
    try {
        // 0. парсим командную строку
        auto args = ParseCommandLine(argc, argv);
        if ( !args ) {
            return EXIT_SUCCESS;
        }
//std::cout << "Command line arguments: tick = " << args->time_delta << ", config_file = '" << args->config_file << "', www_root = '" << args->www_root << ", randomize = " << std::boolalpha << args->randomize << std::endl;

        // 1. Загружаем карту из файла, строим модель игры, создаём приложение
        model::Game game = json_loader::LoadGame(GetAndCheckPath(args->config_file, false));
        fs::path    root = GetAndCheckPath(args->www_root, true);
        bool  debug_mode = args->time_delta == 0 ? true : false;
        app::Application app(game, debug_mode, args->randomize);

        // 2. Инициализируем io_context и создаём strand
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        auto api_strand = net::make_strand(ioc);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{api_strand, app, root, debug_mode};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // 6. Если не в отладочном режиме, запускаем время
        if ( !debug_mode ) {
           auto ticker = std::make_shared<Ticker>(api_strand, std::chrono::milliseconds(args->time_delta),
                [&app](std::chrono::milliseconds delta) { app.Tick(delta.count()); }
            );
            ticker->Start();
        }

        // 7. Запускаем обработку асинхронных операций
        logger::LogStart(address.to_string(), port);
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
