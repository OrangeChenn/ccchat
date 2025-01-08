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
    Success = 0,
    ErrorJson = 1001,  // Json错误码
    RPCFailed = 1002,   // RPC请求错误
    VarifyExpired = 1003, // 验证码超时
    VarifyCodeErr = 1004,
    UserExists = 1005,
    PasswdErr = 1006,
};

#endif // __CONST_H__
