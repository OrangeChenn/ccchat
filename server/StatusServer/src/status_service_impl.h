#ifndef __STATUS_SERVICE_IMPL_H__
#define __STATUS_SERVICE_IMPL_H__

#include <grpcpp/grpcpp.h>
#include <string>

#include "config_mgr.h"
#include "message.grpc.pb.h"

class ChatServer {
public:
    ChatServer():host(""),port(""),name(""),con_count(0){}
    ChatServer(const ChatServer& cs):host(cs.host), port(cs.port), name(cs.name), con_count(cs.con_count){}
    ChatServer& operator=(const ChatServer& cs) {
        if (&cs == this) {
            return *this;
        }

        host = cs.host;
        name = cs.name;
        port = cs.port;
        con_count = cs.con_count;
        return *this;
    }

    std::string host;
    std::string port;
    std::string name;
    int con_count;
};

class StatusServiceImpl final : public message::StatusService::Service {
public:
    StatusServiceImpl();
    grpc::Status GetChatServer(::grpc::ServerContext* context, 
        const ::message::GetChatServerReq* request, ::message::GetChatServerRsp* response) override;
    grpc::Status Login(::grpc::ServerContext* context,
        const ::message::LoginReq* request, ::message::LoginRsp* response) override;

private:
    ChatServer getChatServer();
    void insertToken(int uid, std::string token);

    std::unordered_map<std::string, ChatServer> m_servers;
    std::mutex m_mutex;
    // int m_server_index;
};

#endif // __STATUS_SERVICE_IMPL_H__
