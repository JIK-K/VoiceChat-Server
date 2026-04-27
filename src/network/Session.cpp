#include "Session.hpp"
#include <iostream>

Session::Session(asio::ip::tcp::socket socket) : _socket(std::move(socket)) {}

void Session::Start() {
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
			std::cout << "join - userId: " << _userId
				<< " roomId: " << _roomId << "\n";

			// 응답
			Send(header, R"({"result":"ok","msg":"join success"})");
		}
		else if (cmd == "leave") {
			std::cout << "leave - userId: " << _userId << "\n";
			Send(header, R"({"result":"ok","msg":"leave success"})");
			_roomId = -1;
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