#ifndef __GATESERVER_H__
#define __GATESERVER_H__

#include "const.h"

class GateServer : public std::enable_shared_from_this<GateServer>
{
public:
	GateServer(boost::asio::io_context& ioc, unsigned short& port);
	void start();
private:
	net::io_context& m_ioc;
	tcp::acceptor m_acceptor;
};

#endif // __GATEsERVER_H__
