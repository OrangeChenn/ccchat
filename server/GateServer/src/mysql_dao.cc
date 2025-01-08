#include "mysql_dao.h"
#include <iostream>
#include "config_mgr.h"

MysqlPool::MysqlPool(const std::string& url, const std::string& user,
        const std::string passwd, const std::string& schema, int size)
    : m_url(url),
      m_user(user),
      m_passwd(passwd),
      m_schema(schema),
      m_pool_size(size),
      m_stop(false) {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
        std::unique_ptr<sql::Connection> con(driver->connect(m_url, m_user, m_passwd));
        con->setSchema(m_schema);
        m_connections.push(std::move(con));
    } catch(const sql::SQLException& e) {
        std::cout << "mysql pool init failed" << std::endl;
    }
}

MysqlPool::~MysqlPool() {
    std::lock_guard lock(m_mutex);
    while(!m_connections.empty()) {
        m_connections.pop();
    }
}

std::unique_ptr<sql::Connection>  MysqlPool::getConnection() {
    std::unique_lock lock(m_mutex);
    m_cond.wait(lock, [this]() {
        if(this->m_stop) {
            return true;
        }
        return !this->m_connections.empty();
    });
    if(m_stop) {
        return nullptr;
    }
    std::unique_ptr<sql::Connection> con(std::move(m_connections.front()));
    m_connections.pop();
    return con;
}

void  MysqlPool::returnConnection(std::unique_ptr<sql::Connection> con) {
    std::lock_guard lock(m_mutex);
    m_connections.push(std::move(con));
    m_cond.notify_one();
}

void  MysqlPool::close() {
    m_stop = true;
    m_cond.notify_all();
}

MysqlDao::MysqlDao() {
    auto& config = ConfigMgr::Inst();
    const std::string& host = config["MySql"]["Host"];
    const std::string& port = config["MySql"]["Port"];
    const std::string& user = config["MySql"]["User"];
    const std::string& passwd = config["MySql"]["Passwd"];
    const std::string& schema = config["MySql"]["Schema"];
    m_pool.reset(new MysqlPool(host + ":" + port, user, passwd, schema, 5));
}

MysqlDao::~MysqlDao() {
    m_pool->close();
}

int MysqlDao::regUser(const std::string& name, const std::string& email, const std::string& passwd) {
    auto con = m_pool->getConnection();
    try {
        if(nullptr == con) {
            m_pool->returnConnection(std::move(con));
            return 0;
        }
        std::unique_ptr<sql::PreparedStatement> stme(con->prepareStatement("CALL reg_user(?,?,?,@result)"));
        stme->setString(1, name);
        stme->setString(2, email);
        stme->setString(3, passwd);

        stme->execute();
        std::unique_ptr<sql::Statement> stme_result(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stme_result->executeQuery("SELECT @result AS result"));
        if(res->next()) {
            int result = res->getInt("result");
            std::cout << "Result: " << result << std::endl;
            m_pool->returnConnection(std::move(con));
            return result;
        }
        m_pool->returnConnection(std::move(con));
        return -1;
    } catch(const sql::SQLException& e) {
        m_pool->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what()
                  << " (MySql error code: " << e.getErrorCode()
                  << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}
