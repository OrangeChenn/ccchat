#ifndef __MYSQL_DAO_H__
#define __MYSQL_DAO_H__

#include <memory>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>

#include "data.h"

class MysqlPool {
public:
    MysqlPool(const std::string& url, const std::string& user,
            const std::string passwd, const std::string& schema, int size);
    ~MysqlPool();
    std::unique_ptr<sql::Connection> getConnection();
    void returnConnection(std::unique_ptr<sql::Connection> con);
    void close();
private:
    std::string m_url;
    std::string m_user;
    std::string m_passwd;
    std::string m_schema;
    int m_pool_size;
    std::queue<std::unique_ptr<sql::Connection>> m_connections;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::atomic<bool> m_stop;
};

class MysqlDao {
public:
    MysqlDao();
    ~MysqlDao();
    int regUser(const std::string& name, const std::string& email, const std::string& passwd);
    bool checkEmailAndUser(const std::string& email, const std::string& name);
    bool checkPasswdByEmail(const std::string& email, const std::string& passwd, UserInfo& user_info);
    bool checkPasswdByName(const std::string& name, const std::string& passwd, UserInfo& user_info);
    bool updatePasswd(const std::string& name, const std::string& passwd);
    bool addFriendApply(int uid, int touid);
    bool authFriendApply(int uid, int touid);
    bool addFriend(const int& from, const int& to, const std::string& bak_name);
    bool getApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& list, int begin, int limit);
    std::shared_ptr<UserInfo> getUser(int uid);
    std::shared_ptr<UserInfo> getUser(const std::string& name);
private:
    std::unique_ptr<MysqlPool> m_pool;
};

#endif // __MYSQL_DAO_H__
