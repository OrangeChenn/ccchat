#include "asio_io_context_pool.h"
#include "gate_server.h"
#include "http_connection.h"

GateServer::GateServer(boost::asio::io_context& ioc, unsigned short& port) 
    : m_ioc(ioc), 
      m_acceptor(ioc, tcp::endpoint(tcp::v4(), port)),
      m_socket(ioc){

}

void GateServer::start() {
	auto self = shared_from_this();
	auto& io_context = AsioIOContextPool::GetInstance()->getIOContext();
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(tcp::socket(io_context));
	m_acceptor.async_accept(m_socket, [self, new_con](beast::error_code ec) {
		try {
			// 出错放弃这个连接，继续监听其他连接
			if(ec) {
				self->start();
				return;
			}
			tcp::endpoint peer_ep = self->m_socket.remote_endpoint();
			std::cout << "New connection ip: " << peer_ep.address().to_string() << std::endl;
			// 创建新连接，并且创建HttpConnection类管理这个连接
			new_con->start();
			self->start();
		}
		catch (const std::exception& e){
			std::cout << "Exception is " << e.what() << std::endl;
			self->start();
		}
	});
}

