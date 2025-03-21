#ifndef __CONST_H__
#define __CONST_H__

#include <functional>

class Defer {
public:
    Defer(std::function<void()> func) : m_func(func) {}
    ~Defer() {
        m_func();
    }
private:
    std::function<void()> m_func;
};

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
    TOKEN_INVALID = 1010,
    UID_INVALID = 1011,
};

#define MAX_LENGTH  1024*2
//头部总长度
#define HEAD_TOTAL_LEN 4
//头部id长度
#define HEAD_ID_LEN 2
//头部数据长度
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000

enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //用户登陆
    ID_CHAT_LOGIN = 1006,
	MSG_CHAT_LOGIN_RSP = 1007, //用户登陆回包
	ID_SEARCH_USER_REQ = 1008, //用户搜索请求
	ID_SEARCH_USER_RSP = 1009, //搜索用户回包
	ID_ADD_FRIEND_REQ = 1010, //申请添加好友请求
	ID_ADD_FRIEND_RSP  = 1011, //申请添加好友回复
	ID_NOTIFY_ADD_FRIEND_REQ = 1012,  //通知用户添加好友申请
	ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
	ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
	ID_TEXT_CHAT_MSG_REQ = 1017, //文本聊天信息请求
	ID_TEXT_CHAT_MSG_RSP = 1018, //文本聊天信息回复
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息
};

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"

#endif // __CONST_H__
