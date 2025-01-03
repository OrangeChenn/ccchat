#ifndef __VARIFY_GRPC_CLENT_H_
#define __VARIFY_GRPC_CLENT_H_

#include <grpcpp/grpcpp.h>
#include <queue>

#include "const.h"
#include "message.grpc.pb.h"
#include "singleton.h"

// using grpc::Channel;
// using grpc::Status;
// using grpc::ClientContext;

// using message::GetVarifyReq;
// using message::GetVarifyRsp;
// using message::VarifyService;

class RPCPool {
public:
    RPCPool(size_t pool_size, const std::string& host, const std::string& port);
    ~RPCPool();
    std::unique_ptr<message::VarifyService::Stub> getConnection();
    void returnConnection(std::unique_ptr<message::VarifyService::Stub> stub);
    void close();

private:
    std::atomic<bool> m_stop;
    size_t m_pool_size;
    std::string m_host;
    std::string m_port;
    std::queue<std::unique_ptr<message::VarifyService::Stub>> m_connections;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

class VarifyGrpcClient : public Singleton<VarifyGrpcClient> {
friend class Singleton<VarifyGrpcClient>;
public:
    ~VarifyGrpcClient() {}
    VarifyGrpcClient(const VarifyGrpcClient&) = delete;
    VarifyGrpcClient& operator=(const VarifyGrpcClient&) = delete;
    message::GetVarifyRsp getVarifyCode(const std::string& email);
private:
    VarifyGrpcClient();
    std::unique_ptr<RPCPool> m_pool;
};

#endif // __VARIFY_GRPC_CLENT_H_
