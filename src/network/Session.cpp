#include "Session.hpp"
#include "../room/RoomManager.hpp"
#include <iostream>
#include <iomanip>
#include "SessionManager.hpp"
#include <atomic>

int generateSafeIntID();

Session::Session(asio::ip::tcp::socket socket) : _socket(std::move(socket)) {}

Session::~Session()
{
    if (_roomId != -1) {
        auto room = RoomManager::Instance().FindRoom(_roomId);
        if (room) {
            json left = {
                {"event", "user_left"},
                {"userId", _userId}
            };
            room->BroadcastJson(_userId, left.dump());
        }
        auto roomDeleted = RoomManager::Instance().LeaveRoom(_roomId, _userId);
        if (roomDeleted) {
            // 방 삭제됬으면 전체 Session에 업데이트 된 room_list 브로드캐스트
            auto roomList = RoomManager::Instance().GetRoomList();
            json j = {
                {"event", "room_list"},
                {"rooms", json::array()}
            };
            for (auto& [rId, count] : roomList)
                j["rooms"].push_back({ {"roomId", rId}, {"count", count} });

            SessionManager::Instance().BroadcastAll(j.dump());
        }
    }
    SessionManager::Instance().RemoveSession(_userId);
    std::cout << "CLIENT DISCONNECT : userId " << _userId << std::endl;
}

void Session::Start()
{
    _userId = generateSafeIntID();

    std::cout << "CLIENT CONNECT : " << _socket.remote_endpoint().address().to_string()
        << " userId: " << _userId << std::endl;

    SessionManager::Instance().AddSession(_userId, shared_from_this());

    PacketHeader header{};
    header.type = PACKET_TYPE_CONTROL;

    json connected = {
        {"event", "connected"},
        {"userId", _userId}
    };
    std::string connectedPayload = connected.dump();
    Send(header, connectedPayload);

    auto roomList = RoomManager::Instance().GetRoomList();
    json j = { 
        {"event", "room_list"}, 
        {"rooms", json::array()} 
    };
    for (auto& [roomId, count] : roomList)
        j["rooms"].push_back({ {"roomId", roomId}, {"count", count} });

    std::string payload = j.dump();
    Send(header, payload);

    ReceiveHeader();
}

void Session::ReceiveHeader()
{
    auto self = shared_from_this();
    asio::async_read(_socket,
        asio::buffer(_headerBuf, HEADER_SIZE),
        [this, self](asio::error_code ec, std::size_t)
        {
            if (ec) {
                std::cout << "CLIENT DISCONNECTED : " << _userId << std::endl;
                return;
            }
            PacketHeader header;
            if (!DeserializeHeader(_headerBuf, HEADER_SIZE, header))
                return;
            ReceivePayload(header);
        });
}

void Session::ReceivePayload(PacketHeader header)
{
    auto self = shared_from_this();
    auto payloadBuf = std::make_shared<std::vector<uint8_t>>(header.payloadLength);

    asio::async_read(_socket,
        asio::buffer(*payloadBuf),
        [this, self, header, payloadBuf](asio::error_code ec, std::size_t)
        {
            if (ec) return;

            std::string jsonStr(payloadBuf->begin(), payloadBuf->end());
            HandlePacket(header, jsonStr);

            ReceiveHeader();
        });
}

void Session::HandlePacket(const PacketHeader& header, const std::string& payload)
{
    if (header.type == PACKET_TYPE_CONTROL)
    {
        json j = json::parse(payload);
        std::string cmd = j.value("cmd", "");
        std::cout << "[DEBUG] Received cmd: " << cmd << std::endl;

        if (cmd == "join")
        {
            //_userId = j.value("userId", 0);
            _roomId = j.value("roomId", 0);
            std::cout << "join - userId: " << _userId << " roomId: " << _roomId << "\n";

            RoomManager::Instance().JoinRoom(_roomId, shared_from_this());

            auto room = RoomManager::Instance().FindRoom(_roomId);

            if (room) {
                json userList = {
                    {"event", "user_list"},
                    {"users", room->GetUserList()}
                };
                PacketHeader responseHeader{};
                responseHeader.type = PACKET_TYPE_CONTROL;
                std::string payload = userList.dump();
                responseHeader.payloadLength = static_cast<uint16_t>(payload.size());
                Send(responseHeader, payload);

                json joined = {
                    {"event", "user_joined"},
                    {"userId", _userId}
                };
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
                bool roomDeleted = RoomManager::Instance().LeaveRoom(_roomId, _userId);

                if (roomDeleted) {
                    // 방 삭제됬으면 전체 Session에 업데이트 된 room_list 브로드캐스트
                    auto roomList = RoomManager::Instance().GetRoomList();
                    json j = { 
                        {"event", "room_list"},
                        {"rooms", json::array()} 
                    };
                    for (auto& [rId, count] : roomList)
                        j["rooms"].push_back({ {"roomId", rId}, {"count", count} });

                    SessionManager::Instance().BroadcastAll(j.dump());
                }
            }

            std::cout << "leave - userId : " << _userId << "\n";
            _roomId = -1;
            Send(header, R"({"result":"ok","msg":"leave success"})");
        }
    }
}

void Session::Send(const PacketHeader& header, const std::string& payload)
{
    // 헤더 복사 후 payloadLength 강제 설정
    PacketHeader fixedHeader = header;
    fixedHeader.payloadLength = static_cast<uint16_t>(payload.size());

    auto packet = std::make_shared<std::vector<uint8_t>>(
        SerializePacket(
            fixedHeader,
            reinterpret_cast<const uint8_t*>(payload.data()),
            fixedHeader.payloadLength
        )
    );

    auto self = shared_from_this();
    asio::async_write(_socket,
        asio::buffer(*packet),
        [this, self, packet](asio::error_code ec, std::size_t) {
            if (ec)
                std::cout << "SEND FAIL : USER : " << _userId << "\n";
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

// UDP Endpoint 함수 (이전 추가한 부분)
void Session::SetUdpEndpoint(const asio::ip::udp::endpoint& endpoint) { _udpEndpoint = endpoint; }
asio::ip::udp::endpoint Session::GetUdpEndpoint() const { return _udpEndpoint; }



int generateSafeIntID() {
    static std::atomic<int> counter(1);
    return counter.fetch_add(1);
}