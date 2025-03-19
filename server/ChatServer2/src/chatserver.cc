#include "chatserver.h"
#include <iostream>
#include "asio_io_context_pool.h"
#include "user_mgr.h"

ChatServer::ChatServer(boost::asio::io_context& io_context, short port) 
        : m_context(io_context)
        , m_port(port) 
        , m_acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)){

    std::cout << "Server start success, listen on port: " << port << std::endl;
    startAccept();
}

ChatServer::~ChatServer() {
    std::cout << "Server destrut listen on port: " << m_port << std::endl;
}

void ChatServer::clearSession(const std::string& uuid) {
    if(m_sessions.find(uuid) != m_sessions.end()) {
        UserMgr::GetInstance()->rmvUserSession(m_sessions[uuid]->getUserUid());
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.erase(uuid);
    }
}

void ChatServer::handleAccept(std::shared_ptr<Session> session, const boost::system::error_code& error) {
    if(!error) {
        session->start();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.insert(std::make_pair(session->getSessionId(), session));
    } else {
        std::cout << "session accept failed, error is: " << error.what() << std::endl;
    }
    startAccept();
}

void ChatServer::startAccept() {
    auto& ioc = AsioIOContextPool::GetInstance()->getIOContext();
    std::shared_ptr<Session> session = std::make_shared<Session>(ioc, this);
    m_acceptor.async_accept(session->getSocket(), std::bind(&ChatServer::handleAccept, this, session, std::placeholders::_1));
}
