#include "TCPListener.hpp"
#include <iostream>

// _acceptor 기본 생성자가 없어서 초기화 리스트 방식
TCPListener::TCPListener(asio::io_context& io, int port, AcceptCallback callback)
	: _acceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	, _callback(callback)
{
}

void TCPListener::Start() {
	std::cout << "TCP SERVER START - PORT : " << _acceptor.local_endpoint().port() << "\n";
	Accept();
}

void TCPListener::Accept() {
	// asio 내부 큐에 등록 해놓고 연결 오면 실행
	_acceptor.async_accept(
		[this](asio::error_code ec, asio::ip::tcp::socket socket) {
			if (!ec) {
				std::cout << "CLIENT CONNECT : " <<
					socket.remote_endpoint().address().to_string() << "\n";

				_callback(std::move(socket));
			}
			else {
				std::cout << "ACCEPT ERROR" << ec.message() << "\n";
			}

			Accept();
		}
	);
}

