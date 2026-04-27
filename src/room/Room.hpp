#pragma once
#include <memory>
#include <vector>
#include <mutex>

class Session;

class Room
{
public:
	Room(int roomId);

	void AddSession(std::shared_ptr<Session> session);
	void RemoveSession(int userId);

	// UDP Broadcast
	void Broadcast(int senderId, const std::vector<uint8_t>& data);
	void BroadcastJson(int senderId, const std::string& json);

	int GetRoomId() const { return _roomId; }

	bool IsEmpty() const;

	std::vector<int> GetUserList() const;
	int GetUserCount() const;

private:
	int _roomId;
	std::vector<std::shared_ptr<Session>> _sessions;
	mutable std::mutex _mutex;
};