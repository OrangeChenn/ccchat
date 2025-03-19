#include "session.h"
#include <cstring> // string.h
#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "const.h"
#include "logic_system.h"

Session::Session(boost::asio::io_context& ioc, ChatServer* server)
    : m_socket(ioc)
    , m_cserver(server)
    , m_close(false) {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    m_session_id = boost::uuids::to_string(uuid);
    m_recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

Session::~Session() {
    std::cout << "" << std::endl;
}

void Session::start() {
    asyncReadHead(HEAD_TOTAL_LEN);
}

void Session::asyncReadHead(int total_len) {
    std::shared_ptr<Session> self = shared_from_this();
    asyncReadFull(total_len, [self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
        try {
            // 读取头部信息错误
            if(ec) {
                std::cout << "Failed to read head, error is: " << ec.what() << std::endl;
                this->close();
                this->m_cserver->clearSession(this->m_session_id);
                return;
            }
            
            // 头部信息不匹配
            if(bytes_transfered < MAX_LENGTH) {
                std::cout << "read length not match, read[" << bytes_transfered << "], total["
                          << MAX_LENGTH << "]";
                this->close();
                this->m_cserver->clearSession(this->m_session_id);
                return;
            }

            this->m_recv_head_node->clear();
            memcpy(this->m_recv_head_node->m_data, this->m_data, bytes_transfered);

            // 判断msg_id合法性
            short msg_id = 0;
            memcpy(&msg_id, this->m_recv_head_node->m_data, HEAD_ID_LEN);
            msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
            std::cout << "msg_id: " << msg_id << std::endl;
            if(msg_id > MAX_LENGTH) {
                std::cout << "msg_id invalid, msg_id: " << msg_id << std::endl;
                this->close();
                this->m_cserver->clearSession(this->m_session_id);
                return;
            }

            // 判断msg_len合法性
            short msg_len = 0;
            memcpy(&msg_len, this->m_recv_head_node->m_data + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
            std::cout << "msg_len: " << msg_len << std::endl;
            if(msg_len > MAX_LENGTH) {
                std::cout << "msg_len invalid, msg_len: " << msg_len << std::endl;
                this->close();
                this->m_cserver->clearSession(this->m_session_id);
                return;
            }
            this->m_recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
            this->asyncReadBody(msg_len);
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
        }
    });
}

void Session::asyncReadFull(std::size_t max_len
        , std::function<void(const boost::system::error_code&, std::size_t)> handler) {
    ::memset(m_data, 0, MAX_LENGTH); // 清空，读取下一条信息
    asyncReadLen(0, max_len, handler);
}

void Session::asyncReadLen(std::size_t read_len, std::size_t total_len
        , std::function<void(const boost::system::error_code&, std::size_t)> handler) {
    std::shared_ptr<Session> self = shared_from_this();
    m_socket.async_read_some(boost::asio::buffer(m_data + read_len, total_len - read_len),
        [read_len, total_len, handler, self] 
                (const boost::system::error_code& ec, std::size_t bytes_transfered) {
            if(ec) {
                handler(ec, read_len + bytes_transfered);
                return;
            }

            // 头部信息读完
            if(read_len + bytes_transfered >= total_len) {
                handler(ec, read_len + bytes_transfered);
                return;
            }

            // 头部信息未读完，继续读取
            self->asyncReadLen(read_len + bytes_transfered, total_len, handler);
        }
    );
}

void Session::asyncReadBody(int total_len) {
    std::shared_ptr<Session> self = shared_from_this();
    try {
        asyncReadFull(total_len, [total_len, self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
            if(ec) {
                std::cout << "Failed to read msg body, error: " << ec.what() << std::endl;
                this->close();
                this->m_cserver->clearSession(this->m_session_id);
                return;
            }
            if(bytes_transfered < (std::size_t)total_len) {
                std::cout << "read data length not match, read[ " << bytes_transfered 
                          << "], total[" << total_len << "]" << std::endl;
                this->close();
                this->m_cserver->clearSession(this->m_session_id);
                return;
            }

            memcpy(this->m_recv_msg_node->m_data, this->m_data, bytes_transfered);
            this->m_recv_msg_node->m_cur_len += bytes_transfered;
            this->m_recv_msg_node->m_data[this->m_recv_msg_node->m_total_len] = '\0';
            std::cout << "recive data is: " << this->m_recv_msg_node->m_data << std::endl;

            LogicSystem::GetInstance()->postMsgToQue(shared_from_this(), this->m_recv_msg_node);
            this->asyncReadHead(HEAD_TOTAL_LEN);
        });
    } catch(const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}

void Session::send(const std::string& msg_data, short msg_id) {

}

void Session::setUserUid(int uid) {
    m_user_uid = uid;
}

int Session::getUserUid() {
    return m_user_uid;
}

void Session::close() {
    m_socket.close();
    m_close = true;
}

std::string Session::getSessionId() {
    return m_session_id;
}

tcp::socket& Session::getSocket() {
    return m_socket;
}
