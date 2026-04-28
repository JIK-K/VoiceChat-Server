#pragma once

#include <cstdint>
#include <vector>
#include <cstring>   // memcpy

// 정확히 13바이트 보장
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  type;
    int32_t  roomId;
    int32_t  userId;
    uint16_t sequence;
    uint16_t payloadLength;
};
#pragma pack(pop)

constexpr uint8_t PACKET_TYPE_CONTROL = 0x01;
constexpr uint8_t PACKET_TYPE_VOICE = 0x02;
constexpr int HEADER_SIZE = 13;

// ====================== 직렬화 ======================
inline std::vector<uint8_t> SerializePacket(const PacketHeader& header,
    const uint8_t* payload,
    uint16_t payloadLen)
{
    std::vector<uint8_t> buf(HEADER_SIZE + payloadLen);

    buf[0] = header.type;
    std::memcpy(&buf[1], &header.roomId, 4);
    std::memcpy(&buf[5], &header.userId, 4);
    std::memcpy(&buf[9], &header.sequence, 2);
    std::memcpy(&buf[11], &header.payloadLength, 2);

    if (payload && payloadLen > 0) {
        std::memcpy(&buf[13], payload, payloadLen);
    }
    return buf;
}

// ====================== 역직렬화 (Windows Little-Endian 직접 복사) ======================
inline bool DeserializeHeader(const uint8_t* buf, int bufLen, PacketHeader& outHeader)
{
    if (bufLen < HEADER_SIZE) return false;

    outHeader.type = buf[0];
    std::memcpy(&outHeader.roomId, &buf[1], 4);
    std::memcpy(&outHeader.userId, &buf[5], 4);
    std::memcpy(&outHeader.sequence, &buf[9], 2);
    std::memcpy(&outHeader.payloadLength, &buf[11], 2);

    return true;
}