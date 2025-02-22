#ifndef __LOGICSYSTEM_H__
#define __LOGICSYSTEM_H__

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include "msg_node.h"
#include "session.h"
#include "singleton.h"

typedef std::function<void(std::shared_ptr<Session>, short, const std::string&)> FuncCallBack;

class LogicSystem : public Singleton<LogicSystem> {
public:
    ~LogicSystem();

    void postMsgToQue(std::shared_ptr<Session> session, std::shared_ptr<RecvNode> recv_node);
    void logicHandler(std::shared_ptr<Session> session, short msg_id, const std::string& msg_data);
private:
    friend class Singleton<LogicSystem>;
    LogicSystem();

    void registerCallBacks();
    void dealMsg();

private:
    std::thread m_work_thread;
    std::mutex m_mutex;
    std::queue<std::shared_ptr<LogicNode>> m_msg_que;
    std::map<short, FuncCallBack> m_fun_callbacks;
    bool m_stop;
};

#endif // __LOGICSYSTEM_H__
