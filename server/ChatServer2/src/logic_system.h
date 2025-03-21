#ifndef __LOGICSYSTEM_H__
#define __LOGICSYSTEM_H__
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include "data.h"
#include "msg_node.h"
#include "session.h"
#include "singleton.h"

class Session;
class LogicNode;
typedef std::function<void(std::shared_ptr<Session>, short, const std::string&)> FuncCallBack;

class LogicSystem : public Singleton<LogicSystem> {
public:
    ~LogicSystem();

    void postMsgToQue(std::shared_ptr<LogicNode> node);
    void logicHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data);
private:
    friend class Singleton<LogicSystem>;
    LogicSystem();

    void registerCallBacks();
    void dealMsg();
    bool getBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo> &user_info);

private:
    std::thread m_work_thread;
    std::condition_variable m_cond;
    std::mutex m_mutex;
    std::queue<std::shared_ptr<LogicNode>> m_msg_que;
    std::map<short, FuncCallBack> m_fun_callbacks;
    std::map<int, std::shared_ptr<UserInfo>> m_users;
    bool m_stop;
};

#endif // __LOGICSYSTEM_H__
