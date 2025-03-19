#ifndef __CHAT_SERVICE_IMPL_H__
#define __CHAT_SERVICE_IMPL_H__
#include <grpcpp/grpcpp.h>

#include "data.h"
#include "message.grpc.pb.h"

class ChatServiceImpl : public message::ChatService::Service {
public:
    ChatServiceImpl();

    ::grpc::Status NotifyAddFriend(::grpc::ServerContext* context, const ::message::AddFriendReq* request, ::message::AddFriendRsp* response) override;
    ::grpc::Status RplyAddFriend(::grpc::ServerContext* context, const ::message::RplyFriendReq* request, ::message::RplyFriendRsp* response) override;
    ::grpc::Status SendChatMsg(::grpc::ServerContext* context, const ::message::SendChatMsgReq* request, ::message::SendChatMsgRsp* response) override;
    ::grpc::Status NotifyAuthFriend(::grpc::ServerContext* context, const ::message::AuthFriendReq* request, ::message::AuthFriendRsp* response) override;
    ::grpc::Status NotifyTextChatMsg(::grpc::ServerContext* context, const ::message::TextChatMsgReq* request, ::message::TextChatMsgRsp* response) override;
    bool getBaseInfo(const std::string base_key, int uid, std::shared_ptr<UserInfo>& user_info);
};

#endif // __CHAT_SERVICE_IMPL_H__
