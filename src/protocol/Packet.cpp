#pragma once

#include <cstdint>
#include <vector>
#include <cstring>     // memcpy
#include <winsock2.h>  // htonl, ntohl, htons, ntohs

// 구조체 패딩 제거 (정확히 13바이트 보장)
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  type;          // 0x01: 제어, 0x02: 음성
    int32_t  roomId;
    int32_t  userId;
    uint16_t sequence;
    uint16_t payloadLength;
};
#pragma pack(pop)

constexpr uint8_t PACKET_TYPE_CONTROL = 0x01;
constexpr uint8_t PACKET_TYPE_VOICE = 0x02;
constexpr int HEADER_SIZE = 13;

// ====================== 직렬화 (inline) ======================
inline std::vector<uint8_t> SerializePacket(const PacketHeader& header,
    const uint8_t* payload,
    uint16_t payloadLen)
{
    std::vector<uint8_t> buf(HEADER_SIZE + payloadLen);

    buf[0] = header.type;
    *reinterpret_cast<int32_t*>(&buf[1]) = htonl(header.roomId);
    *reinterpret_cast<int32_t*>(&buf[5]) = htonl(header.userId);
    *reinterpret_cast<uint16_t*>(&buf[9]) = htons(header.sequence);
    *reinterpret_cast<uint16_t*>(&buf[11]) = htons(header.payloadLength);

    if (payload && payloadLen > 0) {
        std::memcpy(&buf[HEADER_SIZE], payload, payloadLen);
    }
    return buf;
}

// ====================== 역직렬화 (inline) ======================
inline bool DeserializeHeader(const uint8_t* buf, int bufLen, PacketHeader& outHeader)
{
    if (bufLen < HEADER_SIZE) return false;

    outHeader.type = buf[0];
    outHeader.roomId = ntohl(*reinterpret_cast<const int32_t*>(&buf[1]));
    outHeader.userId = ntohl(*reinterpret_cast<const int32_t*>(&buf[5]));
    outHeader.sequence = ntohs(*reinterpret_cast<const uint16_t*>(&buf[9]));
    outHeader.payloadLength = ntohs(*reinterpret_cast<const uint16_t*>(&buf[11]));

    return true;
}