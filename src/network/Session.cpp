#include "Session.hpp"
#include "../room/RoomManager.hpp"
#include <iostream>

Session::Session(asio::ip::tcp::socket socket) : _socket(std::move(socket)) {}
Session::~Session()
{
	if (_roomId != -1)
		RoomManager::Instance().LeaveRoom(_roomId, _userId);
}
void Session::Start() {
	auto roomList = RoomManager::Instance().GetRoomList();
	json j = { {"event", "room_list"}, {"rooms", json::array()} };
	for (auto& [roomId, count] : roomList)
		j["rooms"].push_back({ {"roomId", roomId}, {"count", count} });

	PacketHeader header{};
	header.type = PACKET_TYPE_CONTROL;
	std::string payload = j.dump();
	header.payloadLength = static_cast<uint16_t>(payload.size());
	Send(header, payload);

	ReceiveHeader();
}

void Session::ReceiveHeader() {
	auto self = shared_from_this();

	asio::async_read(_socket,
		asio::buffer(_headerBuf, HEADER_SIZE),
		[this, self](asio::error_code ec, std::size_t) {
			if (ec) {
				std::cout << "CLIENT DISCONNECTED : " << _userId << "\n";
				return;
			}

			PacketHeader header;
			if (!DeserializeHeader(_headerBuf, HEADER_SIZE, header)) {
				std::cout << "HEADER PARSING FAIL" << "\n";
				return;
			}

			ReceivePayload(header);
		}
	);
}

void Session::ReceivePayload(PacketHeader header) {
	auto self = shared_from_this();

	auto payloadBuf = std::make_shared<std::vector<uint8_t>>(header.payloadLength);

	asio::async_read(_socket,
		asio::buffer(*payloadBuf),
		[this, self, header, payloadBuf](asio::error_code ec, std::size_t) {
			if (ec) {
				std::cout << "PAYLOAD RECEIVED FAIL" << "\n";
				return;
			}

			std::string jsonStr(payloadBuf->begin(), payloadBuf->end());

			HandlePacket(header, jsonStr);
			ReceiveHeader();
		}
	);
}

void Session::HandlePacket(const PacketHeader& header, const std::string& payload)
{
	// 제어 패킷 (TCP)
	if (header.type == PACKET_TYPE_CONTROL) {
		json j = json::parse(payload);
		std::string cmd = j["cmd"];

		if (cmd == "join") {
			_userId = j["userId"];
			_roomId = j["roomId"];
			std::cout << "join - userId: " << _userId << " roomId: " << _roomId << "\n";

			RoomManager::Instance().JoinRoom(_roomId, shared_from_this());

			auto room = RoomManager::Instance().FindRoom(_roomId);
			if (room) {
				json userList = { 
					{"event", "user_list"}, 
					{"users", room->GetUserList()} 
				};
				Send(header, userList.dump());

				json joined = { {"event", "user_joined"}, {"userId", _userId} };
				room->BroadcastJson(_userId, joined.dump());
			}
		}
		else if (cmd == "leave") {
			auto room = RoomManager::Instance().FindRoom(_roomId);
			if (room) {
				json left = { 
					{"event", "user_left"}, 
					{"userId", _userId} 
				};
				room->BroadcastJson(_userId, left.dump());
				RoomManager::Instance().LeaveRoom(_roomId, _userId);
			}
			std::cout << "leave - userId: " << _userId << "\n";
			_roomId = -1;
			Send(header, R"({"result":"ok","msg":"leave success"})");
		}
	}
}

void Session::Send(const PacketHeader& header, const std::string& payload)
{
	auto packet = std::make_shared<std::vector<uint8_t>>(
		SerializePacket(
			header,
			reinterpret_cast<const uint8_t*>(payload.data()),
			static_cast<uint16_t>(payload.size())
		)
	);

	auto self = shared_from_this();
	asio::async_write(_socket,
		asio::buffer(*packet),
		[this, self, packet](asio::error_code ec, std::size_t) {
			if (ec) {
				std::cout << "SEND FAIL : USER : " << _userId << "\n";
			}
		}
	);
}

void Session::SendRaw(const std::vector<uint8_t>& data)
{
	auto packet = std::make_shared<std::vector<uint8_t>>(data);
	auto self = shared_from_this();

	asio::async_write(_socket,
		asio::buffer(*packet),
		[this, self, packet](asio::error_code ec, std::size_t) {
			if (ec)
				std::cout << "SENDRAW FAIL : USER : " << _userId << "\n";
		}
	);
}