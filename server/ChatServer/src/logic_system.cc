#include "logic_system.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

LogicSystem::LogicSystem() : m_stop(false) {
    registerCallBacks();
    m_work_thread = std::thread(&LogicSystem::dealMsg, this);
}

LogicSystem::~LogicSystem() {
}

void LogicSystem::postMsgToQue(std::shared_ptr<Session> session, std::shared_ptr<RecvNode> recv_node) {

}

void LogicSystem::logicHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data) {
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    std::cout << "user login uid is: " << root["uid"].asInt()
              << ", token is: " << root["token"].asString() << std::endl;
    session->send(root.toStyledString(), msg_id);
}

void LogicSystem::registerCallBacks() {
    m_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::logicHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::dealMsg() {

}
