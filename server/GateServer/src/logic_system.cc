#include "logic_system.h"
#include "http_connection.h"

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

    regPost("/get_varifycode", [](std::shared_ptr<HttpConnection> con) {
        auto body_str = beast::buffers_to_string(con->m_response.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        con->m_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value str_root;
        bool parse_success = reader.parse(body_str, str_root);
        if(!parse_success) {
            std::cout << "failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(con->m_response.body()) << jsonstr;
            return;
        }
        std::string email = str_root["email"].asString();
        std::cout << "email is " << email << std::endl;
        root["error"] = 0;
        root["email"] = str_root["email"];
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
}

// 注册get处理函数
void LogicSystem::regGet(const std::string& url, HttpHandler handler) {
    m_get_handlers.insert(std::make_pair(url, handler));
}

// 注册post处理函数
void LogicSystem::regPost(const std::string& url, HttpHandler handler) {
    m_post_handlers.insert(std::make_pair(url, handler));
}
