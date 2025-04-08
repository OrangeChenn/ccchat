#include "chat_service_impl.h"
#include "session.h"
#include "mysql_mgr.h"
#include "redis_mgr.h"
#include "user_mgr.h"

ChatServiceImpl::ChatServiceImpl() {

}

::grpc::Status ChatServiceImpl::NotifyAddFriend(::grpc::ServerContext* context, const ::message::AddFriendReq* request, ::message::AddFriendRsp* response) {
    int touid = request->touid();
    std::shared_ptr<Session> session = UserMgr::GetInstance()->getSession(touid);

    // 不在内存中直接返回
    if(session == nullptr) {
        return grpc::Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::SUCCESS;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["sex"] = request->sex();
    rtvalue["nick"] = request->nick();

    std::string return_str = rtvalue.toStyledString();

    session->send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
    return ::grpc::Status::OK;
}

::grpc::Status ChatServiceImpl::RplyAddFriend(::grpc::ServerContext* context, const ::message::RplyFriendReq* request, ::message::RplyFriendRsp* response) {
    return ::grpc::Status::OK;
}

::grpc::Status ChatServiceImpl::SendChatMsg(::grpc::ServerContext* context, const ::message::SendChatMsgReq* request, ::message::SendChatMsgRsp* response) {
    return ::grpc::Status::OK;
}

::grpc::Status ChatServiceImpl::NotifyAuthFriend(::grpc::ServerContext* context, const ::message::AuthFriendReq* request, ::message::AuthFriendRsp* response)  {
    int fromuid = request->fromuid();
    int touid = request->touid();
    std::shared_ptr<Session> session = UserMgr::GetInstance()->getSession(touid);

    if(session == nullptr) {
        return grpc::Status::OK;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::SUCCESS;
    rtvalue["fromuid"] = fromuid;
    rtvalue["touid"] = touid;

    std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
    std::shared_ptr<UserInfo> userinfo = std::make_shared<UserInfo>();
    bool ok = getBaseInfo(base_key, fromuid, userinfo);
    if(ok) {
        rtvalue["name"] = userinfo->name;
        rtvalue["nick"] = userinfo->nick;
        rtvalue["icon"] = userinfo->icon;
        rtvalue["sex"] = userinfo->sex;
    } else {
        response->set_error(ErrorCodes::UID_INVALID);
    }

    return ::grpc::Status::OK;
}

::grpc::Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context, const ::message::TextChatMsgReq* request, ::message::TextChatMsgRsp* response) {
    int touid = request->touid();
    std::shared_ptr<Session> session = UserMgr::GetInstance()->getSession(touid);
    if(session == nullptr) {
        return grpc::Status::OK;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::SUCCESS;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"] = touid;

    Json::Value text_array;
    for(const auto& msg : request->textmsgs()) {
        Json::Value element;
        element["msgid"] = msg.msgid();
        element["content"] = msg.msgcontent();
        text_array.append(element);
    }
    rtvalue["text_array"] = text_array;

    session->send(rtvalue.toStyledString(), ID_TEXT_CHAT_MSG_REQ);

    return ::grpc::Status::OK;
}

bool ChatServiceImpl::getBaseInfo(const std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
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
