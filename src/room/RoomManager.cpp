#include "RoomManager.hpp"
#include "Room.hpp"
#include "../protocol/Packet.hpp"     // PacketHeader, DeserializeHeader, PACKET_TYPE_VOICE
#include <iostream>

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

void RoomManager::LeaveRoom(int roomId, int userId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _rooms.find(roomId);
    if (it == _rooms.end()) return;

    it->second->RemoveSession(userId);

    if (it->second->IsEmpty()) {
        _rooms.erase(it);
        std::cout << "ROOM [" << roomId << "] DELETED" << std::endl;
    }
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
    if (length < HEADER_SIZE)
    {
        std::cout << "[UDP] 패킷 크기 부족 (" << length << " bytes)" << std::endl;
        return;
    }

    // 1. 헤더 파싱
    PacketHeader header;
    if (!DeserializeHeader(reinterpret_cast<const uint8_t*>(data), HEADER_SIZE, header))
    {
        std::cout << "[UDP] 헤더 파싱 실패" << std::endl;
        return;
    }

    // 2. 음성 패킷인지 확인
    if (header.type != PACKET_TYPE_VOICE)
    {
        std::cout << "[UDP] 잘못된 패킷 타입: " << (int)header.type << std::endl;
        return;
    }

    // 3. 해당 방 찾기
    auto room = FindRoom(header.roomId);
    if (!room)
    {
        std::cout << "[UDP] 존재하지 않는 방: roomId=" << header.roomId << std::endl;
        return;
    }

    // 4. Room에게 릴레이 위임 (송신자 제외 브로드캐스트)
    const char* payload = data + HEADER_SIZE;
    int payloadLen = length - HEADER_SIZE;

    room->BroadcastVoice(senderEndpoint, header, payload, payloadLen);
}