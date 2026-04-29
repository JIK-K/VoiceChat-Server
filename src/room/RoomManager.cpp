#include "RoomManager.hpp"
#include "Room.hpp"
#include "../protocol/Packet.hpp"     // PacketHeader, DeserializeHeader, PACKET_TYPE_VOICE
#include <iostream>
#include "../network/UDPListener.hpp"

RoomManager& RoomManager::Instance()
{
    static RoomManager instance;
    return instance;
}

void RoomManager::JoinRoom(int roomId, std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_rooms.find(roomId) == _rooms.end()) {
        _rooms[roomId] = std::make_shared<Room>(roomId);
        std::cout << "ROOM [" << roomId << "] CREATED" << std::endl;
    }
    _rooms[roomId]->AddSession(session);
}

bool RoomManager::LeaveRoom(int roomId, int userId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _rooms.find(roomId);
    if (it == _rooms.end()) return false;

    it->second->RemoveSession(userId);

    if (it->second->IsEmpty()) {
        _rooms.erase(it);
        std::cout << "ROOM [" << roomId << "] DELETED" << std::endl;
        return true;
    }

    return false;
}

std::shared_ptr<Room> RoomManager::FindRoom(int roomId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _rooms.find(roomId);
    if (it == _rooms.end())
        return nullptr;
    return it->second;
}

std::vector<std::pair<int, int>> RoomManager::GetRoomList()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<std::pair<int, int>> result;
    for (auto& pair : _rooms)
        result.push_back({ pair.first, pair.second->GetUserCount() });
    return result;
}

// ==================== 개선된 UDP 음성 패킷 처리 ====================
void RoomManager::HandleUdpVoicePacket(const asio::ip::udp::endpoint& senderEndpoint,
    const char* data, int length)
{
    if (length < HEADER_SIZE) return;

    PacketHeader header;
    if (!DeserializeHeader(reinterpret_cast<const uint8_t*>(data), HEADER_SIZE, header))
        return;

    if (header.type != PACKET_TYPE_VOICE)
        return;

    // 엔드포인트 최신화
    RegisterUdpEndpoint(header.userId, senderEndpoint);

    auto room = FindRoom(header.roomId);

    if (!room)
    {
        // UDP 왔을때 방 만들면 안됨.
        //std::lock_guard<std::mutex> lock(_mutex);
        //_rooms[header.roomId] = std::make_shared<Room>(header.roomId);
        //room = _rooms[header.roomId];
        std::cout << "[UDP] roomId : " << header.roomId << "NO EXIST ROOM\n";
        return;
    }

    const char* payload = data + HEADER_SIZE;
    int payloadLen = length - HEADER_SIZE;

    room->BroadcastVoice(senderEndpoint, header, payload, payloadLen);
}

void RoomManager::RegisterUdpEndpoint(int userId, const asio::ip::udp::endpoint& endpoint)
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    // 동일한 엔드포인트 = 스킵
    auto it = _userEndpoints.find(userId);
    if (it != _userEndpoints.end() && it->second == endpoint)
        return;
    _userEndpoints[userId] = endpoint;
    std::cout << "[UDP] User " << userId << " endpoint 등록: "
        << endpoint.address().to_string() << ":" << endpoint.port() << std::endl;
}

asio::ip::udp::endpoint RoomManager::GetUdpEndpoint(int userId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _userEndpoints.find(userId);
    if (it != _userEndpoints.end())
        return it->second;
    return asio::ip::udp::endpoint();  // 빈 endpoint
}

void RoomManager::RemoveUdpEndpoint(int userId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _userEndpoints.erase(userId);
}

void RoomManager::SendVoicePacket(const asio::ip::udp::endpoint& target,
    const uint8_t* data, std::size_t length)
{
    if (m_udpListener == nullptr)
    {
        std::cout << "[RoomManager] SendVoicePacket 실패: UDPListener가 없습니다." << std::endl;
        return;
    }

    m_udpListener->SendTo(target, data, length);
}

std::vector<std::pair<int, asio::ip::udp::endpoint>> RoomManager::GetAllUdpEndpoints() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<std::pair<int, asio::ip::udp::endpoint>> result;
    for (const auto& pair : _userEndpoints)
    {
        result.emplace_back(pair.first, pair.second);
    }
    return result;
}