#include "http_connection.h"

#include <iostream>

#include "logic_system.h"


// char 转为16进制
unsigned char toHex(unsigned char x) {
	return x > 9 ? x + 55 : x + 48;
}

// char 十进制转十六进制
unsigned char fromHex(unsigned char x) {
	unsigned char y;
	if(x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if(x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if(x >= 'A' && x <= 'Z') y = x - '0';
	else assert(0);
	return y;
}

// url编码
std::string urlEncode(const std::string& str) {
	std::string str_temp = "";
	std::size_t length = str.length();
	for(std::size_t i = 0; i < length; ++i) {
		if(isalnum((unsigned char)str[i]) || (str[i] == '-') || (str[i] == '_')
					|| (str[i] == '.') || (str[i] == '~')) {
			str_temp += str[i];
		} else if(str[i] == ' ') {
			str_temp += '+';
		} else {
			str_temp += '%';
			str_temp += toHex((unsigned char)str[i] >> 4);
			str_temp += toHex((unsigned char)str[i] & 0x0F);
		}
	}
	return str_temp;
}

// url解码
std::string urlDecode(const std::string& str) {
	std::string str_temp = "";
	size_t length = str.length();
	for(size_t i = 0; i < length; ++i) {
		if(str[i] == '+') {
			str_temp += ' ';
		} else if(str[i] == '%') {
			assert(i + 2 < length);
			unsigned char high = fromHex((unsigned char)str[++i]);
			unsigned char low = fromHex((unsigned char)str[++i]);
			str_temp += (high + low);
		} else {
			str_temp += str[i];
		}
	}
	return str_temp;
}

/***********************************************
	HttpConnection
***********************************************/
HttpConnection::HttpConnection(tcp::socket socket) : m_socket(std::move(socket)){

}

void HttpConnection::start() {
	auto self = shared_from_this();
	http::async_read(m_socket, m_buffer, m_request, [self](beast::error_code ec,
												std::size_t bytes_transferred) {
		try {
			if (ec) {
				std::cout << "http read err is " << ec.value() << std::endl;
				return;
			}
			boost::ignore_unused(bytes_transferred);
			self->handleReq();
			self->checkDeadline();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
		}
	});
}

void HttpConnection::checkDeadline() {
	auto self = shared_from_this();
	m_deadline.async_wait([self](beast::error_code ec) {
		if(!ec) {
			self->m_socket.close();
		}
	});
}

void HttpConnection::writeResponse() {
	auto self = shared_from_this();
	m_response.content_length(m_response.body().size());
	http::async_write(m_socket, m_response, [self](beast::error_code ec, std::size_t) {
		self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
		self->m_deadline.cancel();
	});
}

void HttpConnection::handleReq() {
	m_response.version(m_request.version());
	m_response.keep_alive(false);
	bool success = false;
	if(m_request.method() == http::verb::get) {
		preParseGetParam();
		success = LogicSystem::GetInstance()->handlerGet(m_get_url, shared_from_this());
		// if(!success) {
		// 	m_response.result(http::status::not_found);
		// 	m_response.set(http::field::content_type, "text/plain");
		// 	beast::ostream(m_response.body()) << "url not found\r\n";
		// 	writeResponse();
		// }
		// m_response.result(http::status::ok);
		// m_response.set(http::field::server, "GateServer");
		// writeResponse();
	}
	if(m_request.method() == http::verb::post) {
		success = LogicSystem::GetInstance()->handlerPost(m_request.target(), shared_from_this());
		// success = LogicSystem::GetInstance()->handlerPost(m_request.target().to_string(), shared_from_this());
	}
	if(!success) {
		m_response.result(http::status::not_found);
		m_response.set(http::field::content_type, "text/plain");
		beast::ostream(m_response.body()) << "url not found\r\n";
		writeResponse();
	}
	m_response.result(http::status::ok);
	m_response.set(http::field::server, "GateServer");
	writeResponse();
}

void HttpConnection::preParseGetParam() {
	std::string uri = m_request.target();
	/// std::string uri = m_request.target().to_string();
	auto query_pos = uri.find('?');
	if(query_pos == std::string::npos) {
		m_get_url = uri;
		return;
	}
	m_get_url = uri.substr(0, query_pos);
	std::string query_str = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while((pos = query_str.find('&')) != std::string::npos) {
		std::string pair = query_str.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if(eq_pos != std::string::npos) {
			key = urlDecode(pair.substr(0, eq_pos));
			value = urlDecode(pair.substr(eq_pos + 1));
			m_get_params[key] = value;
		}
		query_str.erase(0, pos + 1);
	}
	if(!query_str.empty()) {
		size_t eq_pos = query_str.find('=');
		if(eq_pos != std::string::npos) {
			key = urlDecode(query_str.substr(0, eq_pos));
			value = urlDecode(query_str.substr(eq_pos + 1));
			m_get_params[key] = value;
		}
	}
}
