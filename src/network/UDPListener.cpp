#include "UDPListener.hpp"
#include "../protocol/Packet.hpp"   // HEADER_SIZE 등
#include <iostream>

UDPListener::UDPListener(asio::io_context& io_context, unsigned short port, RoomManager& roomManager)
    : m_ioContext(io_context)
    , m_socket(io_context)
    , m_roomManager(roomManager)
    , m_recvBuffer(2048)
{
    asio::error_code ec;

    m_socket.open(asio::ip::udp::v4(), ec);
    if (ec)
    {
        std::cout << "UDP socket open 실패: " << ec.message() << std::endl;
        return;
    }

    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), port);
    m_socket.bind(endpoint, ec);
    if (ec)
    {
        std::cout << "UDP bind 실패 (포트 " << port << "): " << ec.message() << std::endl;
        return;
    }
    std::cout << "UDP SERVER START - PORT : " << port << std::endl;
}

UDPListener::~UDPListener()
{
    Stop();
}

void UDPListener::Start()
{
    if (m_running) return;
    m_running = true;
    DoReceive();
    std::cout << "UDP SERVER LISTENING (비동기)" << std::endl;
}

void UDPListener::Stop()
{
    if (!m_running) return;
    m_running = false;

    asio::error_code ec;
    m_socket.close(ec);
    std::cout << "UDPListener 수신 중지" << std::endl;
}

void UDPListener::DoReceive()
{
    if (!m_running) return;

    m_socket.async_receive_from(
        asio::buffer(m_recvBuffer),
        m_senderEndpoint,
        [this](asio::error_code ec, std::size_t bytes_received)
        {
            if (!m_running) return;

            if (!ec && bytes_received > 0)
            {
                m_roomManager.HandleUdpVoicePacket(m_senderEndpoint,
                    m_recvBuffer.data(),
                    static_cast<int>(bytes_received));
            }
            else if (ec && ec != asio::error::operation_aborted)
            {
                std::cout << "UDP async_receive_from 오류: " << ec.message() << std::endl;
            }

            DoReceive();  // 다시 수신 대기
        });
}

void UDPListener::SendTo(const asio::ip::udp::endpoint& target,
    const uint8_t* data, std::size_t length)
{
    if (!m_socket.is_open()) return;

    asio::error_code ec;
    m_socket.send_to(asio::buffer(data, length), target, 0, ec);

    if (ec)
    {
        std::cout << "UDP SendTo 실패: " << ec.message() << std::endl;
    }
}