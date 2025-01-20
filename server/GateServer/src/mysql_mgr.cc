#include "mysql_mgr.h"

MysqlMgr::MysqlMgr() {

}

MysqlMgr::~MysqlMgr() {
    
}

int MysqlMgr::regUser(const std::string& name, const std::string& email, const std::string& passwd) {
    return m_dao.regUser(name, email, passwd);
}

bool MysqlMgr::checkEmailAndUser(const std::string& email, const std::string& name) {
    return m_dao.checkEmailAndUser(email, name);
}

bool MysqlMgr::checkPasswdByEmail(const std::string& email, const std::string& passwd, UserInfo& user_info) {
    return m_dao.checkPasswdByEmail(email, passwd, user_info);
}

bool MysqlMgr::checkPasswdByName(const std::string& name, const std::string& passwd, UserInfo& user_info) {
    return m_dao.checkPasswdByName(name, passwd, user_info);
}

bool MysqlMgr::updatePasswd(const std::string& name, const std::string& passwd) {
    return m_dao.updatePasswd(name, passwd);
}
