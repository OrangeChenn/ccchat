#include "chat_grpc_client.h"
#include "const.h"

ChatConPool::ChatConPool(size_t pool_size, const std::string& host, const std::string& port)
    : m_stop(false)
    , m_pool_size(pool_size)
    , m_host(host)
    , m_port(port)    
{
    for(size_t i = 0; i < pool_size; ++i) {
        std::shared_ptr<grpc::Channel> channel =
            grpc::CreateChannel(m_host + ":" + m_port, grpc::InsecureChannelCredentials());
        m_connections.push(message::ChatService::NewStub(channel));
    }
}

ChatConPool::~ChatConPool() {
    std::lock_guard<std::mutex> lock(m_mutex);
    close();
    while(!m_connections.empty()) {
        m_connections.pop();
    }
}

std::unique_ptr<message::ChatService::Stub> ChatConPool::getConnection() {
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
    std::unique_ptr<message::ChatService::Stub> stub = std::move(m_connections.front());
    m_connections.pop();
    return stub;
}

void ChatConPool::returnConnection(std::unique_ptr<message::ChatService::Stub> con) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_stop) {
        return;
    }
    m_connections.push(std::move(con));
    m_cond.notify_one();
}

void ChatConPool::close() {
    m_stop = true;
    m_cond.notify_all();
}


ChatGrpcClient::ChatGrpcClient() {
    auto& cfg = ConfigMgr::Inst();
    auto server_list = cfg["PeerServer"]["Servers"];
    
    std::vector<std::string> words;
    std::string tmp_word;
    std::stringstream ss(server_list);
    while(std::getline(ss, tmp_word, ',')) {
        words.push_back(tmp_word);
    }

    for(auto& word : words) {
        if(word.empty()) {
            continue;
        }
        m_pools[cfg[word]["Name"]] =
            std::make_unique<ChatConPool>(5, cfg[word]["Host"], cfg[word]["Port"]);
    }
}

message::AddFriendRsp ChatGrpcClient::NotifyAddFriend(const std::string& server_ip,
                                                    const message::AddFriendReq& req) {
    message::AddFriendRsp rsp;
    grpc::ClientContext context;
    rsp.set_applyuid(req.applyuid());
    rsp.set_touid(req.touid());
    auto find_iter = m_pools.find(server_ip);
    // 目标服务器不存在
    if(find_iter == m_pools.end()) {
        rsp.set_error(ErrorCodes::SERVER_IP_INVALID);
        return rsp;
    }

    auto& pool = find_iter->second;
    auto stub = pool->getConnection();
    Defer defer([&pool, &stub]() {
        pool->returnConnection(std::move(stub));
    });
    grpc::Status status = stub->NotifyAddFriend(&context, req, &rsp);
    if(!status.ok()) {
        rsp.set_error(ErrorCodes::RPC_FAILED);
        return rsp;
    }

    rsp.set_error(ErrorCodes::SUCCESS);
    return rsp;
}

message::AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(const std::string& server_ip,
                                                    const message::AuthFriendReq& req) {
    message::AuthFriendRsp rsp;
    return rsp;
}

bool ChatGrpcClient::getBaseInfo(const std::string& base_key, int uid,
                                    std::shared_ptr<UserInfo>& user_info) {
    return true;
}

message::TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(const std::string& server_ip,
                        const message::TextChatMsgReq& req, const Json::Value& rtvalue) {
    message::TextChatMsgRsp rsp;
    return rsp;
}
