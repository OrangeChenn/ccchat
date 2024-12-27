#include "gate_server.h"
#include "http_connection.h"

GateServer::GateServer(boost::asio::io_context& ioc, unsigned short& port) 
    : m_ioc(ioc), 
      m_acceptor(ioc, tcp::endpoint(tcp::v4(), port)),
      m_socket(ioc){

}

void GateServer::start() {
	auto self = shared_from_this();
	m_acceptor.async_accept(m_socket, [self](beast::error_code ec) {
		try {
			// 出错放弃这个连接，继续监听其他连接
			if(ec) {
				self->start();
				return;
			}
			// 创建新连接，并且创建HttpConnection类管理这个连接
			std::make_shared<HttpConnection>(std::move(self->m_socket))->start();
			self->start();
		}
		catch (const std::exception& e){
			std::cout << "Exception is " << e.what() << std::endl;
			self->start();
		}
	});
}

