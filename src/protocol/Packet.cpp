#include "Packet.hpp"

// 헤더 + payload를 하나의 바이트 배열로 합쳐서 반환
// 네트워크로 전송하기 직전에 호출
std::vector<uint8_t> SerializePacket(const PacketHeader& header,
    const uint8_t* payload,
    uint16_t payloadLen)
{
    // 헤더(13바이트) + payload 크기만큼 버퍼 할당
    std::vector<uint8_t> buf(HEADER_SIZE + payloadLen);

    // 헤더 구조체를 버퍼 앞쪽에 복사
    // pack(1) 덕분에 구조체 메모리 레이아웃 = 전송할 바이트 순서와 동일
    std::memcpy(buf.data(), &header, HEADER_SIZE);

    // payload가 있으면 헤더 뒤에 이어서 복사
    if (payload && payloadLen > 0)
        std::memcpy(buf.data() + HEADER_SIZE, payload, payloadLen);

    return buf;
}

// 수신한 바이트 배열에서 헤더만 파싱해서 outHeader에 채워줌
// payload는 여기서 처리 안 함 → 호출부에서 header.payloadLength만큼 추가로 읽어야 함
bool DeserializeHeader(const uint8_t* buf, int bufLen, PacketHeader& outHeader)
{
    // 13바이트 미만이면 헤더가 잘린 것 → 잘못된 패킷
    if (bufLen < HEADER_SIZE)
        return false;

    // 버퍼 앞 13바이트를 구조체로 복사
    std::memcpy(&outHeader, buf, HEADER_SIZE);
    return true;
}