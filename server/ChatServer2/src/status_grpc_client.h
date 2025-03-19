#ifndef __STATUS_GRPC_CLIENT_H__
#define __STATUS_GRPC_CLIENT_H__
#include <grpcpp/grpcpp.h>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "config_mgr.h"
#include "message.grpc.pb.h"
#include "singleton.h"

class StatusConPool {
public:
    StatusConPool(size_t pool_size, const std::string& host, const std::string& port);
    ~StatusConPool();
    std::unique_ptr<message::StatusService::Stub> getConnection();
    void returnConnection(std::unique_ptr<message::StatusService::Stub> stub);
    void close();

private:
    std::atomic<bool> m_stop;
    size_t m_pool_size;
    std::string m_host;
    std::string m_port;
    std::queue<std::unique_ptr<message::StatusService::Stub>> m_connections;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

class StatusGrpcClient : public Singleton<StatusGrpcClient> {
friend class Singleton<StatusGrpcClient>;
public:
    ~StatusGrpcClient();
    message::LoginRsp Login(int uid, const std::string& token);
private:
    StatusGrpcClient();
    std::unique_ptr<StatusConPool> m_pool;
};

#endif // __STATUS_GRPC_CLIENT_H__
