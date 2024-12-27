#include "const.h"

#include "gate_server.h"

int main(int argc, char** argv)
{

    try {
        unsigned short port = 8888;
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
