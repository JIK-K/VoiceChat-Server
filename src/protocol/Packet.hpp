#pragma once
#include <cstdint>  // uint8_t, int32_t, uint16_t 고정 크기 타입
#include <vector>   // std::vector
#include <cstring>  // memcpy

// 구조체 패딩 제거 → 정확히 13바이트 보장
// 없으면 컴파일러가 자동으로 빈 바이트를 삽입해서 크기가 달라짐
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  type;          // 패킷 종류 (0x01: TCP 제어, 0x02: UDP 음성)
    int32_t  roomId;        // 대상 방 번호
    int32_t  userId;        // 송신자 ID
    uint16_t sequence;      // 패킷 순서 번호 (음성 패킷 순서 보정용)
    uint16_t payloadLength; // payload 길이 (헤더 뒤에 오는 데이터 크기)
};
#pragma pack(pop)

constexpr uint8_t PACKET_TYPE_CONTROL = 0x01; // TCP 제어 패킷 (login, join, leave 등)
constexpr uint8_t PACKET_TYPE_VOICE = 0x02; // UDP 음성 패킷 (Opus 인코딩 데이터)
constexpr int     HEADER_SIZE = 13;   // PacketHeader 고정 크기 (바이트)

// 직렬화: 헤더 + payload → 전송할 바이트 배열로 변환
// header    : 전송할 헤더 구조체
// payload   : 전송할 데이터 (JSON 문자열 또는 음성 데이터)
// payloadLen: payload 길이
std::vector<uint8_t> SerializePacket(const PacketHeader& header,
    const uint8_t* payload,
    uint16_t payloadLen);

// 역직렬화: 수신한 바이트 배열 → 헤더 구조체로 파싱
// buf      : 수신한 바이트 배열
// bufLen   : 버퍼 길이 (13바이트 미만이면 false 반환)
// outHeader: 파싱 결과를 담을 구조체 (함수 안에서 값을 채워줌)
bool DeserializeHeader(const uint8_t* buf, int bufLen, PacketHeader& outHeader);