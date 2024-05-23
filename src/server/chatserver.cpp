#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : m_server(loop, listenAddr, nameArg), m_loop(loop)
{
    // 注册链接回调
    m_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    m_server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    m_server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    m_server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        conn->setTcpNoDelay(true);
        // LOG_WARN << "connected";
        SSLConnectionPtr ssl_connptr = make_shared<SSLConnection>(conn);
        ssl_connptr->set_type(SSL_TYPE::SERVER);
        m_connMap[conn] = ssl_connptr;
        ssl_connptr->set_receive_callback(std::bind(&ChatServer::handledata, this, _1, _2, _3));
        ssl_connptr->onConnection(conn);
        //std::bind(&SSL_Helper::onMessage, this, _1, _2, _3)
        //conn->setConnectionCallback()
    }
    // 客户端断开链接
    else
    {
        ChatService::instance()->clientCloseException(m_connMap[conn]);
        m_connMap.erase(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp timestamp)
{
    auto& ssl_helper = m_connMap[conn];
    ssl_helper->onMessage(conn, buf, timestamp);
}

// 上报读写事件相关信息的回调函数
int ChatServer::handledata(SSLConnection* ssl_conn, unsigned char* data, size_t datalen)
// (const TcpConnectionPtr &conn,
//                            Buffer *buffer,
//                            Timestamp time)
{
    string buf(data, data + datalen);
    SSLConnectionPtr ssl_connptr = m_connMap[ssl_conn->get_conn()];
    Timestamp time;

    // 测试，添加json打印代码
    //cout << buf << endl;

    // 数据的反序列化
    json js = json::parse(buf);
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取=》业务handler=》conn  js  time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(ssl_connptr, js, time);
}