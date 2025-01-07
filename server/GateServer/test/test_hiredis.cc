#include <assert.h>
#include <hiredis/hiredis.h>
#include <string.h>
#include <string>
#include "../src/redis_mgr.h"

void TestRedis() {
    //连接redis 需要启动才可以进行连接
//redis默认监听端口为6387 可以再配置文件中修改
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c->err)
    {
        printf("Connect to redisServer faile:%s\n", c->errstr);
        redisFree(c);        return;
    }
    printf("Connect to redisServer Success\n");

    std::string redis_password = "123456";
    redisReply* r = (redisReply*)redisCommand(c, "AUTH %s", redis_password.c_str());
     if (r->type == REDIS_REPLY_ERROR) {
         printf("Redis认证失败！\n");
    }else {
        printf("Redis认证成功！\n");
         }

    //为redis设置key
    const char* command1 = "set stest1 value1";

    //执行redis命令行
    r = (redisReply*)redisCommand(c, command1);

    //如果返回NULL则说明执行失败
    if (NULL == r)
    {
        printf("Execut command1 failure\n");
        redisFree(c);        return;
    }

    //如果执行失败则释放连接
    if (!(r->type == REDIS_REPLY_STATUS && (strcmp(r->str, "OK") == 0 || strcmp(r->str, "ok") == 0)))
    {
        printf("Failed to execute command[%s]\n", command1);
        freeReplyObject(r);
        redisFree(c);        return;
    }

    //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command1);

    const char* command2 = "strlen stest1";
    r = (redisReply*)redisCommand(c, command2);

    //如果返回类型不是整形 则释放连接
    if (r->type != REDIS_REPLY_INTEGER)
    {
        printf("Failed to execute command[%s]\n", command2);
        freeReplyObject(r);
        redisFree(c);        return;
    }

    //获取字符串长度
    int length = r->integer;
    freeReplyObject(r);
    printf("The length of 'stest1' is %d.\n", length);
    printf("Succeed to execute command[%s]\n", command2);

    //获取redis键值对信息
    const char* command3 = "get stest1";
    r = (redisReply*)redisCommand(c, command3);
    if (r->type != REDIS_REPLY_STRING)
    {
        printf("Failed to execute command[%s]\n", command3);
        freeReplyObject(r);
        redisFree(c);        return;
    }
    printf("The value of 'stest1' is %s\n", r->str);
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command3);

    const char* command4 = "get stest2";
    r = (redisReply*)redisCommand(c, command4);
    if (r->type != REDIS_REPLY_NIL)
    {
        printf("Failed to execute command[%s]\n", command4);
        freeReplyObject(r);
        redisFree(c);        return;
    }
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command4);

    //释放连接资源
    redisFree(c);

}

void TestRedisMgr() {
    // assert(RedisMgr::GetInstance()->connect("127.0.0.1", 6379));
    assert(RedisMgr::GetInstance()->auth("123456"));
    assert(RedisMgr::GetInstance()->set("blogwebsite","llfc.club"));
    std::string value="";
    assert(RedisMgr::GetInstance()->get("blogwebsite", value) );
    assert(RedisMgr::GetInstance()->get("nonekey", value) == false);
    assert(RedisMgr::GetInstance()->hset("bloginfo","blogwebsite", "llfc.club"));
    assert(RedisMgr::GetInstance()->hget("bloginfo","blogwebsite") != "");
    assert(RedisMgr::GetInstance()->existsKey("bloginfo"));
    assert(RedisMgr::GetInstance()->del("bloginfo"));
    assert(RedisMgr::GetInstance()->del("bloginfo"));
    assert(RedisMgr::GetInstance()->existsKey("bloginfo") == false);
    assert(RedisMgr::GetInstance()->lpush("lpushkey1", "lpushvalue1"));
    assert(RedisMgr::GetInstance()->lpush("lpushkey1", "lpushvalue2"));
    assert(RedisMgr::GetInstance()->lpush("lpushkey1", "lpushvalue3"));
    assert(RedisMgr::GetInstance()->lpop("lpushkey1", value));
    assert(RedisMgr::GetInstance()->rpop("lpushkey1", value));
    assert(RedisMgr::GetInstance()->lpop("lpushkey1", value));
    assert(RedisMgr::GetInstance()->lpop("lpushkey2", value)==false);
    RedisMgr::GetInstance()->close();
}

int main(int argc, char** argv) {
    // TestRedis();
    TestRedisMgr();
    return 0;
}
