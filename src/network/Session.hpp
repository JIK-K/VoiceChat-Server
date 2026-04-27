#pragma once
#define ASIO_STANDALONE
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <functional>
#include "../protocol/Packet.hpp"

using json = nlohmann::json;

/*
* shared_ptr을 사용한다. -> 참조 카운터가 0이 될때까지 객체 안사라짐
* async_read은 등록만하고 바로 리턴한다(asio 내부 큐에 들어감) 이후
* 호출될때 Session 객체가 존재하지 않을 수 있기 때문에 사용함.
*/
class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(asio::ip::tcp::socket socket);
	~Session();

	void Start();
	void Send(const PacketHeader& header, const std::string& payload);
	void SendRaw(const std::vector<uint8_t>& data);

	int GetUserId() const { return _userId; }
	int GetRoomId() const { return _roomId; }
	void setRoomId(int roomId) { _roomId = roomId; }

private:
	void ReceiveHeader();
	void ReceivePayload(PacketHeader header);
	void HandlePacket(const PacketHeader& heaser, const std::string& payload);

	asio::ip::tcp::socket _socket;
	uint8_t _headerBuf[HEADER_SIZE];
	int _userId = 0;
	int _roomId = 0;
};
