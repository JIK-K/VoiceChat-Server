#include "Room.hpp"
#include "../network/Session.hpp"
#include "../protocol/Packet.hpp"
#include <iostream>
#include <algorithm>   // std::remove_if
#include "RoomManager.hpp"

Room::Room(int roomId) : _roomId(roomId) {}

void Room::AddSession(std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.push_back(session);
    std::cout << "ROOM [" << _roomId << "] ADD SESSION USERID : "
        << session->GetUserId() << std::endl;
}

void Room::RemoveSession(int userId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(
        std::remove_if(_sessions.begin(), _sessions.end(),
            [userId](const std::shared_ptr<Session>& s) {
                return s->GetUserId() == userId;
            }),
        _sessions.end()
    );
    std::cout << "ROOM [" << _roomId << "] REMOVE SESSION userId: "
        << userId << std::endl;
}

// ==================== UDP 음성 브로드캐스트 ====================
void Room::BroadcastVoice(const asio::ip::udp::endpoint& senderEndpoint,
    const PacketHeader& header,
    const char* payload, int payloadLen)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto fullPacket = SerializePacket(header,
        reinterpret_cast<const uint8_t*>(payload),
        static_cast<uint16_t>(payloadLen));

    int targetCount = 0;
    RoomManager& rm = RoomManager::Instance();

    for (const auto& session : _sessions)
    {
        if (session->GetUserId() == header.userId)
            continue;
        
        auto endpoint = rm.GetUdpEndpoint(session->GetUserId());
        if (endpoint.port() == 0)
            continue;

        targetCount++;
        rm.SendVoicePacket(endpoint, fullPacket.data(), fullPacket.size());
    }

    if (targetCount > 0)
    {
        std::cout << "ROOM [" << _roomId << "] Voice Broadcast → "
            << targetCount << " clients ("
            << fullPacket.size() << " bytes) | From User "
            << header.userId << std::endl;
    }
}
// ====================================================================

void Room::Broadcast(int senderId, const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& session : _sessions) {
        if (session->GetUserId() != senderId)
            session->SendRaw(data);
    }
}

void Room::BroadcastJson(int senderId, const std::string& payload)
{
    std::lock_guard<std::mutex> lock(_mutex);
    PacketHeader header{};
    header.type = PACKET_TYPE_CONTROL;
    header.payloadLength = static_cast<uint16_t>(payload.size());

    for (auto& session : _sessions) {
        if (session->GetUserId() != senderId)
            session->Send(header, payload);
    }
}

bool Room::IsEmpty() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _sessions.empty();
}

std::vector<int> Room::GetUserList() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<int> userIds;
    for (auto& session : _sessions)
        userIds.push_back(session->GetUserId());
    return userIds;
}

int Room::GetUserCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return static_cast<int>(_sessions.size());
}