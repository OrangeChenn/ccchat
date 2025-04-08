#include "mysql_dao.h"
#include <iostream>
#include "const.h"
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
        std::unique_ptr<sql::PreparedStatement> pstme(con->prepareStatement("CALL reg_user(?,?,?,@result)"));
        pstme->setString(1, name);
        pstme->setString(2, email);
        pstme->setString(3, passwd);

        pstme->execute();
        std::unique_ptr<sql::Statement> pstme_result(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(pstme_result->executeQuery("SELECT @result AS result"));
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

bool MysqlDao::checkEmailAndUser(const std::string& email, const std::string& name) {
    std::unique_ptr<sql::Connection> con = m_pool->getConnection();
    try {
        if(nullptr == con) {
            m_pool->returnConnection(std::move(con));
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> pstmd(con->prepareStatement("SELECT email FROM user WHERE name=?"));
        pstmd->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmd->executeQuery());
        while(res->next()) {
            std::string test_email = res->getString("email");
            std::cout << "Check email: " << test_email << std::endl;
            if(email != test_email)  {
                m_pool->returnConnection(std::move(con));
                return false;
            }
        }
        m_pool->returnConnection(std::move(con));
        return true;
    } catch(const sql::SQLException& e) {
        m_pool->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::checkPasswdByEmail(const std::string& email, const std::string& passwd, UserInfo& user_info) {
    std::unique_ptr<sql::Connection> con = m_pool->getConnection();
    if(nullptr == con) {
        return false;
    }
    
    Defer defer([&, this]() {
        m_pool->returnConnection(std::move(con));
    });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE email = ?"));
        pstmt->setString(1, email);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";
        while(res->next()) {
            origin_pwd = res->getString("passwd");
            std::cout << "passwd: "  << origin_pwd << std::endl;
            break;
        }
        if(passwd != origin_pwd) {
            return false;
        }
        user_info.email = email;
        user_info.name = res->getString("name");
        user_info.uid = res->getInt("uid");
        user_info.passwd = origin_pwd;
        return true;
    } catch(const sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::checkPasswdByName(const std::string& name, const std::string& passwd, UserInfo& user_info) {
    std::unique_ptr<sql::Connection> con = m_pool->getConnection();
    if(nullptr == con) {
        return false;
    }

    Defer defer([&, this]() {
        m_pool->returnConnection(std::move(con));
    });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";
        while(res->next()) {
            origin_pwd = res->getString("passwd");
            std::cout << "passwd: " << origin_pwd << std::endl;
            break;
        }
        if(passwd != origin_pwd) {
            return false;
        }
        
        user_info.email = res->getString("email");
        user_info.name = name;
        user_info.passwd = origin_pwd;
        user_info.uid = res->getInt("uid");
        return true;
    } catch(const sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::updatePasswd(const std::string& name, const std::string& passwd) {
    std::unique_ptr<sql::Connection> con = m_pool->getConnection();
    try {
        if(nullptr == con) {
            m_pool->returnConnection(std::move(con));
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> pstmd(con->prepareStatement("UPDATE user SET passwd = ? WHERE name = ?"));
        pstmd->setString(1, passwd);
        pstmd->setString(2, name);
        int row = pstmd->executeUpdate();
        std::cout << "update rows: " << row << std::endl;
        m_pool->returnConnection(std::move(con));
        return true;
    } catch(const sql::SQLException& e) {
        m_pool->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::addFriendApply(int uid, int touid) {
    std::unique_ptr<sql::Connection> con = m_pool->getConnection();
    if(con == nullptr) {
        return false;
    }
    
    Defer defer([this, &con]() {
        this->m_pool->returnConnection(std::move(con));
    });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmd(con->prepareStatement(
            "INSERT INTO friend_apply (from_uid, to_uid) values(?,?)"
            "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"
        ));
        pstmd->setInt(1, uid);
        pstmd->setInt(2, touid);

        int row_affected = pstmd->executeUpdate();
        if(row_affected < 0) {
            return false;
        }
        return true;
    } catch(const sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return true;
}

bool MysqlDao::authFriendApply(int uid, int touid) {
    auto con = m_pool->getConnection();
    if(con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        this->m_pool->returnConnection(std::move(con));
    });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("UPDATE friend_apply SET status = 1 "
                                            "WHERE from_id = ? AND to_uid = ?"));
        pstmt->setInt(1, uid);
        pstmt->setInt(2, touid);

        // std::unique_ptr<sql::ResultSet> res(pstmt->executeUpdate());
        int row = pstmt->executeUpdate();
        if(row < 0) {
            return false;
        }

        return true;
    } catch(const sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::addFriend(const int& from, const int& to, const std::string& bak_name) {
    auto con = m_pool->getConnection();
    if(con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        this->m_pool->returnConnection(std::move(con));
    });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("INSERT IGNAORE INTO friend(self_id, friend_id, bak) "
                                                                            "VALUES (?, ?, ?) "));

        pstmt->setInt(1, from);
        pstmt->setInt(2, to);
        pstmt->setString(3, bak_name);
        int row = pstmt->executeUpdate();
        if(row < 0) {
            return false;
        }

        return true;
    } catch(const sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDao::getApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& list, int begin, int limit) {
    auto con = m_pool->getConnection();
    if(con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        this->m_pool->returnConnection(std::move(con));
    });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("select apply.from_uid, apply.status, user.name, "
                        "user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
                                                    "and apply.id > ? order by apply.id ASC LIMIT ? "));

        pstmt->setInt(1, touid);
        pstmt->setInt(2, begin);
        pstmt->setInt(3, limit);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while(res->next()) {
            auto name = res->getString("name");
            auto uid = res->getInt("from_uid");
            auto status = res->getInt("status");
            auto nick = res->getString("nick");
            auto sex = res->getInt("sex");
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
            list.push_back(apply_ptr);
        }
        return true;
    } catch(const sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

std::shared_ptr<UserInfo> MysqlDao::getUser(int uid) {
    std::unique_ptr<sql::Connection> con = m_pool->getConnection();
    if(nullptr == con) {
        return nullptr;
    }

    Defer defer([this, &con]() {
        this->m_pool->returnConnection(std::move(con));
    });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmd(con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmd->setInt(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmd->executeQuery());
        std::shared_ptr<UserInfo> user_info(nullptr);
        while(res->next()) {
            user_info.reset(new UserInfo);
            user_info->email = res->getString("email");
            user_info->name = res->getString("name");
            user_info->uid = res->getInt("uid");
            user_info->passwd = res->getString("passwd");
            user_info->nick = res->getString("nick");
            user_info->desc = res->getString("desc");
            user_info->sex = res->getInt("sex");
            break;
        }
        return user_info;
    } catch(const sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDao::getUser(const std::string& name) {
    std::unique_ptr<sql::Connection> con = m_pool->getConnection();
    if(nullptr == con) {
        return nullptr;
    }
    Defer defer([this, &con]() {
        this->m_pool->returnConnection(std::move(con));
    });

    try{
        std::unique_ptr<sql::PreparedStatement> pstmd(con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmd->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmd->executeQuery());
        std::shared_ptr<UserInfo> user_info(nullptr);
        while(res->next()) {
            user_info.reset(new UserInfo);
            user_info->email = res->getString("email");
            user_info->name = res->getString("name");
            user_info->uid = res->getInt("uid");
            user_info->passwd = res->getString("passwd");
            user_info->nick = res->getString("nick");
            user_info->desc = res->getString("desc");
            user_info->sex = res->getInt("sex");
            break;
        }
        return user_info;
    } catch(const sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}
