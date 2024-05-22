#include "db.h"
#include <muduo/base/Logging.h>

// 数据库配置信息
// static string server = "127.0.0.1";
// static string user = "root";
// static string password = "root";
// static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL()
{
    // m_conn = mysql_init(nullptr);
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    // if (m_conn != nullptr)
    //     mysql_close(m_conn);
}

// 连接数据库
bool MySQL::connect(MYSQL* m_sql)
{
    m_conn = m_sql;
    if (m_conn != nullptr)
    {
        // C和C++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文显示？
        mysql_query(m_conn, "set names utf8");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
        return false;
    }
    
    return true;
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }

    return true;
}

// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    
    return mysql_use_result(m_conn);
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return m_conn;
}