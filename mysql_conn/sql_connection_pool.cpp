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

sql_conn_pool* sql_conn_pool::GetSingleton ()
{
    static sql_conn_pool connPool;
    return &connPool;
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
void sql_conn_pool::init (std::string url, std::string User, std::string PassWd, std::string DatabaseName, int Port, int MaxConn, int close_log)
{
    m_close_log = close_log;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWd;
    m_DatabaseName = DatabaseName;
    m_close_log = close_log;

    for (int i = 0;i < m_MaxConn;i++)
    {
        MYSQL* conn = nullptr;
        conn = mysql_init (conn);

        if (nullptr == conn) {
            LOG_ERROR ("MySQL Error");
            exit (-1);
        }
        conn = mysql_real_connect (conn, url.c_str (), User.c_str (), PassWd.c_str (), DatabaseName.c_str (), Port, nullptr, 0);

        if (conn == nullptr) {
            LOG_ERROR ("MySql Connect Error");
            exit (-1);
        }
        connList.push_back (conn);
        ++m_FreeConn;
    }

    reserve = semaphore (m_FreeConn);

    m_MaxConn = m_FreeConn;
}

// when there a request, return a aviliable conn , update cur_conn & free_conn
MYSQL* sql_conn_pool::GetConnection ()
{
    MYSQL* conn = nullptr;

    if (0 == connList.size ())
        return nullptr;

    reserve.wait ();
    lock.lock ();

    conn = connList.front ();
    connList.pop_front ();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock ();
    return conn;
}

// release currently use conn
bool sql_conn_pool::ReleaseConnection (MYSQL* conn)
{
    if (nullptr == conn)
        return false;

    lock.lock ();

    connList.push_back (conn);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock ();

    reserve.post ();
    return true;
}

// return number of currently free conn
int sql_conn_pool::GetFreeConn ()
{
    return this->m_FreeConn;
}




sql_conn_RAII::sql_conn_RAII (MYSQL** sql, sql_conn_pool* connPool)
{
    *sql = connPool->GetConnection ();

    connRAII = *sql;
    poolRAII = connPool;
}
sql_conn_RAII::~sql_conn_RAII ()
{
    poolRAII->ReleaseConnection (connRAII);
}
