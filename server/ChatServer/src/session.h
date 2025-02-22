#ifndef __SESSION_H__
#define __SESSION_H__

#include <boost/asio.hpp>
#include "chatserver.h"
#include "const.h"
#include "msg_node.h"

using tcp = boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_context& ioc, ChatServer* server);
    ~Session();

    tcp::socket& getSocket();
    void start();
    void close();
    void send(const std::string& msg_data, short msg_id);
    std::string getSessionId();
    
private:
    void asyncReadHead(int total_len);
    void asyncReadFull(std::size_t max_len
            , std::function<void(const boost::system::error_code&, std::size_t)> handler);
    void asyncReadLen(std::size_t read_len, std::size_t total_len
            , std::function<void(const boost::system::error_code&, std::size_t)> handler);
    void asyncReadBody(int total_len);

private:
    tcp::socket m_socket;
    ChatServer* m_cserver;
    std::string m_session_id;
    char m_data[MAX_LENGTH];
    bool m_close;
    std::shared_ptr<RecvNode> m_recv_msg_node;
    std::shared_ptr<MsgNode> m_recv_head_node;
};

class LogicNode {
public:

private:
    std::shared_ptr<Session> m_session;
};

#endif // __SESSION_H__
