#include "status_service_impl.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "const.h"
#include "redis_mgr.h"

std::string generate_unique_string() {
    // uuid 全球唯一标识符
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    std::string unique_string = boost::uuids::to_string(uuid);
    return unique_string;
}

StatusServiceImpl::StatusServiceImpl() {
    ConfigMgr& cfg = ConfigMgr::Inst();
    std::string server_list = cfg["ChatServers"]["Name"];
    std::stringstream ss(server_list);
    std::string tmp = "";
    ChatServer server;
    while(std::getline(ss, tmp, ',')) {
        if(tmp.empty()) {
            continue;
        }
        server.host = cfg[tmp]["Host"];
        server.port = cfg[tmp]["Port"];
        server.name = cfg[tmp]["Name"];
        m_servers[server.name] = server;
    }
}

grpc::Status StatusServiceImpl::GetChatServer(::grpc::ServerContext* context, 
                                              const ::message::GetChatServerReq* request,
                                              ::message::GetChatServerRsp* response) {

    // ChatServer& server = m_servers[m_server_index++]; // 可优化，实现负载均衡
    ChatServer server = getChatServer();
    response->set_host(server.host);
    response->set_port(server.port);
    response->set_error(ErrorCodes::SUCCESS);
    response->set_token(generate_unique_string());
    insertToken(request->uid(), response->token());
    return grpc::Status::OK;
}

grpc::Status StatusServiceImpl::Login(::grpc::ServerContext* context,
        const ::message::LoginReq* request, ::message::LoginRsp* response) {
    auto uid = request->uid();
    auto token = request->token();
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->get(token_key, token_value);
    if(!success) {
        response->set_error(ErrorCodes::UID_INVALID);
        return grpc::Status::OK;
    }
    if(token != token_value) {
        response->set_error(ErrorCodes::TOKEN_INVALID);
        return grpc::Status::OK;
    }
    response->set_error(ErrorCodes::SUCCESS);
    response->set_uid(uid);
    response->set_token(token);
    return grpc::Status::OK;
}

ChatServer StatusServiceImpl::getChatServer() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ChatServer min_server = m_servers.begin()->second;
    std::string count_str = RedisMgr::GetInstance()->hget(LOGIN_COUNT, min_server.name);
    if(count_str.empty()) {
        min_server.con_count = INT_MAX;
    } else {
        min_server.con_count = std::stoi(count_str);
    }

    for(auto& server : m_servers) {
        if(server.second.name == min_server.name) {
            continue;
        }
        std::string cnt_str = RedisMgr::GetInstance()->hget(LOGIN_COUNT, server.second.name);
        if(cnt_str.empty()) {
            server.second.con_count = INT_MAX;
        } else {
            server.second.con_count = std::stoi(cnt_str);
        }
        if(server.second.con_count < min_server.con_count) {
            min_server = server.second;
        }
    }
    return min_server;
}

void StatusServiceImpl::insertToken(int uid, std::string token) {
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid;
    RedisMgr::GetInstance()->set(token_key, token);
}
