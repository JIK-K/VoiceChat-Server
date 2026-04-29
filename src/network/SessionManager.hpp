#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>

class Session;

class SessionManager
{
public:
    static SessionManager& Instance();

    void AddSession(int userId, std::shared_ptr<Session> session);
    void RemoveSession(int userId);

    // 모든 세션한테 브로드캐스트
    void BroadcastAll(const std::string& payload);

private:
    SessionManager() = default;
    std::unordered_map<int, std::shared_ptr<Session>> _sessions;
    mutable std::mutex _mutex;
};