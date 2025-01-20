#include "status_service_impl.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "const.h"

std::string generate_unique_string() {
    // uuid 全球唯一标识符
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    std::string unique_string = boost::uuids::to_string(uuid);
    return unique_string;
}

StatusServiceImpl::StatusServiceImpl() : m_server_index(0) {
    ConfigMgr& cfg_mgr = ConfigMgr::Inst();
    ChatServer server;
    server.host = cfg_mgr["ChatServer1"]["Host"];
    server.port = cfg_mgr["ChatServer1"]["Port"];
    m_servers.push_back(server);
    server.host = cfg_mgr["ChatServer2"]["Host"];
    server.port = cfg_mgr["ChatServer2"]["Host"];
    m_servers.push_back(server);
}

grpc::Status StatusServiceImpl::GetChatServer(::grpc::ServerContext* context, 
                                              const ::message::GetChatServerReq* request,
                                              ::message::GetChatServerRsp* response) {
    if(m_server_index == (int)m_servers.size()) {
        m_server_index = 0;
    }
    ChatServer& server = m_servers[m_server_index++];
    response->set_host(server.host);
    response->set_port(server.port);
    response->set_error(ErrorCodes::SUCCESS);
    response->set_token(generate_unique_string());
    return grpc::Status::OK;
}
