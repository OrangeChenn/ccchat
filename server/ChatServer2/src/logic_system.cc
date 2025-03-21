#include "logic_system.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "config_mgr.h"
#include "const.h"
#include "mysql_mgr.h"
#include "redis_mgr.h"
#include "status_grpc_client.h"
#include "user_mgr.h"

LogicSystem::LogicSystem() : m_stop(false) {
    registerCallBacks();
    m_work_thread = std::thread(&LogicSystem::dealMsg, this);
}

LogicSystem::~LogicSystem() {
    m_stop = true;
    m_cond.notify_one();
    m_work_thread.join();
}

void LogicSystem::postMsgToQue(std::shared_ptr<LogicNode> node) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_msg_que.push(node);
    if(m_msg_que.size() >= 1) {
        lock.unlock();
        m_cond.notify_one();
    }
}

void LogicSystem::logicHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    int uid = root["uid"].asInt();
    std::string token = root["token"].asString();
    std::cout << "user login uid is: " << uid
              << ", token is: " << token << std::endl;
    // session->send(root.toStyledString(), msg_id);

    // message::LoginRsp rsq = 
    //         StatusGrpcClient::GetInstance()->Login(root["uid"].asInt(), root["token"].asString());

    Json::Value rt_value;
    Defer defer([session, &rt_value]() {
        session->send(rt_value.toStyledString(), MSG_CHAT_LOGIN_RSP);
    });

    // 从redis获取token判断是否正确
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->get(token_key, token_value);
    if(!success) {
        rt_value["error"] = ErrorCodes::UID_INVALID;
        return;
    }
    if(token_value != token) {
        rt_value["error"] = ErrorCodes::TOKEN_INVALID;
        return;
    }

    rt_value["error"] = ErrorCodes::SUCCESS;

    std::string base_key = USER_BASE_INFO + uid_str;
    auto user_info = std::make_shared<UserInfo>();
    bool b_base = getBaseInfo(base_key, uid, user_info);
    if(!b_base) {
        rt_value["error"] = ErrorCodes::UID_INVALID;
        return;
    }

    rt_value["uid"] = uid;
    rt_value["passwd"] = user_info->passwd;
    rt_value["name"] = user_info->name;
    rt_value["email"] = user_info->email;
    rt_value["nick"] = user_info->nick;
    rt_value["desc"] = user_info->desc;
    rt_value["sex"] = user_info->sex;
    rt_value["icon"] = user_info->icon;

    // 增加本服务器的登录数量
    std::string server_name = ConfigMgr::Inst().getValue("SelfServer", "Name");
    std::string rd_res = RedisMgr::GetInstance()->hget(LOGIN_COUNT, server_name);
    int count = 0;
    if(!rd_res.empty()) {
        count = std::stoi(rd_res);
    }
    ++count;
    RedisMgr::GetInstance()->hset(LOGIN_COUNT, server_name, std::to_string(count));

    std::string ipkey = USERIPPREFIX + uid_str;
    RedisMgr::GetInstance()->set(ipkey, server_name);

    UserMgr::GetInstance()->setUserSession(uid, session);
}

void LogicSystem::registerCallBacks() {
    m_fun_callbacks[MSG_CHAT_LOGIN_RSP] = std::bind(&LogicSystem::logicHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::dealMsg() {
    for(;;) {
        std::unique_lock<std::mutex> lock(m_mutex);
        while(m_msg_que.empty() && !m_stop) {
            m_cond.wait(lock);
        }

        // 服务器如果关闭了，处理全部
        if(m_stop) {
            while(!m_msg_que.empty()) {
                std::shared_ptr<LogicNode> node = m_msg_que.front();
                short msg_id = node->m_recvnode->m_msg_id;
                std::cout << "recv_msg id is " << msg_id << std::endl;
                if(m_fun_callbacks.find(msg_id) == m_fun_callbacks.end()) {
                    m_msg_que.pop();
                    continue;
                }
                m_fun_callbacks[msg_id](node->m_session, msg_id,
                    std::string(node->m_recvnode->m_data, node->m_recvnode->m_cur_len));
                m_msg_que.pop();
            }
            break;
        }

        // 处理单条
        std::shared_ptr<LogicNode> node = m_msg_que.front();
        short msg_id = node->m_recvnode->m_msg_id;
        std::cout << "recv_msg id is " << msg_id << std::endl;
        if(m_fun_callbacks.find(msg_id) == m_fun_callbacks.end()) {
            m_msg_que.pop();
            std::cout << "msg id [" << msg_id << "] handler not found" << std::endl;
            continue;
        }
        m_fun_callbacks[msg_id](node->m_session, msg_id,
            std::string(node->m_recvnode->m_data, node->m_recvnode->m_cur_len));
        m_msg_que.pop();
    }
}

bool LogicSystem::getBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo> &user_info) {
    return true;
}
