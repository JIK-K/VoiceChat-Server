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

    RoomManager& roomManager = RoomManager::Instance();

    UDPListener udpListener(io, 9001, roomManager);
    roomManager.SetUdpListener(&udpListener);

    tcpListener.Start();
    udpListener.Start();
    // asio 내부 큐를 돌리는 함수
    io.run();

    return 0;
}