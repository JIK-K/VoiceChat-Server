#pragma once

#include <asio.hpp>
#include <vector>
#include <atomic>

class RoomManager;

class UDPSocket
{
public:
    explicit UDPSocket(asio::io_context& io_context, RoomManager* roomManager);
    ~UDPSocket();

    bool Initialize(unsigned short port);

    void StartReceiving();
    void StopReceiving();

    bool SendTo(const asio::ip::udp::endpoint& clientEndpoint,
        const char* data, std::size_t length);

private:
    void DoReceive();

    asio::io_context& m_ioContext;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_senderEndpoint;
    std::vector<char> m_recvBuffer;

    RoomManager* m_roomManager = nullptr;
    std::atomic<bool> m_running{ false };
};