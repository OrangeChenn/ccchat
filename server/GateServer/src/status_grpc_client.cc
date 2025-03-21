#include "status_grpc_client.h"
#include "config_mgr.h"
#include "const.h"

StatusConPool::StatusConPool(size_t pool_size, const std::string& host, const std::string& port)
    : m_stop(false)
    , m_pool_size(pool_size)
    , m_host(host)
    , m_port(port) {
    for(size_t i = 0; i < pool_size; ++i) {
        std::shared_ptr<grpc::Channel> channel =
            grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
        m_connections.push(message::StatusService::NewStub(channel));
    }
}

StatusConPool::~StatusConPool() {
    std::lock_guard<std::mutex> lock(m_mutex);
    close();
    while(!m_connections.empty()) {
        m_connections.pop();
    }
}

std::unique_ptr<message::StatusService::Stub> StatusConPool::getConnection() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]() {
        if(m_stop) {
            return true;
        }
        return !m_connections.empty();
    });
    if(m_stop) {
        return nullptr;
    }
    std::unique_ptr<message::StatusService::Stub> stub(std::move(m_connections.front()));
    m_connections.pop();
    return stub;
}

void StatusConPool::returnConnection(std::unique_ptr<message::StatusService::Stub> stub) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_stop) {
        return;
    }
    m_connections.push(std::move(stub));
    m_cond.notify_one();
}

void StatusConPool::close() {
    m_stop = true;
    m_cond.notify_all();
}

//
StatusGrpcClient::StatusGrpcClient() {
    auto& cfg_mgr = ConfigMgr::Inst();
    std::string host = cfg_mgr["StatusServer"]["Host"];
    std::string port = cfg_mgr["StatusServer"]["Port"];
    m_pool.reset(new StatusConPool(5, host, port));
}

StatusGrpcClient::~StatusGrpcClient() {

}

message::GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
    std::cout << "uid: " << uid << ", StatusGrpcClient::GetChatServer" << std::endl;
    grpc::ClientContext context;
    message::GetChatServerReq req;
    message::GetChatServerRsp rsp;
    req.set_uid(uid);
    auto stub = m_pool->getConnection();
    grpc::Status status = stub->GetChatServer(&context, req, &rsp);
    if(status.ok()) {
        m_pool->returnConnection(std::move(stub));
        return rsp;
    } else {
        rsp.set_error(ErrorCodes::RPC_FAILED);
        return rsp;
    }
}
