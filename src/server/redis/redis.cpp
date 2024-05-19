#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis()
    : m_publish_context(nullptr), m_subcribe_context(nullptr), redis_password("1359")
{
}

Redis::~Redis()
{
    if (m_publish_context != nullptr)
    {
        redisFree(m_publish_context);
    }

    if (m_subcribe_context != nullptr)
    {
        redisFree(m_subcribe_context);
    }
}

bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    m_publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == m_publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    m_subcribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == m_subcribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    m_data_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == m_data_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }
    
    if (!redis_password.empty())
    {
        if (((redisReply *)redisCommand(m_publish_context, "AUTH %s", redis_password))->type == REDIS_REPLY_ERROR)
        {
            cerr << "wrong redis password!" << endl;
            return false;
        }
        if (((redisReply *)redisCommand(m_subcribe_context, "AUTH %s", redis_password))->type == REDIS_REPLY_ERROR)
        {
            cerr << "wrong redis password!" << endl;
            return false;
        }
        if (((redisReply *)redisCommand(m_data_context, "AUTH %s", redis_password))->type == REDIS_REPLY_ERROR)
        {
            cerr << "wrong redis password!" << endl;
            return false;
        }
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;

    return true;
}

string Redis::get_state(int id){
    redisReply *reply = (redisReply *)redisCommand(m_data_context, "GET %d", id);
    if(reply->len == 0){
        cout<<"user "<<id<<" is not exist in redis"<<endl;
        return "null";
    }
    std::string value(reply->str);
    freeReplyObject(reply);
    return value;

}

bool Redis::set_state(int id, string state){
    redisReply *reply = (redisReply *)redisCommand(m_data_context, "SET %d %s", id, state.c_str());
    if (nullptr == reply)
    {
        cerr << "set command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::del_user(int id){
    redisReply *reply = (redisReply *)redisCommand(m_data_context, "DEL %d", id);
    if (nullptr == reply)
    {
        cerr << "set command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(m_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->m_subcribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->m_subcribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetReply

    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->m_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->m_subcribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->m_subcribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            m_notify_message_handler(atoi(reply->element[1]->str) , reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int,string)> fn)
{
    this->m_notify_message_handler = fn;
}