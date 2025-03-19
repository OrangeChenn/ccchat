#include "chat_service_impl.h"

ChatServiceImpl::ChatServiceImpl() {

}

::grpc::Status ChatServiceImpl::NotifyAddFriend(::grpc::ServerContext* context, const ::message::AddFriendReq* request, ::message::AddFriendRsp* response) {
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
