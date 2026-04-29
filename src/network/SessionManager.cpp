#include "SessionManager.hpp"
#include "Session.hpp"
#include "../protocol/Packet.hpp"

SessionManager& SessionManager::Instance()
{
    static SessionManager instance;
    return instance;
}

void SessionManager::AddSession(int userId, std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions[userId] = session;
}

void SessionManager::RemoveSession(int userId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(userId);
}

void SessionManager::BroadcastAll(const std::string& payload)
{
    std::lock_guard<std::mutex> lock(_mutex);
    PacketHeader header{};
    header.type = PACKET_TYPE_CONTROL;
    header.payloadLength = static_cast<uint16_t>(payload.size());

    for (auto& pair : _sessions)
        pair.second->Send(header, payload);
}