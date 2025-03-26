#include "logic_system.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "config_mgr.h"
#include "const.h"
#include "mysql_mgr.h"
#include "redis_mgr.h"
#include "chat_grpc_client.h"
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

    // 从数据库获取申请列表
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    bool b_apply = getFriendApplyInfo(uid, apply_list);
    if(b_apply) {
        for(auto& apply : apply_list) {
            Json::Value obj;
            obj["name"] = apply->_name;
            obj["uid"] = apply->_uid;
            obj["icon"] = apply->_icon;
            obj["nick"] = apply->_nick;
            obj["sex"] = apply->_sex;
            obj["desc"] = apply->_desc;
            obj["status"] = apply->_status;
            rt_value["apply_list"].append(obj);
        }
    }

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

void LogicSystem::searchInfo(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    std::string uid_str = root["uid"].asString();
    std::cout << "user Searchinfo uid is " << uid_str << std::endl;
    Json::Value rtvalue;
    Defer defer([this, &rtvalue, session]() {
        session->send(rtvalue.toStyledString(), ID_SEARCH_USER_RSP);
    });

    bool b_digit = isPureDigit(uid_str);
    if(b_digit) {
        getUserByUid(uid_str, rtvalue);
    } else {
        getUserByName(uid_str, rtvalue);
    }
}

void LogicSystem::addFriendApply(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    int uid = root["uid"].asInt();
    std::string applyname = root["applyname"].asString();
    std::string bakname = root["bakname"].asString();
    int touid = root["touid"].asInt();
    std::cout << "user login uid is  " << uid << " applyname  is "
        << applyname << " bakname is " << bakname << " touid is " << touid << std::endl;

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::SUCCESS;
    Defer defer([session, msg_id, &rtvalue]() {
        session->send(rtvalue.toStyledString(), msg_id);
    });

    // 更新数据库
    MysqlMgr::GetInstance()->addFriendApply(uid, touid);

    // 从redis查找touid对应的server ip
    std::string touid_str = std::to_string(touid);
    std::string touid_key = USERIPPREFIX + touid_str;
    std::string touid_server = "";
    bool b_get_server = RedisMgr::GetInstance()->get(touid_key, touid_server);
    if(!b_get_server) {
        return;
    }

    auto& cfg = ConfigMgr::Inst();
    std::string self_name = cfg["SelfServer"]["Name"];

    // 如果是连接在本ChatServer上直接通知
    if(self_name == touid_server) {
        std::shared_ptr<Session> session = UserMgr::GetInstance()->getSession(touid);
        if(session) {
            Json::Value notify;
            notify["error"] = ErrorCodes::SUCCESS;
            notify["uid"] = uid;
            notify["name"] = applyname;
            notify["desc"] = "";
            session->send(notify.toStyledString(), ID_NOTIFY_ADD_FRIEND_REQ);
        }
        return;
    }

    std::string base_key = USER_BASE_INFO + std::to_string(uid);
    std::shared_ptr<UserInfo> apply_info = std::make_shared<UserInfo>();
    bool ok = getBaseInfo(base_key, uid, apply_info);

    message::AddFriendReq add_req;
    add_req.set_applyuid(uid);
    add_req.set_touid(touid);
    add_req.set_name(applyname);
    add_req.set_desc("");
    if(ok) {
        add_req.set_icon(apply_info->icon);
        add_req.set_nick(apply_info->nick);
        add_req.set_sex(apply_info->sex);
    }
 
    // grpc通知目标服务器
    ChatGrpcClient::GetInstance()->NotifyAddFriend(touid_server, add_req);
}

