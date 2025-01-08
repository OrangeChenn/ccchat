#include "logic_system.h"
#include "http_connection.h"
#include "varify_grpc_client.h"
#include "redis_mgr.h"
#include "mysql_mgr.h"

LogicSystem::~LogicSystem() {}

LogicSystem::LogicSystem() {
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
            root["error"] = ErrorCodes::ErrorJson;
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
        Json::Value root; // 回复client
        Json::Reader reader;
        Json::Value src_root; // 解析请求
        bool parse_success = reader.parse(body_str, src_root);
        if(!parse_success) {
            std::cout << "Failed to parse JSON data! " << std::endl;
            root["error"] = ErrorCodes::ErrorJson;
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
            root["error"] = ErrorCodes::PasswdErr;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }

        // 判断验证码是否合理
        std::string varify_code;
        bool b_get_varify = RedisMgr::GetInstance()->get("code_" + src_root["email"].asString(), varify_code);
        if(!b_get_varify) {
            std::cout << "get varify code expired" << std::endl;
            root["error"] = ErrorCodes::VarifyExpired;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }

        if(varify_code != src_root["varifycode"].asString()) {
            std::cout << "varify code error" << std::endl;
            root["error"] = ErrorCodes::VarifyCodeErr;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }

        // bool b_user_exist = RedisMgr::GetInstance()->existsKey(src_root["user"].asString());
        // if(b_user_exist) {
        //     std::cout << "user exist" << std::endl;
        //     root["error"] = ErrorCodes::UserExists;
        //     std::string jsonstr = root.toStyledString();
        //     beast::ostream(con->m_response.body()) << jsonstr;
        //     return;
        // }
        
        // 查找数据库判断用户是否已经存在
        int uid = MysqlMgr::GetInstance()->regUser(name, email, passwd);
        if(uid == 0 || uid == -1) {
            std::cout << "user or email exist" << std::endl;
            root["error"] = ErrorCodes::UserExists;
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
