#ifndef __CHAT_GRPC_CLIENT_H__
#define __CHAT_GRPC_CLIENT_H__
#include <condition_variable>
#include <grpcpp/grpcpp.h>
// #include <json/json.h>
#include <json/value.h>
#include <mutex>
#include <queue>

#include "config_mgr.h"
#include "data.h"
#include "message.grpc.pb.h"
#include "singleton.h"

class ChatConPool {
public:
    ChatConPool(size_t pool_size, const std::string& host, const std::string& port);
    ~ChatConPool();

    std::unique_ptr<message::ChatService::Stub> getConnection();
    void returnConnection(std::unique_ptr<message::ChatService::Stub> con);
    void close();
private:
    std::atomic<bool> m_stop;
    size_t m_pool_size;
    std::string m_host;
    std::string m_port;
    std::queue<std::unique_ptr<message::ChatService::Stub>> m_connections;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

class ChatGrpcClient : public Singleton<ChatGrpcClient> {
friend class Singleton<ChatGrpcClient>;
public:
    ~ChatGrpcClient() {}

    message::AddFriendRsp NotifyAddFriend(const std::string& server_ip, const message::AddFriendReq& req);
    message::AuthFriendRsp NotifyAuthFriend(const std::string& server_ip, const message::AuthFriendReq& req);
    bool getBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo>& user_info);
    message::TextChatMsgRsp NotifyTextChatMsg(const std::string& server_ip, const message::TextChatMsgReq& req, const Json::Value& rtvalue);

private:
    ChatGrpcClient();
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> m_pools;
};

#endif // __CHAT_GRPC_CLIENT_H__
