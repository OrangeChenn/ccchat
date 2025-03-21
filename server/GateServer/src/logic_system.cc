#include "logic_system.h"
#include "http_connection.h"
#include "varify_grpc_client.h"
#include "redis_mgr.h"
#include "status_grpc_client.h"
#include "mysql_mgr.h"

LogicSystem::~LogicSystem() {}

LogicSystem::LogicSystem() {
    // GET请求测试
    regGet("/get_test", [](std::shared_ptr<HttpConnection> con) {
        beast::ostream(con->m_response.body()) << "receive get_test req" << std::endl;
        int i = 0;
        for(auto& elem : con->m_get_params) {
            i++;
            beast::ostream(con->m_response.body()) << "param" << i << " key is " << elem.first;
            beast::ostream(con->m_response.body()) << ", " << " value is" << elem.second << std::endl;
        }
    });

    // 获取验证码
    regPost("/get_varifycode", [](std::shared_ptr<HttpConnection> con) {
        auto body_str = beast::buffers_to_string(con->m_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        con->m_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if(!parse_success) {
            std::cout << "failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::JSON_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }
        std::string email = src_root["email"].asString();
        std::cout << "email is " << email << std::endl;
        message::GetVarifyRsp response = VarifyGrpcClient::GetInstance()->getVarifyCode(email);
        root["error"] = response.error();
        // root["error"] = ErrorCodes::Error_Json;
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString();
        beast::ostream(con->m_response.body()) << jsonstr;
    });

    // 新用户注册
    regPost("/user_register", [](std::shared_ptr<HttpConnection> con) {
        auto body_str = beast::buffers_to_string(con->m_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        con->m_response.set(http::field::content_type, "text/json");
        Json::Value root; // 回复client
        Json::Reader reader;
        Json::Value src_root; // 解析请求
        bool parse_success = reader.parse(body_str, src_root);
        if(!parse_success) {
            std::cout << "Failed to parse JSON data! " << std::endl;
            root["error"] = ErrorCodes::JSON_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }

        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto passwd = src_root["passwd"].asString();
        auto confirm = src_root["confirm"].asString();
        if(passwd != confirm) {
            std::cout << "password err " << std::endl;
            root["error"] = ErrorCodes::PASSWD_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }

        // 判断验证码是否合理
        std::string varify_code;
        bool b_get_varify = RedisMgr::GetInstance()->get(CODEPREFIX + src_root["email"].asString(), varify_code);
        if(!b_get_varify) {
            std::cout << "get varify code expired" << std::endl;
            root["error"] = ErrorCodes::VARIFY_EXPIRED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }

        if(varify_code != src_root["varifycode"].asString()) {
            std::cout << "varify code error" << std::endl;
            root["error"] = ErrorCodes::VARIFY_CODE_ERR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }

        // bool b_user_exist = RedisMgr::GetInstance()->existsKey(src_root["user"].asString());
        // if(b_user_exist) {
        //     std::cout << "user exist" << std::endl;
        //     root["error"] = ErrorCodes::USER_EXISTS;
        //     std::string jsonstr = root.toStyledString();
        //     beast::ostream(con->m_response.body()) << jsonstr;
        //     return;
        // }
        
        // 查找数据库判断用户是否已经存在
        int uid = MysqlMgr::GetInstance()->regUser(name, email, passwd);
        if(uid == 0 || uid == -1) {
            std::cout << "user or email exist" << std::endl;
            root["error"] = ErrorCodes::USER_EXISTS;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }

        root["error"] = 0;
        root["uid"] = uid;
        root["email"] = email;
        root["user"] = name;
        root["passwd"] = passwd;
        root["confirm"] = confirm;
        root["varifycode"] = src_root["varifycode"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(con->m_response.body()) << jsonstr;
    });

    // 重置密码
    regPost("/reset_pwd", [](std::shared_ptr<HttpConnection> con)  {
        std::string body_str = beast::buffers_to_string(con->m_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;

        // 解析json
        bool parse_success = reader.parse(body_str, src_root);
        if(!parse_success) {
            std::cout << "Failed to parse JSON" << std::endl;
            root["error"] = ErrorCodes::JSON_ERROR;
            std::string json_str = root.toStyledString();
            beast::ostream(con->m_response.body()) << json_str;
            return;
        }

        std::string name = src_root["user"].asString();
        std::string email = src_root["email"].asString();
        std::string passwd = src_root["passwd"].asString();
        std::string varifycode= src_root["varifycode"].asString();
        
        // 判断验证码是否存在或过期
        std::string varify = "";
        bool b_get_varify = RedisMgr::GetInstance()->get(CODEPREFIX + src_root["email"].asString(), varify);
        if(!b_get_varify) {
            std::cout << "varify expired" << std::endl;
            root["error"] = ErrorCodes::VARIFY_EXPIRED;
            std::string json_str = root.toStyledString();
            beast::ostream(con->m_response.body()) << json_str;
            return;
        }

        // 判断验证码是否正确
        if(varify != varifycode) {
            std::cout << "varify error" << std::endl;
            root["error"] = ErrorCodes::VARIFY_CODE_ERR;
            beast::ostream(con->m_response.body()) << root.toStyledString();
            return;
        }

        // 判断用户名和邮箱是否匹配
        bool email_valid = MysqlMgr::GetInstance()->checkEmailAndUser(email, name);
        if(!email_valid) {
            std::cout << "user name and email not match" << std::endl;
            root["error"] = ErrorCodes::EMAIL_NOT_MATCH;
            beast::ostream(con->m_response.body()) << root.toStyledString();
            return;
        }

        // 更新密码
        bool update_success = MysqlMgr::GetInstance()->updatePasswd(name, passwd);
        if(!update_success) {
            std::cout << "update passwd error" << std::endl;
            root["error"] = ErrorCodes::PASSWD_UPDATE_ERROR;
            beast::ostream(con->m_response.body()) << root.toStyledString();
            return;
        }

        // 更新成功
        std::cout << "update passwd success" << std::endl;
        root["error"] = ErrorCodes::SUCCESS;
        root["user"] = name;
        root["email"] = email;
        root["passwd"] = passwd;
        root["varifycode"] = varifycode;
        beast::ostream(con->m_response.body()) << root.toStyledString();
    });

    // 用户登录逻辑（用户名登录）
    regPost("/user_login", [](std::shared_ptr<HttpConnection> con) {
        auto body_str = beast::buffers_to_string(con->m_request.body().data());
        std::cout << "recive body: " << body_str << std::endl;
        con->m_response.set(http::field::content_type, "type/json");
        
        Json::Value root_src;
        Json::Reader reader;
        Json::Value root;
        // 解析请求JSON
        bool parse_success = reader.parse(body_str, root_src);
        if(!parse_success) {
            std::cout << "Failed to parse JSON data" << std::endl; 
            root["error"] = ErrorCodes::JSON_ERROR;
            beast::ostream(con->m_response.body()) << root.toStyledString();
            return;
        }

        // 判断用户名与密码是否正确
        std::string name = root_src["user"].asString();
        std::string passwd = root_src["passwd"].asString();
        UserInfo user_info;
        bool passwd_valid = MysqlMgr::GetInstance()->checkPasswdByName(name, passwd, user_info);
        if(!passwd_valid) {
            std::cout << "user passwd not match" << std::endl;
            root["error"] = ErrorCodes::PASSWD_INVALID;
            beast::ostream(con->m_response.body()) << root.toStyledString();
            return;
        }

        // 查询StatusServer分配到合适的ChatServer连接信息
        auto reply = StatusGrpcClient::GetInstance()->GetChatServer(user_info.uid);
        std::cout << "host: " << reply.host() << ", port: " 
            << reply.port() << ", token: " << reply.token() << std::endl;
        if(reply.error()) {
            std::cout << "grpc get chat server failed, error is: " << reply.error() << std::endl;
            root["error"] = ErrorCodes::RPC_FAILED;
            beast::ostream(con->m_response.body()) << root.toStyledString();
            return;
        }
        std::cout << "succeed to load userinfo uid is: " << user_info.uid << std::endl;
        root["error"] = ErrorCodes::SUCCESS;
        root["user"] = name;
        root["uid"] = user_info.uid;
        root["token"] = reply.token();
        root["port"] = reply.port();
        root["host"] = reply.host();
        beast::ostream(con->m_response.body()) << root.toStyledString();
    });

    // 用户登录逻辑（邮箱登录）
    regPost("/email_login", [](std::shared_ptr<HttpConnection> con) {
        auto body_str = beast::buffers_to_string(con->m_request.body().data());
        std::cout << "recive body: " << body_str << std::endl;
        con->m_response.set(http::field::content_type, "type/json");

        // 解析JSON数据
        Json::Value root_src;
        Json::Value root;
        Json::Reader reader;
        bool parse_success = reader.parse(body_str, root_src);
        if(!parse_success) {
            std::cout << "Failed to parse JSON body data" << std::endl;
            root["error"] = ErrorCodes::JSON_ERROR;
            beast::ostream(con->m_response.body()) << root.toStyledString();
        }

        // 判断邮箱与密码是否正确
        std::string email = root_src["emial"].asString();
        std::string passwd = root_src["passwd"].asString();
        UserInfo user_info;
        bool passwd_valid = MysqlMgr::GetInstance()->checkPasswdByEmail(email, passwd, user_info);
        if(!passwd_valid) {
            std::cout << "email passwd not match" << std::endl;
            root["error"] = ErrorCodes::PASSWD_INVALID;
            beast::ostream(con->m_response.body()) << root.toStyledString();
        }

        // 分配ChatServer
        message::GetChatServerRsp reply = StatusGrpcClient::GetInstance()->GetChatServer(user_info.uid);
        if(reply.error()) {
            std::cout << "grpc get chat server failed, error is: " << reply.error() << std::endl;
            root["error"] = ErrorCodes::RPC_FAILED;
            beast::ostream(con->m_response.body()) << root.toStyledString();
        }

        std::cout << "succeed to load userinfo uid is: " << user_info.uid << std::endl;
        root["error"] = ErrorCodes::SUCCESS;
        root["emial"] = email;
        root["uid"] = user_info.uid;
        root["token"] = reply.token();
        root["host"] = reply.host();
        beast::ostream(con->m_response.body()) << root.toStyledString();
    });
}

bool LogicSystem::handlerGet(const std::string& path, std::shared_ptr<HttpConnection> con) {
    if(m_get_handlers.find(path) == m_get_handlers.end()) {
        return false;
    }
    m_get_handlers[path](con);
    return true;
}

bool LogicSystem::handlerPost(const std::string& path, std::shared_ptr<HttpConnection> con) {
    if(m_post_handlers.find(path) == m_post_handlers.end()) {
        return false;
    }
    m_post_handlers[path](con);
    return true;
}

// 注册get处理函数
void LogicSystem::regGet(const std::string& url, HttpHandler handler) {
    m_get_handlers.insert(std::make_pair(url, handler));
}

// 注册post处理函数
void LogicSystem::regPost(const std::string& url, HttpHandler handler) {
    m_post_handlers.insert(std::make_pair(url, handler));
}
