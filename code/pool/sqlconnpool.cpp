#include "sqlconnpool.h"
#include "../log/log.h"
#include <cassert>
#include <mutex>
#include <mysql/mysql.h>
#include <semaphore.h>

SqlConnPool ::SqlConnPool ()
{
    userCount_ = 0;
    freeCount_ = 0;
}
SqlConnPool::~SqlConnPool ()
{
    ClosePool ();
}

SqlConnPool *
SqlConnPool::Instance ()
{
    static SqlConnPool connPool;
    return &connPool;
}

void
SqlConnPool::ClosePool ()
{
    std::lock_guard<std::mutex> locker (mtx_);
    while (!connQue_.empty ())
    {
        auto item = connQue_.front ();
        connQue_.pop ();
        mysql_close (item);
    }
    mysql_library_end ();
}


MYSQL *
SqlConnPool::GetConn ()
{
    MYSQL *sql = nullptr;
    if (connQue_.empty ())
    {
        LOG_WARN ("SqlConnPool busy!!");
        return nullptr;
    }

    sem_wait (&semId_);

    {
        std::lock_guard<std::mutex> locker (mtx_);
        sql = connQue_.front ();
        connQue_.pop ();
    }
    return sql;
}

void
SqlConnPool::Init (const char *host, int port,
                   const char *user, const char *passwd,
                   const char *dbName, int connsize = 10)
{
    assert (connsize > 0);

    for (int i = 0; i < connsize; ++i)
    {
        MYSQL *sql = nullptr;
        sql        = mysql_init (sql);
        if (!sql)
        {
            LOG_ERROR ("MySql init error!");
            assert (sql);
        }

        sql = mysql_real_connect (sql,
                                  host, user, passwd,
                                  dbName,
                                  port, nullptr, 0);
        if (!sql)
        {
            LOG_ERROR ("MySql Connect error!");
        }
        connQue_.push (sql);
    }

    MAX_CONN_ = connsize;
    sem_init (&semId_, 0, MAX_CONN_);
}