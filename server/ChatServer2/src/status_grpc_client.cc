#include "status_grpc_client.h"
#include "const.h"

StatusConPool::StatusConPool(size_t pool_size, const std::string& host, const std::string& port)
    : m_stop(false)
    , m_pool_size(pool_size)
    , m_host(host)
    , m_port(port) {
    for(std::size_t i = 0; i < pool_size; ++i) {
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
        if(this->m_stop) {
            return true;
        }
        return !this->m_connections.empty();
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

StatusGrpcClient::StatusGrpcClient() {
    auto& cfg_mgr = ConfigMgr::Inst();
    std::string host = cfg_mgr["StatusServer"]["Host"];
    std::string port = cfg_mgr["StatusServer"]["Port"];
    m_pool.reset(new StatusConPool(5, host, port));
}

StatusGrpcClient::~StatusGrpcClient() {

}

message::LoginRsp StatusGrpcClient::Login(int uid, const std::string& token) {
    grpc::ClientContext context;
    message::LoginReq req;
    message::LoginRsp rsq;
    req.set_uid(uid);
    req.set_token(token.c_str());
    auto stub = m_pool->getConnection();
    grpc::Status status = stub->Login(&context, req, &rsq);
    if(status.ok()) {
        m_pool->returnConnection(std::move(stub));
        return rsq;
    } else {
        rsq.set_error(ErrorCodes::RPC_FAILED);
        return rsq;
    }
}
