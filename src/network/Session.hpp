#pragma once
#define ASIO_STANDALONE
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <functional>
#include "../protocol/Packet.hpp"

using json = nlohmann::json;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(asio::ip::tcp::socket socket);
    ~Session();

    void Start();
    void Send(const PacketHeader& header, const std::string& payload);
    void SendRaw(const std::vector<uint8_t>& data);

    int GetUserId() const { return _userId; }
    int GetRoomId() const { return _roomId; }
    void setRoomId(int roomId) { _roomId = roomId; }

    // UDP 엔드포인트 연동
    void SetUdpEndpoint(const asio::ip::udp::endpoint& endpoint);
    asio::ip::udp::endpoint GetUdpEndpoint() const;

private:
    void ReceiveHeader();
    void ReceivePayload(PacketHeader header);
    void HandlePacket(const PacketHeader& header, const std::string& payload);

    asio::ip::tcp::socket _socket;
    uint8_t _headerBuf[HEADER_SIZE];

    int _userId = 0;
    int _roomId = 0;

    asio::ip::udp::endpoint _udpEndpoint;   // ← UDP 주소 저장
};