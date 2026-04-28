#include "Session.hpp"
#include "../room/RoomManager.hpp"
#include <iostream>
#include <iomanip>

Session::Session(asio::ip::tcp::socket socket) : _socket(std::move(socket)) {}

Session::~Session()
{
    if (_roomId != -1)
        RoomManager::Instance().LeaveRoom(_roomId, _userId);
}

void Session::Start()
{
    std::cout << "CLIENT CONNECT : " << _socket.remote_endpoint().address().to_string() << std::endl;

    auto roomList = RoomManager::Instance().GetRoomList();
    json j = { {"event", "room_list"}, {"rooms", json::array()} };
    for (auto& [roomId, count] : roomList)
        j["rooms"].push_back({ {"roomId", roomId}, {"count", count} });

    PacketHeader header{};
    header.type = PACKET_TYPE_CONTROL;
    std::string payload = j.dump();
    header.payloadLength = static_cast<uint16_t>(payload.size());
    Send(header, payload);

    ReceiveHeader();
}

void Session::ReceiveHeader()
{
    auto self = shared_from_this();
    asio::async_read(_socket,
        asio::buffer(_headerBuf, HEADER_SIZE),
        [this, self](asio::error_code ec, std::size_t)
        {
            if (ec) {
                std::cout << "CLIENT DISCONNECTED : " << _userId << std::endl;
                return;
            }
            PacketHeader header;
            if (!DeserializeHeader(_headerBuf, HEADER_SIZE, header))
                return;
            ReceivePayload(header);
        });
}

void Session::ReceivePayload(PacketHeader header)
{
    auto self = shared_from_this();
    auto payloadBuf = std::make_shared<std::vector<uint8_t>>(header.payloadLength);

    asio::async_read(_socket,
        asio::buffer(*payloadBuf),
        [this, self, header, payloadBuf](asio::error_code ec, std::size_t)
        {
            if (ec) return;

            std::string jsonStr(payloadBuf->begin(), payloadBuf->end());
            HandlePacket(header, jsonStr);

            ReceiveHeader();
        });
}

void Session::HandlePacket(const PacketHeader& header, const std::string& payload)
{
    if (header.type == PACKET_TYPE_CONTROL)
    {
        json j = json::parse(payload);
        std::string cmd = j.value("cmd", "");
        std::cout << "[DEBUG] Received cmd: " << cmd << std::endl;

        if (cmd == "join")
        {
            _userId = j.value("userId", 0);
            _roomId = j.value("roomId", 0);
            std::cout << "join - userId: " << _userId << " roomId: " << _roomId << "\n";

            RoomManager::Instance().JoinRoom(_roomId, shared_from_this());
        }
    }
}

void Session::Send(const PacketHeader& header, const std::string& payload) { /* 기존 코드 */ }
void Session::SendRaw(const std::vector<uint8_t>& data) { /* 기존 코드 */ }

// UDP Endpoint 함수 (이전 추가한 부분)
void Session::SetUdpEndpoint(const asio::ip::udp::endpoint& endpoint) { _udpEndpoint = endpoint; }
asio::ip::udp::endpoint Session::GetUdpEndpoint() const { return _udpEndpoint; }