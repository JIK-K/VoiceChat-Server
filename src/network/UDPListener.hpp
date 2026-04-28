#pragma once

#include <asio.hpp>
#include <vector>
#include <atomic>
#include "../room/RoomManager.hpp"

class UDPListener
{
public:
    UDPListener(asio::io_context& io_context, unsigned short port, RoomManager& roomManager);
    ~UDPListener();

    void Start();
    void Stop();

private:
    void DoReceive();

    asio::io_context& m_ioContext;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_senderEndpoint;
    std::vector<char> m_recvBuffer;

    RoomManager& m_roomManager;
    std::atomic<bool> m_running{ false };
};