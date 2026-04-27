#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include "Room.hpp"

class Session;

class RoomManager
{
public:
	static RoomManager& Instance();

	void JoinRoom(int roomId, std::shared_ptr<Session> session);

	void LeaveRoom(int roomId, int userId);

	// UDP BroadCast 방 찾기
	std::shared_ptr<Room> FindRoom(int roomId);

	// 방 목록 반환
	std::vector<std::pair<int, int>> GetRoomList();

private:
	RoomManager() = default;

	// roomId - room
	std::unordered_map<int, std::shared_ptr<Room>> _rooms;
	std::mutex _mutex;
};
