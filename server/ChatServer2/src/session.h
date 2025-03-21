#ifndef __SESSION_H__
#define __SESSION_H__

#include <boost/asio.hpp>
#include "chatserver.h"
#include "const.h"
#include "logic_system.h"
#include "msg_node.h"

using tcp = boost::asio::ip::tcp;

class ChatServer;
class RecvNode;
class MsgNode;
class SendNode;
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_context& ioc, ChatServer* server);
    ~Session();

    tcp::socket& getSocket();
    void setUserUid(int uid);
    int getUserUid();
    void start();
    void close();
    void send(char* msg, short msg_len, short msg_id);
    void send(const std::string& msg_data, short msg_id);
    std::string getSessionId();
    
private:
    void asyncReadHead(int total_len);
    void asyncReadFull(std::size_t max_len
            , std::function<void(const boost::system::error_code&, std::size_t)> handler);
    void asyncReadLen(std::size_t read_len, std::size_t total_len
            , std::function<void(const boost::system::error_code&, std::size_t)> handler);
    void asyncReadBody(int total_len);

    void handlerWrite(const boost::system::error_code& error, std::shared_ptr<Session> shared_self);

private:
    tcp::socket m_socket;
    ChatServer* m_cserver;
    std::string m_session_id;
    char m_data[MAX_LENGTH];
    std::queue<std::shared_ptr<SendNode>> m_send_que;
    std::mutex m_send_mutex;
    bool m_close;
    std::shared_ptr<RecvNode> m_recv_msg_node;
    std::shared_ptr<MsgNode> m_recv_head_node;
    int m_user_uid;
};

class LogicNode {
friend class LogicSystem;
public:
    LogicNode(std::shared_ptr<Session> session, std::shared_ptr<RecvNode> recvnode);
private:
    std::shared_ptr<Session> m_session;
    std::shared_ptr<RecvNode> m_recvnode;
};

#endif // __SESSION_H__
