#pragma comment(lib, "ws2_32.lib")
#define ASIO_STANDALONE 
#include <iostream>
#include <asio.hpp>
#include "network/TCPListener.hpp"
#include "network/UDPListener.hpp"
#include "network/Session.hpp"

using json = nlohmann::json;

int main()
{
    asio::io_context io;

    TCPListener tcpListener(io, 9000,
        [](asio::ip::tcp::socket socket) {
            auto session = std::make_shared<Session>(std::move(socket));
            session->Start();
        }
    );

	UDPListener udpListener(io, 9001, RoomManager::Instance());

    tcpListener.Start();
    udpListener.Start();
    // asio 내부 큐를 돌리는 함수
    io.run();

    return 0;
}