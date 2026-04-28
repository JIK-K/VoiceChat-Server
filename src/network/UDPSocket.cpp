#include "UDPSocket.hpp"
#include "../room/RoomManager.hpp"
#include <iostream>

UDPSocket::UDPSocket(asio::io_context& io_context, RoomManager* roomManager)
    : m_ioContext(io_context)
    , m_socket(io_context)
    , m_roomManager(roomManager)
    , m_recvBuffer(2048)
{
}

UDPSocket::~UDPSocket()
{
    StopReceiving();
}

bool UDPSocket::Initialize(unsigned short port)
{
    asio::error_code ec;

    m_socket.open(asio::ip::udp::v4(), ec);
    if (ec)
    {
        std::cout << "UDP socket open 실패: " << ec.message() << std::endl;
        return false;
    }

    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), port);
    m_socket.bind(endpoint, ec);
    if (ec)
    {
        std::cout << "UDP bind 실패 (포트 " << port << "): " << ec.message() << std::endl;
        return false;
    }

    std::cout << "UDP 소켓 초기화 완료 (포트 " << port << ")" << std::endl;
    return true;
}

void UDPSocket::StartReceiving()
{
    if (m_running) return;
    m_running = true;
    DoReceive();
    std::cout << "UDP 비동기 수신 시작 (asio)" << std::endl;
}

void UDPSocket::StopReceiving()
{
    if (!m_running) return;
    m_running = false;

    asio::error_code ec;
    m_socket.close(ec);
    std::cout << "UDP 수신 중지" << std::endl;
}

bool UDPSocket::SendTo(const asio::ip::udp::endpoint& clientEndpoint,
    const char* data, std::size_t length)
{
    if (!m_socket.is_open()) return false;

    asio::error_code ec;
    m_socket.send_to(asio::buffer(data, length), clientEndpoint, 0, ec);

    if (ec)
    {
        std::cout << "UDP SendTo 실패: " << ec.message() << std::endl;
        return false;
    }
    return true;
}

void UDPSocket::DoReceive()
{
    if (!m_running) return;

    m_socket.async_receive_from(
        asio::buffer(m_recvBuffer),
        m_senderEndpoint,
        [this](asio::error_code ec, std::size_t bytes_received)
        {
            if (!m_running) return;

            if (!ec && bytes_received > 0 && m_roomManager)
            {
                m_roomManager->HandleUdpVoicePacket(m_senderEndpoint,
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