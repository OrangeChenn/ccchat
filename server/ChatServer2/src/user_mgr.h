#ifndef __USER_MGR_H__
#define __USER_MGR_H__
#include "session.h"
#include "singleton.h"

class UserMgr : public Singleton<UserMgr> {
friend class Singleton<UserMgr>;
public:
    ~UserMgr();

    std::shared_ptr<Session> getSession(int uid);
    void setUserSession(int uid, std::shared_ptr<Session> session);
    void rmvUserSession(int uid);
private:
    UserMgr();
    std::mutex m_mutex;
    std::unordered_map<int, std::shared_ptr<Session>> m_uid_to_session;
};

#endif // __USER_MGR_H__
