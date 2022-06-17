#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include<mysql/mysql.h>
#include"../lock/lock.h"
#include<list>
#include<string>
#include"../log/log.h"

class sql_conn_pool
{
private:
    int m_MaxConn;  // number of max conn
    int m_CurConn;  // number of current conn in use
    int m_FreeConn; // number of current free conn
    locker lock;
    semaphore reserve;
    std::list<MYSQL*>connList;   // pool of conn

    sql_conn_pool ();
    ~sql_conn_pool ();

public:
    std::string m_url;      // the addr of host;
    std::string m_Port;     // port of Database
    std::string m_User;     // user for db;
    std::string m_PassWord; // passwd of dbr;
    std::string m_DatabaseName;     // the database name which choose
    int m_close_log;    // switch of log

public:
    void init (std::string url, std::string User, std::string Passwd, std::string DatabaseName, int Port, int maxConn, int close_log);

    // the Singleton Pattern(单例模式)
    static sql_conn_pool* GetSingleton ();

    MYSQL* GetConnection ();    // get the conn of db
    bool ReleaseConnection (MYSQL* conn);
    int GetFreeConn ();     // get the number of free conn
    void DestoryPool ();     // destory all conn
};


class sql_conn_RAII
{
private:
    MYSQL* connRAII;
    sql_conn_pool* poolRAII;

public:
    sql_conn_RAII (MYSQL** conn, sql_conn_pool* connPoll);
    ~sql_conn_RAII ();
};

#endif