void LogicSystem::registerCallBacks() {
    // 登录回调处理
    m_fun_callbacks[MSG_CHAT_LOGIN_RSP] = std::bind(&LogicSystem::logicHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // 查询好友处理
    m_fun_callbacks[ID_SEARCH_USER_RSP] = std::bind(&LogicSystem::searchInfo, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // 好友申请处理
    m_fun_callbacks[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::addFriendApply, this,
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

void LogicSystem::getUserByUid(const std::string& uid_str, Json::Value& rtvalue) {
    rtvalue["error"] = ErrorCodes::SUCCESS;
    std::string base_key = USER_BASE_INFO + uid_str;
    std::string info_str = "";
    bool b_find_redis = RedisMgr::GetInstance()->get(base_key, info_str);
    if(b_find_redis) { // 从redis中查到用户信息
        Json::Value root;
        Json::Reader reader;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto passwd = root["passwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << passwd << " email is " << email <<" icon is " << icon << std::endl;

		rtvalue["uid"] = uid;
		rtvalue["passwd"] = passwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return;
    } else { // redis中没有用户信息，从mysql数据库中查找
        int uid = std::stoi(uid_str);
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInstance()->getUser(uid);
        if(user_info == nullptr) { // 数据库中也不存在，uid无效
            rtvalue["error"] = ErrorCodes::UID_INVALID;
            return;
        }
        Json::Value redis_root;
        redis_root["uid"] = user_info->uid;
        redis_root["passwd"] = user_info->passwd;
        redis_root["name"] = user_info->name;
        redis_root["email"] = user_info->email;
        redis_root["nick"] = user_info->nick;
        redis_root["desc"] = user_info->desc;
        redis_root["sex"] = user_info->sex;
        redis_root["icon"] = user_info->icon;
    
        // 存到redis，方便下次查询
        RedisMgr::GetInstance()->set(base_key, redis_root.toStyledString());

        rtvalue["uid"] = user_info->uid;
        rtvalue["passwd"] = user_info->passwd;
        rtvalue["name"] = user_info->name;
        rtvalue["email"] = user_info->email;
        rtvalue["nick"] = user_info->nick;
        rtvalue["desc"] = user_info->desc;
        rtvalue["sex"] = user_info->sex;
        rtvalue["icon"] = user_info->icon;
    }
}

void LogicSystem::getUserByName(const std::string& name, Json::Value& rtvalue) {
    rtvalue["error"] = ErrorCodes::SUCCESS;
    std::string base_key = USER_BASE_INFO + name;
    std::string info_str = "";
    bool b_find_redis = RedisMgr::GetInstance()->get(base_key, info_str);
    if(b_find_redis) { // redis中查到用户信息
        Json::Value root;
        Json::Reader reader;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto passwd = root["passwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << passwd << " email is " << email <<" icon is " << icon << std::endl;

		rtvalue["uid"] = uid;
		rtvalue["passwd"] = passwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return;
    } else { // redis中没查到用户信息，从mysql数据库中查找
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInstance()->getUser(name);
        if(user_info) {
            // mysql中也不存在用户信息，uid无效
            rtvalue["error"] = ErrorCodes::UID_INVALID;
            return;
        }
        Json::Value redis_root;
        redis_root["uid"] = user_info->uid;
        redis_root["passwd"] = user_info->passwd;
        redis_root["name"] = user_info->name;
        redis_root["email"] = user_info->email;
        redis_root["nick"] = user_info->nick;
        redis_root["desc"] = user_info->desc;
        redis_root["sex"] = user_info->sex;
        redis_root["icon"] = user_info->icon;
    
        // 存到redis，方便下次查询
        RedisMgr::GetInstance()->set(base_key, redis_root.toStyledString());

        rtvalue["uid"] = user_info->uid;
        rtvalue["passwd"] = user_info->passwd;
        rtvalue["name"] = user_info->name;
        rtvalue["email"] = user_info->email;
        rtvalue["nick"] = user_info->nick;
        rtvalue["desc"] = user_info->desc;
        rtvalue["sex"] = user_info->sex;
        rtvalue["icon"] = user_info->icon;
    }
}

bool LogicSystem::isPureDigit(const std::string& str) {
    for(char c : str) {
        if(!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

bool LogicSystem::getBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo> &userinfo) {
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->get(base_key, info_str);
    if(b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->name = root["name"].asString();
        userinfo->email = root["email"].asString();
        userinfo->passwd = root["passwd"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
        std::cout << "user login uid is  " << userinfo->uid 
                  << " name  is " << userinfo->name 
                  << " passwd is " << userinfo->passwd 
                  << " email is " << userinfo->email << std::endl;
    } else {
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInstance()->getUser(uid);
        if(user_info == nullptr) {
            return false;
        }
        userinfo  = user_info;

        // 存到redis，redis比mysql快
        Json::Value redis_root;
        redis_root["uid"] = uid;
        redis_root["name"] = userinfo->name;
        redis_root["email"] = userinfo->email;
        redis_root["passwd"] = userinfo->passwd;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisMgr::GetInstance()->set(base_key, redis_root.toStyledString());
    }
    return true;
}

bool LogicSystem::getFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list) {
    return MysqlMgr::GetInstance()->getApplyList(to_uid, list, 0, 10);
}
