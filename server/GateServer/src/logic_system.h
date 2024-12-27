#ifndef __LOGICSYSTEM_H__
#define __LOGICSYSTEM_H__

#include <functional>
#include <map>

#include "const.h"
#include "http_connection.h"
#include "singleton.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
class LogicSystem : public Singleton<LogicSystem> {
public:
    ~LogicSystem();
    bool handlerGet(const std::string& path, std::shared_ptr<HttpConnection> con);
    bool handlerPost(const std::string& path, std::shared_ptr<HttpConnection> con);
    void regGet(const std::string& url, HttpHandler handler);
    void regPost(const std::string& url, HttpHandler handler);
private:
    friend class Singleton<LogicSystem>;
    LogicSystem();
    std::map<std::string, HttpHandler> m_get_handlers;
    std::map<std::string, HttpHandler> m_post_handlers;
};

#endif // __LOGICSYSTEM_H__
