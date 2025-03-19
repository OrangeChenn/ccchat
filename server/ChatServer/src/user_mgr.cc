#include "user_mgr.h"

UserMgr::UserMgr() {

}

UserMgr::~UserMgr() {
    m_uid_to_session.clear();
}

std::shared_ptr<Session> UserMgr::getSession(int uid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto iter = m_uid_to_session.find(uid);
    if(iter == m_uid_to_session.end()) {
        return nullptr;
    }
    return iter->second;
}

void UserMgr::setUserSession(int uid, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_uid_to_session[uid] = session;
}

void UserMgr::rmvUserSession(int uid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_uid_to_session.erase(uid);
}
