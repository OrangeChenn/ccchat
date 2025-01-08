#include "mysql_mgr.h"

MysqlMgr::MysqlMgr() {

}

MysqlMgr::~MysqlMgr() {
    
}

int MysqlMgr::regUser(const std::string& name, const std::string& email, const std::string& passwd) {
    return m_dao.regUser(name, email, passwd);
}