#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include "Room.hpp"
#include <asio.hpp>          // ← 추가
#include <string>            // ← 추가 (로그용)

class Session;

class RoomManager
{
public:
    static RoomManager& Instance();

    void JoinRoom(int roomId, std::shared_ptr<Session> session);
    void LeaveRoom(int roomId, int userId);

    std::shared_ptr<Room> FindRoom(int roomId);
    std::vector<std::pair<int, int>> GetRoomList();

    // ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
    // UDP 음성 패킷 처리 (UDPSocket에서 호출됨)
    void HandleUdpVoicePacket(const asio::ip::udp::endpoint& senderEndpoint,
        const char* data, int length);
    // ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←

private:
    RoomManager() = default;

    // roomId - room
    std::unordered_map<int, std::shared_ptr<Room>> _rooms;
    std::mutex _mutex;
};