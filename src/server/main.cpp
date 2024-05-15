#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
#include "sqlconnpool.h"
using namespace std;

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    static string host = "127.0.0.1";
    static string user = "root";
    static string password = "root";
    static string dbname = "chat";
    static int connSize = 10;

    SqlConnPool::Instance()->Init(host.c_str(),3306,user.c_str(),password.c_str(),dbname.c_str(),connSize);

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}