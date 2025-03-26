#include "chat_service_impl.h"
#include "session.h"
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
    return ::grpc::Status::OK;
}

::grpc::Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context, const ::message::TextChatMsgReq* request, ::message::TextChatMsgRsp* response) {
    return ::grpc::Status::OK;
}

bool ChatServiceImpl::getBaseInfo(const std::string base_key, int uid, std::shared_ptr<UserInfo>& user_info) {
    return true;
}
