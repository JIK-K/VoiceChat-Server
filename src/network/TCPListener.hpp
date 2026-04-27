#pragma once
#define ASIO_STANDALONE
#include <asio.hpp>
#include <memory>
#include <functional>

using AcceptCallback = std::function<void(asio::ip::tcp::socket)>;

class TCPListener
{
public:
	TCPListener(asio::io_context& io, int port, AcceptCallback callback);
	void Start();

private:
	void Accept();
	asio::ip::tcp::acceptor _acceptor;
	AcceptCallback _callback;
};