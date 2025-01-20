#include "asio_io_context_pool.h"
#include <iostream>

AsioIOContextPool::AsioIOContextPool(std::size_t size /* = 2 */) 
        : m_io_contexts(size),
          m_works(size),
          m_next_io_context(0){
    for(std::size_t i = 0; i < size; ++i) {
        m_works[i] = std::unique_ptr<Work>(new Work(m_io_contexts[i]));
    }

    for(std::size_t i = 0; i < size; ++i) {
        m_threads.emplace_back([this, i]() {
            this->m_io_contexts[i].run();
        });
    }
}

AsioIOContextPool::~AsioIOContextPool() {
    stop();
    std::cout << "AsioIOContextPool destruct" << std::endl;
}

boost::asio::io_context& AsioIOContextPool::getIOContext() {
    auto& context = m_io_contexts[m_next_io_context++];
    if(m_next_io_context == m_io_contexts.size()) {
        m_next_io_context = 0;
    }
    return context;
}

void AsioIOContextPool::stop() {
    for(auto& work : m_works) {
        work->get_io_context().stop();
        work.reset();
    }

    for(auto& thread : m_threads) {
        thread.join();
    }
}
