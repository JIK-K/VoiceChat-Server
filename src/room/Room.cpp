#include "Room.hpp"
#include "../network/Session.hpp"
#include <iostream>

Room::Room(int roomId) : _roomId(roomId) {}

void Room::AddSession(std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_sessions.push_back(session);
	std::cout << "ROOM [ " << _roomId << " ] ADD SESSION USERID : " << session->GetUserId() << "\n";
}

void Room::RemoveSession(int userId)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // vector에서의 erase는 map이랑 다름 
    // std::remove_if로 삭제할 대상을 맨뒤로 보냄. 여기서부터 지운다는 표시정도
    // _sessions.erase로 진짜지움. 
    // Erase-Remove 방식
    _sessions.erase(
        std::remove_if(_sessions.begin(), _sessions.end(),
            [userId](const std::shared_ptr<Session>& s) {
                return s->GetUserId() == userId;
            }
        ),
        _sessions.end()
    );
    std::cout << "ROOM [" << _roomId << "] REMOVE SESSION userId: " << userId << "\n";
}

// UDP Broadcast
void Room::Broadcast(int senderId, const std::vector<uint8_t>& data) {
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

std::vector<int> Room::GetUserList() const {
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