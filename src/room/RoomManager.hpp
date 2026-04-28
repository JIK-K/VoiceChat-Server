#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include "Room.hpp"
#include <asio.hpp>

class Session;
class UDPListener;

class RoomManager
{
public:
    static RoomManager& Instance();

    void JoinRoom(int roomId, std::shared_ptr<Session> session);
    void LeaveRoom(int roomId, int userId);

    std::shared_ptr<Room> FindRoom(int roomId);
    std::vector<std::pair<int, int>> GetRoomList();

    // 엔드포인트 매핑
    void RegisterUdpEndpoint(int userId, const asio::ip::udp::endpoint& endpoint);
    asio::ip::udp::endpoint GetUdpEndpoint(int userId) const;
    void RemoveUdpEndpoint(int userId);

    // UDPListener 설정 및 음성 패킷 전송
    void SetUdpListener(UDPListener* listener) { m_udpListener = listener; }

    void SendVoicePacket(const asio::ip::udp::endpoint& target,
        const uint8_t* data, std::size_t length);

    void HandleUdpVoicePacket(const asio::ip::udp::endpoint& senderEndpoint,
        const char* data, int length);

    std::vector<std::pair<int, asio::ip::udp::endpoint>> GetAllUdpEndpoints() const;

private:
    RoomManager() = default;

    std::unordered_map<int, std::shared_ptr<Room>> _rooms;
    std::unordered_map<int, asio::ip::udp::endpoint> _userEndpoints;

    mutable std::mutex _mutex;           // const 함수에서도 lock 가능
    UDPListener* m_udpListener = nullptr;   // ← 여기서 선언!!
};