#ifndef __HTTPCONNECTION_H__
#define __HTTPCONNECTION_H__

#include "const.h"
#include <unordered_map>

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(tcp::socket socket);
	void start();
private:
	void checkDeadline();
	void writeResponse();
	void handleReq();
	void preParseGetParam();
	std::string m_get_url;
	std::unordered_map<std::string, std::string> m_get_params;
	tcp::socket m_socket;
	beast::flat_buffer m_buffer{ 8192 };
	http::request<http::dynamic_body> m_request;
	http::response<http::dynamic_body> m_response;
	net::steady_timer m_deadline{
		m_socket.get_executor(), std::chrono::seconds(60)
	};
};

#endif // __HTTPCONNECTION_H__
