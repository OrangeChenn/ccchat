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

bool MysqlMgr::addFriendApply(int uid, int touid) {
    return m_dao.addFriendApply(uid, touid);
}

bool MysqlMgr::authFriendApply(int uid, int touid) {
    return m_dao.authFriendApply(uid, touid);
}

bool MysqlMgr::addFriend(const int& from, const int& to, const std::string& bak_name) {
    return m_dao.addFriend(from, to, bak_name);
}

bool MysqlMgr::getApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& list, int begin, int limit) {
    return m_dao.getApplyList(touid, list, begin, limit);
}

std::shared_ptr<UserInfo> MysqlMgr::getUser(int uid) {
    return m_dao.getUser(uid);
}

std::shared_ptr<UserInfo> MysqlMgr::getUser(const std::string& name) {
    return m_dao.getUser(name);
}
