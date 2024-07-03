#include "audio.h"

#include <boost/asio.hpp>
#include <cstring>
#include <iostream>


namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;


void StartServer(uint16_t port) {
    // создаём плейер
    Player player(ma_format_u8, 1);

    // создаём и запускаем UDP сервер
    static const size_t max_buffer_size = 65507;    // MAX UDP packet size
    try {
        boost::asio::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        // Запускаем сервер в цикле, чтобы можно было работать со многими клиентами
        for (;;) {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
            std::array<char, max_buffer_size> recv_buf;
            udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента
            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);
            std::cout << "Receiving done" << std::endl;

            // проигрываем полученный вук
            player.PlayBuffer(recv_buf.data(), recv_buf.size() / player.GetFrameSize(), 1.5s);
            std::cout << "Playing done" << std::endl;
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

void StartClient(uint16_t port) {
    //
    std::string server_ip;
    std::cout << "Point server IP address to record and send message: ";
    std::getline(std::cin, server_ip);

    // создаём рекодер
    Recorder recorder(ma_format_u8, 1);
    // и пишем звук
    auto rec_result = recorder.Record(65000, 1.5s);
    std::cout << "Recording done" << std::endl;
    
    // и отправляем звук по UDP
    try {
        net::io_context io_context;

        // Перед отправкой данных нужно открыть сокет. 
        // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
        udp::socket socket(io_context, udp::v4());

        boost::system::error_code ec;
        auto endpoint = udp::endpoint(net::ip::make_address(server_ip, ec), port);
        socket.send_to(net::buffer(rec_result.data.data(), rec_result.frames * recorder.GetFrameSize()), endpoint);
        std::cout << "Sending done" << std::endl;

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    
}

int main(int argc, char** argv) {
    // проверяем параметры запуска 
    if (argc != 3) {
        std::cout << "Usage: "sv << argv[0] << " <server|clent> <port>"sv << std::endl;
        return 1;
    }
    if ( std::strcmp(argv[1], "client") && std::strcmp(argv[1], "server") ) {
        std::cout << "First parameter mustbe 'client' or 'server'" << std::endl;
        return 2;
    }
    int port = 0;
    try {
        port = std::stoi(argv[2]);
    } catch (...) {
        std::cout << "Port isn,t number or too big/small number" << std::endl;
        return 3;
    }
    if ( port < 1024 || port > 65535 ) {
        std::cout << "Port is out of 1024 ... 65535 range" << std::endl;
        return 4;
    }

    //
    if ( !std::strcmp(argv[1], "client") ) {
        StartClient(port);
    }
    //
    if ( !std::strcmp(argv[1], "server") ) {
        StartServer(port);
    }
    //
    return 0;
}
