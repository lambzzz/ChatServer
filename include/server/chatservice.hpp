#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "redis.hpp"
#include "groupmodel.hpp"
#include "friendmodel.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "json.hpp"
#include "SSLConnection.h"
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const SSLConnectionPtr &conn, json &js, Timestamp)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 处理登录业务
    void login(const SSLConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const SSLConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const SSLConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const SSLConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const SSLConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const SSLConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const SSLConnectionPtr &conn, json &js, Timestamp time);
    // 处理注销业务
    void loginout(const SSLConnectionPtr &conn, json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const SSLConnectionPtr &conn);
    // 服务器异常，业务重置方法
    void reset();
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    ChatService();

    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> m_msgHandlerMap;
    // 存储在线用户的通信连接
    unordered_map<int, SSLConnectionPtr> m_userConnMap;
    // 定义互斥锁，保证_userConnMap的线程安全
    mutex m_connMutex;

    // 数据操作类对象
    UserModel m_userModel;
    OfflineMsgModel m_offlineMsgModel;
    FriendModel m_friendModel;
    GroupModel m_groupModel;

    // redis操作对象
    Redis m_redis;
};

#endif