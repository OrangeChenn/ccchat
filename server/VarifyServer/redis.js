const config_module = require('./config');
const Redis = require('ioredis');

const RedisCli = new Redis({
    host: config_module.redis_host,
    port: config_module.redis_port,
    password: config_module.redis_passwd,
});

/**
 * 监听错误信息
 */
RedisCli.on('error', function(err) {
    console.log("RedisCli connect error");
    RedisCli.quit();
});

/**
 * 根据key获取value
 * @param {*} key
 * @returns
 */
async function getRedis(key) {
    try {
        const result = await RedisCli.get(key);
        if(null == result) {
            console.log("result:", "<" + result + ">", "This key cannot be find....");
            return null;
        }
        console.log("result:", "<" + result + ">", "Get key success....");
        return result;
    }catch(error) {
        console.log('GetRedis error is', error);
        return null;
    }
};

/**
 * 根据key查询redis中是否存在key
 * @param {*} key
 * @returns
 */
async function queryRedis(key) {
    try {
        const result = await RedisCli.exists(key);
        if(null == result) {
            console.log("result:", "<" + result + ">", "This key is null...");
            return null;
        }
        console.log("result:", "<" + result + ">", "With this key...");
        return result;
    } catch(err) {
        console.log("QueryRedis error: ", err);
        return null;
    }
}

/**
 * 设置key和value，过期时间
 * @param {*} key
 * @param {*} value
 * @param {*} exptime
 * @returns
 */
async function setRedisExpire(key, value, exptime) {
    try {
        await RedisCli.set(key, value);
        await RedisCli.expire(key, exptime);
        return true;
    } catch(err) {
        console.log("setRedisExpire error: ", err);
        return null;
    }
}

/**
 * 退出函数
 */
function quit() {
    RedisCli.quit();
}

module.exports = {getRedis, queryRedis, setRedisExpire, quit}
