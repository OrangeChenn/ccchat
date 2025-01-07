#include "redis_mgr.h"

#include <iostream>
#include <string.h>

#include "config_mgr.h"

RedisConPool::RedisConPool(size_t pool_size, const char* host, int port, const char* passwd) 
    : m_stop(false),
      m_pool_size(pool_size),
      m_host(host),
      m_port(port) {
    for(size_t i = 0; i < pool_size; ++i) {
        redisContext* context = redisConnect(host, port);
        if(nullptr == context || context->err != 0) {
            if(nullptr == context) {
                redisFree(context);
            }
            continue;
        }
        redisReply* reply = (redisReply*)redisCommand(context, "AUTH %s", passwd);
        if(reply->type == REDIS_REPLY_ERROR) {
            std::cout << "认证失败" << std::endl;
            freeReplyObject(reply);
            continue;
        }
        std::cout << "认证成功" << std::endl;
        freeReplyObject(reply);
        m_connections.push(context);
    }
}

RedisConPool::~RedisConPool() {
    std::lock_guard lock(m_mutex);
    while(!m_connections.empty()) {
        auto context = m_connections.front();
        m_connections.pop();
        redisFree(context);
    }
}

redisContext* RedisConPool::getConnection() {
    std::unique_lock lock(m_mutex);
    m_cond.wait(lock, [this]() {
        if(m_stop) {
            return true;
        }
        return !this->m_connections.empty();
    });
    if(m_stop) {
        return nullptr;
    }
    auto context = m_connections.front();
    m_connections.pop();
    return context;
}

void RedisConPool::returnConnection(redisContext* context) {
    std::lock_guard lock(m_mutex);
    if(m_stop) {
        return;
    }
    m_connections.push(context);
    m_cond.notify_one();
}

void RedisConPool::close() {
    m_stop = true;
    m_cond.notify_all();
}


RedisMgr::RedisMgr() {
    ConfigMgr& cfg_mgr = ConfigMgr::Inst();
    std::string host = cfg_mgr["Redis"]["Host"];
    std::string port = cfg_mgr["Redis"]["Port"];
    std::string passwd = cfg_mgr["Redis"]["Passwd"];
    m_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), passwd.c_str()));
}

RedisMgr::~RedisMgr() {
    // close();
    m_pool->close();
}

bool RedisMgr::get(const std::string& key, std::string &value) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "GET %s", key.c_str());
    if(nullptr == reply) {
        std::cout << "Failed to execute command [ GET " << key << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    if(reply->type != REDIS_REPLY_STRING) {
        std::cout << "Failed to execute command [ GET " << key << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    std::cout << "Succeed to execute command [ GET " << key << " ]" << std::endl;
    m_pool->returnConnection(context);
    return true;
}

bool RedisMgr::set(const std::string& key, const std::string& value) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "SET %s %s", key.c_str(), value.c_str());
    if(nullptr == reply) {
        std::cout << "Failed to execute command [ SET " << key << " " << value << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    if(!(reply->type == REDIS_REPLY_STATUS 
            && (strcmp(reply->str, "OK") || strcmp(reply->str, "ok")))) {
        std::cout << "Failed to execute command [ SET " << key << " " << value << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Succeed to execute command [ SET " << key << " " << value << " ]" << std::endl;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

bool RedisMgr::auth(const std::string& password) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "AUTH %s", password.c_str());
    if(reply->type == REDIS_REPLY_ERROR) {
        std::cout << "认证失败" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "认证成功" << std::endl;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

bool RedisMgr::lpush(const std::string& key, const std::string& value) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "LPUSH %s %s", key.c_str(), value.c_str());
    if(nullptr == reply) {
        std::cout << "Failed to execute command [ LPUSH " << key << " " << value << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    if(reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
        std::cout << "Failed to execute command [ LPUSH " << key << " " << value << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Succeed to execute command [ LPUSH " << key << " " << value << " ]" << std::endl;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

bool RedisMgr::lpop(const std::string& key, std::string& value) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "LPOP %s", key.c_str());
    if(nullptr == reply || reply->type == REDIS_REPLY_NIL) {
        std::cout << "Failed to execute command [ LPOP " << key << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Succeed to execute command [ LPOP " << key << " ]" << std::endl;
    value = reply->str;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

bool RedisMgr::rpush(const std::string& key, const std::string& value) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "RPUSH %s %s", key.c_str(), value.c_str());
    if(nullptr == reply) {
        std::cout << "Failed to execute command [ RPUSH " << key << " " << value << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    if(reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
        std::cout << "Failed to execute command [ RPUSH " << key << " " << value << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Succeed to execute command [ RPUSH " << key << " " << value << " ]" << std::endl;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

bool RedisMgr::rpop(const std::string& key, std::string& value) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "RPOP %s", key.c_str());
    if(nullptr == reply || reply->type == REDIS_REPLY_NIL) {
        std::cout << "Failed to execute command [ RPOP " << key << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Succeed to execute command [ RPOP " << key << " ]" << std::endl;
    value = reply->str;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

bool RedisMgr::hset(const std::string& key, const std::string& hkey, const std::string& value) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
    if(nullptr == reply || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Failed to execute command [ HSET " << key << " " 
                << hkey << " " << value << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Succeed to execute command [ HSET " << key << " " 
            << hkey << " " << value << " ]" << std::endl;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

bool RedisMgr::hset(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen) {
    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = hvalue;
    argvlen[3] = hvaluelen;

    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommandArgv(context, 4, argv, argvlen);
    if(nullptr == reply || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Failed to execute command [ HSET " << key << " " 
                << hkey << " " << hvalue << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Succeed to execute command [ HSET " << key << " " 
            << hkey << " " << hvalue << " ]" << std::endl;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

std::string RedisMgr::hget(const std::string& key, const std::string& hkey) {
    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.length();

    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return "";
    }
    auto reply = (redisReply*)redisCommandArgv(context, 3, argv, argvlen);
    if(nullptr == reply || reply->type == REDIS_REPLY_NIL) { // (nil)
        std::cout << "Failed to execute command [ HSET " 
                << key << " " << hkey << " ]" << std::endl;
        freeReplyObject(reply);
        return "";
    }
    std::cout << "Succeed to execute command [ HSET " 
            << key << " " << hkey << " ]" << std::endl;
    std::string value = reply->str;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return value;
}

bool RedisMgr::del(const std::string& key) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "DEL %s", key.c_str());
    if(nullptr == reply || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Failed to execute command [ DEL " << key << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout << "Succeed to execute command [ DEL " << key << " ]" << std::endl;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}


bool RedisMgr::existsKey(const std::string& key) {
    redisContext* context = m_pool->getConnection();
    if(nullptr == context) {
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "EXISTS %s", key.c_str());
    if(nullptr == reply || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
        std::cout <<  "Failed to execute command [ EXISTS " << key << " ]" << std::endl;
        freeReplyObject(reply);
        return false;
    }
    std::cout <<  "Succeed to execute command [ EXISTS " << key << " ]" << std::endl;
    freeReplyObject(reply);
    m_pool->returnConnection(context);
    return true;
}

void RedisMgr::close() {
    // redisFree(context);
}
