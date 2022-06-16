#include"sql_connection_pool.h"

sql_conn_pool::sql_conn_pool ()
{
    m_CurConn = 0;
    m_FreeConn = 0;
}
sql_conn_pool::~sql_conn_pool ()
{
    DestoryPool ();
}

// destory db conn pool
void sql_conn_pool::DestoryPool ()
{
    lock.lock ();
    if (connList.size () > 0)
    {
        std::list<MYSQL*>::iterator it;
        for (it = connList.begin ();it != connList.end ();it++) {
            MYSQL* conn = *it;
            mysql_close (conn);
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear ();
    }
    lock.unlock ();
}

// initialization
void sql_conn_pool::init (std::string url, std::string User, std::string PassWd, std::string DatabaseName, int Port, int MaxConn, int close_log) :
    m_url (url), m_User (User), m_PassWord (PassWd), m_DatebaseName (DatabaseName)
{
    m_close_log = close_log;

    for (int i = 0;i < m_MaxConn;i++)
    {
        MYSQL* conn = nullptr;
        conn = mysql_init (conn);

        if (nullptr == conn) {
            
        }
    }
}
