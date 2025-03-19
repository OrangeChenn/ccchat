#ifndef __CHATSERVER_H__
#define __CHATSERVER_H__

#include <boost/asio.hpp>
#include <memory>
#include <map>
#include <mutex>

#include "session.h"

class Session;
class ChatServer {
public:
    ChatServer(boost::asio::io_context& io_context, short port);
    ~ChatServer();

    void clearSession(const std::string& uuid);

private:
    void handleAccept(std::shared_ptr<Session> session, const boost::system::error_code& error);
    void startAccept();

private:
    boost::asio::io_context &m_context;
    short m_port;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::map<std::string, std::shared_ptr<Session>> m_sessions;
    std::mutex m_mutex;
};

#endif // __CHATSERVER_H__
