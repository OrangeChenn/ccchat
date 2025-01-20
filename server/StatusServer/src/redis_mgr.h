#ifndef __REDIS_MGR_H__
#define __REDIS_MGR_H__

#include <hiredis/hiredis.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include "singleton.h"

class RedisConPool {
public:
    RedisConPool(size_t pool_size, const char* host, int port, const char* passwd);
    ~RedisConPool();
    redisContext* getConnection();
    void returnConnection(redisContext* context);
    void close();
private:
    std::atomic<bool> m_stop;
    size_t m_pool_size;
    const char* m_host;
    int m_port;
    std::queue<redisContext*> m_connections;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

class RedisMgr : public Singleton<RedisMgr>, std::enable_shared_from_this<RedisMgr> {
friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();

    bool get(const std::string& key, std::string &value);
    bool set(const std::string& key, const std::string& value);
    bool auth(const std::string& password);
    bool lpush(const std::string& key, const std::string& value);
    bool lpop(const std::string& key, std::string& value);
    bool rpush(const std::string& key, const std::string& value);
    bool rpop(const std::string& key, std::string& value);
    bool hset(const std::string& key, const std::string& hkey, const std::string& value);
    bool hset(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string hget(const std::string& key, const std::string& hkey);
    bool del(const std::string& key);
    bool existsKey(const std::string& key);
    void close();
private:
    RedisMgr();
    // redisContext* m_context = nullptr;
    std::unique_ptr<RedisConPool> m_pool;
    // redisReply* reply = nullptr;
};

#endif // __REDIS_MGR_H__
