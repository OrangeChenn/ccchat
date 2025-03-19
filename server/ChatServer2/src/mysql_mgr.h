#ifndef __MYSQL_MGR_H__
#define __MYSQL_MGR_H__

#include "mysql_dao.h"
#include "singleton.h"

class MysqlMgr : public Singleton<MysqlMgr> {
friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr();
    int regUser(const std::string& name, const std::string& email, const std::string& passwd);
    bool checkEmailAndUser(const std::string& email, const std::string& name);
    bool checkPasswdByEmail(const std::string& email, const std::string& passwd, UserInfo& user_info);
    bool checkPasswdByName(const std::string& name, const std::string& passwd, UserInfo& user_info);
    bool updatePasswd(const std::string& name, const std::string& passwd);
    std::shared_ptr<UserInfo> getUser(int uid);
    std::shared_ptr<UserInfo> getUser(const std::string& name);
    
private:
    MysqlMgr();
    MysqlDao m_dao;
}; 

#endif // __MYSQL_MGR_H__
