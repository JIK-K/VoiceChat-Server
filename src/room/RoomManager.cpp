#include "RoomManager.hpp"
#include <iostream>

RoomManager& RoomManager::Instance()
{
	static RoomManager instance;
	return instance;
}

void RoomManager::JoinRoom(int roomId, std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(_mutex);

	// find를 하면 그 위치를 가리키는 iterator를 반환한다.
	// 못찾았으면 맨 마지막 다음칸 -> end()를 반환한다.
	if (_rooms.find(roomId) == _rooms.end()) {
		_rooms[roomId] = std::make_shared<Room>(roomId);
		std::cout << "ROOM [ " << roomId << " ] CREATED" << "\n";
	}

	_rooms[roomId]->AddSession(session);
}

void RoomManager::LeaveRoom(int roomId, int userId)
{
	std::lock_guard<std::mutex> lock(_mutex);

	// std::unordered_map<int, std::shared_ptr<Room>>::iterator it = _rooms.find(roomId) 랑 같은거
	auto it = _rooms.find(roomId);
	if (it == _rooms.end()) return;

	// std::shared_ptr<Room>을 가리키는 것
	it->second->RemoveSession(userId);

	if (it->second->IsEmpty()) {
		_rooms.erase(it);
		std::cout << "ROOM [ " << roomId << " ] DELETED " << "\n";
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
		result.push_back({pair.first, pair.second->GetUserCount()});

	return result;
}