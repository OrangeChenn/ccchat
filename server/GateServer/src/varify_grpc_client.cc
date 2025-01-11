#include "varify_grpc_client.h"
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "config_mgr.h"


/****************************************************************** 
 *                       RPCPool
******************************************************************/
RPCPool::RPCPool(size_t pool_size, const std::string& host, const std::string& port) 
    :   m_stop(false),
        m_pool_size(pool_size),
        m_host(host),
        m_port(port) {
    for(size_t i = 0; i < pool_size; ++i) {
        std::shared_ptr<grpc::Channel> channel = 
                grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
        m_connections.push(message::VarifyService::NewStub(channel));
    }
}

RPCPool::~RPCPool() {
    std::lock_guard lock(m_mutex);
    close();
    while(!m_connections.empty()) {
        m_connections.pop();
    }
}

std::unique_ptr<message::VarifyService::Stub> RPCPool::getConnection() {
    std::unique_lock lock(m_mutex);
    m_cond.wait(lock, [this]() {
        if(m_stop) {
            return true;
        }
        return !this->m_connections.empty();
    });
    if(m_stop) {
        return nullptr;
    }
    auto stub = std::move(m_connections.front());
    m_connections.pop();
    return stub;
}

void RPCPool::returnConnection(std::unique_ptr<message::VarifyService::Stub> stub) {
    std::lock_guard lock(m_mutex);
    if(m_stop) {
        return;
    }
    m_connections.push(std::move(stub));
    m_cond.notify_one();
}

void RPCPool::close() {
    m_stop = true;
    m_cond.notify_all();
}

/****************************************************************** 
 *                      VarifyGrpcClient
******************************************************************/
VarifyGrpcClient::VarifyGrpcClient() {
    auto& cfg_mgr = ConfigMgr::Inst();
    std::string host = cfg_mgr["VarifyServer"]["Host"];
    std::string port = cfg_mgr["VarifyServer"]["Port"];
    std::cout << "host: " << host << ", port: " << port << std::endl; 
    m_pool.reset(new RPCPool(5, host, port));
}

message::GetVarifyRsp VarifyGrpcClient::getVarifyCode(const std::string& email) {
    grpc::ClientContext context;
    message::GetVarifyReq request;
    message::GetVarifyRsp response;
    request.set_email(email);
    auto stub = m_pool->getConnection();
    grpc::Status status = stub->GetVarifyCode(&context, request, &response);
    if(status.ok()) {
        m_pool->returnConnection(std::move(stub));
        return response;
    }
    m_pool->returnConnection(std::move(stub));
    response.set_error(ErrorCodes::RPC_FAILED);
    return response;
}
