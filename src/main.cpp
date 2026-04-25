#pragma comment(lib, "ws2_32.lib") // Asio가 내부적으로 Winsock 사용하므로 링크 필요
#define ASIO_STANDALONE            // Boost 없이 Asio 단독 사용
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include "protocol/Packet.hpp"
#include <iostream>

using json = nlohmann::json;

int main()
{
    // Asio 이벤트 루프 (모든 비동기 작업의 중심)
    asio::io_context io;

    // TCP 소켓 생성 + 포트 9000 바인딩 + listen 한 번에 처리
    asio::ip::tcp::acceptor acceptor(
        io,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 9000)
    );

    std::cout << "서버 시작 - 포트 9000 대기 중...\n";

    // 클라이언트 연결 수락 (연결 올 때까지 여기서 블로킹)
    asio::ip::tcp::socket clientSocket(io);
    acceptor.accept(clientSocket);

    std::cout << "클라이언트 연결됨: "
        << clientSocket.remote_endpoint().address().to_string() << "\n";

    // ── 헤더 수신 ──────────────────────────────────────────
    // 정확히 13바이트만 읽음 (헤더 크기 고정)
    uint8_t headerBuf[HEADER_SIZE];
    asio::read(clientSocket, asio::buffer(headerBuf, HEADER_SIZE));

    // 바이트 배열 → PacketHeader 구조체로 파싱
    PacketHeader header;
    if (!DeserializeHeader(headerBuf, HEADER_SIZE, header)) {
        std::cout << "헤더 파싱 실패\n";
        return 1;
    }

    // ── payload 수신 ────────────────────────────────────────
    // 헤더의 payloadLength만큼 추가로 읽음
    std::vector<uint8_t> payloadBuf(header.payloadLength);
    asio::read(clientSocket, asio::buffer(payloadBuf));

    // ── JSON 파싱 ───────────────────────────────────────────
    // payload를 문자열로 변환 후 JSON 파싱
    std::string jsonStr(payloadBuf.begin(), payloadBuf.end());
    json j = json::parse(jsonStr);

    std::cout << "수신한 cmd: " << j["cmd"] << "\n";
    std::cout << "userId: " << j["userId"] << "\n";

    // ── 응답 전송 ───────────────────────────────────────────
    // 응답 JSON 생성
    json response = { {"result", "ok"}, {"msg", "login success"} };
    std::string responseStr = response.dump();

    // 응답 헤더 구성
    PacketHeader responseHeader;
    responseHeader.type = PACKET_TYPE_CONTROL;
    responseHeader.roomId = 0;
    responseHeader.userId = j["userId"];
    responseHeader.sequence = 1;
    responseHeader.payloadLength = static_cast<uint16_t>(responseStr.size());

    // 헤더 + JSON payload → 바이트 배열로 직렬화
    auto packet = SerializePacket(
        responseHeader,
        reinterpret_cast<const uint8_t*>(responseStr.data()),
        responseHeader.payloadLength
    );

    // 전송
    asio::write(clientSocket, asio::buffer(packet));
    std::cout << "응답 전송 완료\n";

    return 0;
}