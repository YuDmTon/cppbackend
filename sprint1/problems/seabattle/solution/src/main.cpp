#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        while ( !IsGameEnded() ) {
            PrintFields();
            if ( my_initiative ) {
                std::cout << "Your turn: ";
                size_t x, y;
                if ( !SendMove(socket, x, y) ) {
                    std::cout << "Wrong move! Try again." << std::endl;
                    continue;
                } 
                bool pass_move = false;
                if ( !ReadResult(socket, pass_move, x, y) ) {
                    std::cout << "Can read result! Game was stopped." << std::endl;
                    return;
                }
                if ( pass_move ) {
                    my_initiative = false;
                }
            } else {
                std::cout << "Waiting for turn..." << std::endl;
                bool pass_move = false;
                if ( !ReadMove(socket, pass_move) ) {
                    std::cout << "Can read result! Game was stopped." << std::endl;
                    return;
                }
                if ( pass_move ) {
                    my_initiative = true;
                }
            }
        }
    }

private:
    //// my turn //////////////////////////////////////////////////////////////////
    bool SendMove(tcp::socket& socket, size_t& x, size_t& y) {
        std::string move;
        std::getline(std::cin, move);
        auto parsedMove = ParseMove(move);
        if ( parsedMove ) {
            x = parsedMove->second;
            y = parsedMove->first;
            std::cout << "--- SendMove(): " << x << ", " << y << std::endl;
            WriteExact(socket, move);
        } else {
            std::cout << "--- SendMove(): Wrong move! Try again." << std::endl;
            return false;
        }
        return true;
    }

    bool ReadResult(tcp::socket& socket, bool& pass_move, size_t x, size_t y) {
        auto result = ReadExact<1>(socket);
        std::string print_result;
        pass_move = false;
        if ( result ) {
            std::cout << "--- ReadResult(): -> '" << *result << "'" << std::endl;
            switch ( (*result)[0] ) {
                case 'M': 
                    print_result = "Miss!"; 
                    pass_move    = true; 
                    other_field_.MarkMiss(x, y);
                    break;
                case 'K': 
                    print_result = "Kill!"; 
                    other_field_.MarkKill(x, y);
                    break;
                case 'H': 
                    print_result = "Hit!";  
                    other_field_.MarkHit(x, y);
                    break;
                default:
                    std::cout << "--- ReadResult(): unknown result !!!" << std::endl;
                    return false;
            }
            std::cout << print_result << std::endl;
        } else {
            std::cout << "--- ReadResult(): can't read !!!" << std::endl;
            return false;
        }
        return true;
    }

    //// waiting for turn /////////////////////////////////////////////////////////
    bool ReadMove(tcp::socket& socket, bool& pass_move) {
        auto move = ReadExact<2>(socket);
        if ( move ) {
            std::cout << "--- ReadMove(): -> '" << *move << "'" << std::endl;
            auto parsedMove = ParseMove(*move);
            if ( parsedMove ) {
                std::cout << "--- ReadMove(): " << parsedMove->second << ", " << parsedMove->first << std::endl;
                std::cout << "Shot to " << MoveToString(*parsedMove) << std::endl;
                SeabattleField::ShotResult result = my_field_.Shoot(parsedMove->second, parsedMove->first);
                pass_move = result == SeabattleField::ShotResult::MISS;
                SendResult(socket, result);
            } else {
                std::cout << "--- ReadMove(): Wrong move! Try again." << std::endl;
                return false;
            }
        } else {
            std::cout << "--- ReadMove(): can't read !!!" << std::endl;
            return false;
        }
        return true;
    }

    bool SendResult(tcp::socket& socket, SeabattleField::ShotResult result) {
        std::string send_result = "?";
        switch ( result ) {
            case SeabattleField::ShotResult::MISS: send_result = "M"; break;
            case SeabattleField::ShotResult::HIT:  send_result = "H"; break;
            case SeabattleField::ShotResult::KILL: send_result = "K"; break;
        }
        WriteExact(socket, send_result);
        return true;
    }

    //// author ///////////////////////////////////////////////////////////////////
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first + 'A'), static_cast<char>(move.second + '1')};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

//// server ////////////////////////////////////////////////////////////////////////////////////
void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    // создаём север
    net::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;
    //
    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);
    if (ec) {
        std::cout << "Can't accept connection"sv << std::endl;
        return;
    }    

    // начинаем игру
    agent.StartGame(socket, false);
};

//// client ////////////////////////////////////////////////////////////////////////////////////
void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    // создаём клиента
    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str.c_str(), ec), port);
    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        return;
    }
    //
    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);
    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
        return;
    }

    // начинаем игру
    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
