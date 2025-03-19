#include <csignal>

#include <iostream>
#include <boost/asio.hpp>

#include "asio_io_context_pool.h"
#include "chatserver.h"
#include "chat_service_impl.h"
#include "config_mgr.h"
#include "redis_mgr.h"

bool b_stop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main(int argc, char** argv) {
    auto& cfg = ConfigMgr::Inst();
    std::string server_name = cfg["SelfServer"]["Name"];
    try {
        auto pool = AsioIOContextPool::GetInstance();

        // 登录数设置为0
        RedisMgr::GetInstance()->hset(LOGIN_COUNT, server_name, "0");

        // grpc监听服务
        std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
        ChatServiceImpl service;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        // 构建并启动
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        // 单独启动一个线程处理 gRPC 服务
        std::thread grpc_server_thread([&server]() {
            server->Wait(); // 阻塞等待服务器关闭
        });

        // tcp监听服务
        boost::asio::io_context ioc;
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, pool](auto, auto) {
            ioc.stop();
            pool->stop();
        });
        std::string port_str = cfg["SelfServer"]["Port"];
        ChatServer cserver(ioc, atoi(port_str.c_str()));
        ioc.run();

        RedisMgr::GetInstance()->hdel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->close();
        grpc_server_thread.join();
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return 0;
}