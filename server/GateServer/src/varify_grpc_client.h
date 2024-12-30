#ifndef __VARIFY_GRPC_CLENT_H_
#define __VARIFY_GRPC_CLENT_H_

#include <grpcpp/grpcpp.h>

#include "const.h"
#include "message.grpc.pb.h"
#include "singleton.h"

// using grpc::Channel;
// using grpc::Status;
// using grpc::ClientContext;

// using message::GetVarifyReq;
// using message::GetVarifyRsp;
// using message::VarifyService;

class VarifyGrpcClient : public Singleton<VarifyGrpcClient> {
friend class Singleton<VarifyGrpcClient>;
public:
    message::GetVarifyRsp getVarifyCode(const std::string& email) {
        grpc::ClientContext context;
        message::GetVarifyReq request;
        message::GetVarifyRsp response;
        request.set_email(email);
        grpc::Status status = m_stub->GetVarifyCode(&context, request, &response);
        if(status.ok()) {
            return response;
        }
        response.set_error(ErrorCodes::RPCFailed);
        return response;
    }
private:
    VarifyGrpcClient() {
        std::shared_ptr<grpc::Channel> channel = 
                grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
        m_stub = message::VarifyService::NewStub(channel);
    }
    std::unique_ptr<message::VarifyService::Stub> m_stub;
};

#endif // __VARIFY_GRPC_CLENT_H_
