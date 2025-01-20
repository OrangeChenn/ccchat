#include <iostream>
#include <boost/asio.hpp>
#include <memory>
#include <thread>

#include "const.h"
#include "config_mgr.h"
#include "status_service_impl.h"

void runSrver() {
    ConfigMgr& cfg_mgr = ConfigMgr::Inst();
    std::string server_address(cfg_mgr["StatusServer"]["Host"] + ":" + cfg_mgr["StatusServer"]["Port"]);
    StatusServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 创建boost.asio的io_context
    boost::asio::io_context io_context;
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    
    // 设置异步等待SIGINT信号
    signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
        if(!error) {
            std::cout << "Shutting down server..." << std::endl;
            server->Shutdown();
        }
    });

    // 在单独线程中运行io_context
    std::thread([&io_context]() {
        io_context.run();
    }).detach();

    server->Wait();
    io_context.stop();
}

int main(int argc, char** argv) {
    try {
        runSrver();
    } catch(const std::exception& e) {
        std::cerr << " Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}