#ifndef __STATUS_SERVICE_IMPL_H__
#define __STATUS_SERVICE_IMPL_H__

#include <grpcpp/grpcpp.h>
#include <vector>

#include "config_mgr.h"
#include "message.grpc.pb.h"

struct ChatServer {
    std::string host;
    std::string port;
};

class StatusServiceImpl final : public message::StatusService::Service {
public:
    StatusServiceImpl();
    grpc::Status GetChatServer(::grpc::ServerContext* context, 
        const ::message::GetChatServerReq* request, ::message::GetChatServerRsp* response) override;

    std::vector<ChatServer> m_servers;
    int m_server_index;
};

#endif // __STATUS_SERVICE_IMPL_H__
