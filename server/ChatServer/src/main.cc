#include <csignal>

#include <iostream>
#include <boost/asio.hpp>

#include "asio_io_context_pool.h"
#include "chatserver.h"
#include "config_mgr.h"

int main(int argc, char** argv) {
    try {
        auto& config = ConfigMgr::Inst();
        auto pool = AsioIOContextPool::GetInstance();
        boost::asio::io_context ioc;
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, pool](auto, auto) {
            ioc.stop();
            pool->stop();
        });
        std::string port_str = config["SelfServer"]["Port"];
        ChatServer server(ioc, atoi(port_str.c_str()));
        ioc.run();
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return 0;
}