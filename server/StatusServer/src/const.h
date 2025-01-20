#ifndef __CONST_H__
#define __CONST_H__

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

enum ErrorCodes {
    SUCCESS = 0,
    JSON_ERROR = 1001,  // Json错误码
    RPC_FAILED = 1002,   // RPC请求错误
    VARIFY_EXPIRED = 1003, // 验证码超时
    VARIFY_CODE_ERR = 1004,
    USER_EXISTS = 1005,
    PASSWD_ERROR = 1006,
    EMAIL_NOT_MATCH = 1007,
    PASSWD_UPDATE_ERROR = 1008,
    PASSWD_INVALID = 1009,
};

const std::string CODEPREFIX = "code_";

#endif // __CONST_H__
