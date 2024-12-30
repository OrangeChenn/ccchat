#include "const.h"
#include "config_mgr.h"
#include "gate_server.h"

int main(int argc, char** argv)
{
    ConfigMgr config_mgr;
    unsigned short gate_port = atoi(config_mgr["GateServer"]["Port"].c_str());
    std::cout << "gate_prot = " << gate_port << std::endl; 
    try {
        unsigned short port = 8899;
        net::io_context ioc;
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int signal_number) {
            if(ec) {
                return;
            }
            ioc.stop();
        });
        std::make_shared<GateServer>(ioc, port)->start();
        ioc.run();
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
