#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include<mysql/mysql.h>
#include"../lock/lock.h"
#include<list>
#include<string>

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
    std::string m_DatebaseName;     // the database name which choose
    int m_close_log;    // switch of log

public:
    MYSQL* GetConnection ();    // get the conn of db
    bool ReleaseConnection (MYSQL* conn);
    int GetFreeConn ();     //
};


#endif
