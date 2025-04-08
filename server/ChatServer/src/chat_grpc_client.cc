#include "chat_grpc_client.h"
#include <json/value.h>
#include <json/reader.h>
#include "const.h"
#include "mysql_mgr.h"
#include "redis_mgr.h"

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
    rsp.set_error(ErrorCodes::SUCCESS);
    rsp.set_fromuid(req.fromuid());
    rsp.set_touid(req.touid());

    auto iter = m_pools.find(server_ip);
    if(iter == m_pools.end()) {
        rsp.set_error(ErrorCodes::SERVER_IP_INVALID);
        return rsp;
    }
    auto& pool = iter->second;
    auto stub = pool->getConnection();
    Defer defer([&pool, &stub] () {
        pool->returnConnection(std::move(stub));
    });

    grpc::ClientContext context;
    grpc::Status status = stub->NotifyAuthFriend(&context, req, &rsp);
    if(!status.ok()) {
        rsp.set_error(ErrorCodes::RPC_FAILED);
    }
    
    return rsp;
}

bool ChatGrpcClient::getBaseInfo(const std::string& base_key, int uid,
                                    std::shared_ptr<UserInfo>& userinfo) {
    //优先查redis中查询用户信息
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->get(base_key, info_str);
    if (b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->name = root["name"].asString();
        userinfo->passwd = root["passwd"].asString();
        userinfo->email = root["email"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
        std::cout << "user login uid is  " << userinfo->uid << " name  is "
            << userinfo->name << " pwd is " << userinfo->passwd << " email is " << userinfo->email << std::endl;
    }
    else {
        //redis中没有则查询mysql
        //查询数据库
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInstance()->getUser(uid);
        if (user_info == nullptr) {
            return false;
        }

        userinfo = user_info;

        //将数据库内容写入redis缓存
        Json::Value redis_root;
        redis_root["uid"] = uid;
        redis_root["passwd"] = userinfo->passwd;
        redis_root["name"] = userinfo->name;
        redis_root["email"] = userinfo->email;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisMgr::GetInstance()->set(base_key, redis_root.toStyledString());
    }
    return true;
}

message::TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(const std::string& server_ip,
                        const message::TextChatMsgReq& req, const Json::Value& rtvalue) {
    message::TextChatMsgRsp rsp;
    rsp.set_error(ErrorCodes::SUCCESS);

    rsp.set_fromuid(req.fromuid());
    rsp.set_touid(req.touid());
    for(const auto& obj : req.textmsgs()) {
        message::TextChatData* msg = rsp.add_textmsgs();
        msg->set_msgid(obj.msgid());
        msg->set_msgcontent(obj.msgcontent());
    }

    auto iter = m_pools.find(server_ip);
    if(iter == m_pools.end()) {
        return rsp;
    }

    auto& pool = iter->second;
    grpc::ClientContext context;
    auto stub = pool->getConnection();
    Defer defer([&stub, &pool] () {
        pool->returnConnection(std::move(stub));
    });

    grpc::Status status = stub->NotifyTextChatMsg(&context, req, &rsp);

    if(!status.ok()) {
        rsp.set_error(ErrorCodes::RPC_FAILED);
    }

    return rsp;
}
