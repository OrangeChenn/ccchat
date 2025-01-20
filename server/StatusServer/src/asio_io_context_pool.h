#ifndef __ASIO_IO_SERVER_POOL_H__
#define __ASIO_IO_SERVER_POOL_H__
#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <thread>
#include "singleton.h"

class AsioIOContextPool : public Singleton<AsioIOContextPool> {
friend class Singleton<AsioIOContextPool>;
public:
    using IOContext = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOContextPool();
    AsioIOContextPool(const AsioIOContextPool&) = delete;
    AsioIOContextPool& operator=(const AsioIOContextPool&) = delete;

    boost::asio::io_context& getIOContext();
    void stop();
private:
    AsioIOContextPool(std::size_t size = 2);
    std::vector<IOContext> m_io_contexts;
    std::vector<WorkPtr> m_works;
    std::vector<std::thread> m_threads;
    std::size_t m_next_io_context;
};

#endif // __ASIO_IO_SERVER_POOL_H__
