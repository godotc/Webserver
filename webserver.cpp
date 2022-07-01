#include "webserver.h"
#include "lock/lock.h"
#include "mysql_conn/sql_connection_pool.h"
#include "timer/list_timer.h"
#include <cstddef>
#include <map>
#include <mysql/mysql.h>
#include <string>

Webserver::Webserver ()
{
    // object of http_conn
    users = new http_conn[MAX_FD];

    // dir of root
    char server_path[200];
    getcwd (server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc (strlen (server_path) + strlen (root) + 1);
    strcpy (m_root, server_path);
    strcat (m_root, root);

    // thread_pools

    // timer
    users_timer = new client_data[MAX_FD];
}
Webserver::~Webserver ()
{
    close (m_epollfd);
    close (m_listenfd);
    close (m_pipefd[1]);
    close (m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}

void
Webserver::init (int port, std::string user, std::string passwd,
                 std::string databaseName, int log_write, int opt_linger,
                 int trigmode, int sql_num, int thread_num, int close_log,
                 int actor_model)
{
    m_port = port;
    m_user = user;
    m_passsword = passwd;
    m_databaseName = databaseName;
    m_sql_num = thread_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

/* void
http_conn::initmysql_result (sql_conn_pool *sql_conn_pool)
{
    // fetch an conn from sql_coon_pool
    MYSQL *mysql = nullptr;
    sql_conn_RAII mysqlconn (&mysql, sql_conn_pool);

    // check username,passswd in table user
    if (mysql_query (mysql, "select username,passwd from user"))
        {
            LOG_ERROR ("SELECT error:%s\n", mysql_error (mysql));
        }

    // get result
    MYSQL_RES *result = mysql_store_result (mysql);

    // get cols
    int num_files = mysql_num_fields (result);

    // get array
    MYSQL_FIELD *fields = mysql_fetch_fields (result);

    // getting next col, insert to map:users
    while (MYSQL_ROW row = mysql_fetch_row (result))
        {
            std::string username (row[0]);
            std::string passwd (row[1]);
            users[username] = passwd;
        }

} */

void
Webserver::thread_pool ()
{
    m_pool = new threadpool<http_conn> (m_actormodel, m_sqlConnPool,
                                        m_thread_num);
}
void
Webserver::sql_pool ()
{
    // init connection
    m_sqlConnPool = sql_conn_pool::GetSingleton ();
    m_sqlConnPool->init ("localhost", m_user, m_passsword, m_databaseName,
                         3306, m_sql_num, m_close_log);

    // init users table map
    users->initmysql_result (m_sqlConnPool);
}

void
Webserver::log_write ()
{
    if (0 == m_close_log)
    {
        // init LOG
        if (1 == m_log_write)
            Log::get_singleton ()->init ("./ServerLog", m_close_log, 2000,
                                         800000, 800);
        else
            Log::get_singleton ()->init ("./ServerLog", m_close_log, 2000,
                                         800000, 0);
    }
}
void
Webserver::trig_mode ()
{
    // LT+LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTRgimode = 0;
        m_CONNTRigmode = 0;
    }
    // LT+ET
    if (1 == m_TRIGMode)
    {
        m_LISTENTRgimode = 0;
        m_CONNTRigmode = 1;
    }
    // ET+LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTRgimode = 1;
        m_CONNTRigmode = 0;
    }
    // ET+ET
    if (0 == m_TRIGMode)
    {
        m_LISTENTRgimode = 1;
        m_CONNTRigmode = 1;
    }
}

void
Webserver::eventListen ()
{
    // socket
    m_listenfd = socket (PF_INET, SOCK_STREAM, 0);
    assert (m_listenfd >= 0);

    // close conn gracefully(优雅关闭链接)
    if (0 == m_OPT_LINGER)
    {
        struct linger temp = { 0, 1 };
        setsockopt (m_listenfd, SOL_SOCKET, SO_LINGER, &temp,
                    sizeof (temp));
    }
    else if (1 == m_OPT_LINGER)
    {
        struct linger temp = { 1, 1 };
        setsockopt (m_listenfd, SOL_SOCKET, SO_LINGER, &temp,
                    sizeof (temp));
    }

    int                ret = 0;
    struct sockaddr_in address;
    memset (&address, 0, sizeof (address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons (m_port);

    // reuse port
    int flag = 1;
    setsockopt (m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag));

    // bind
    ret = bind (m_listenfd, (struct sockaddr *)&address, sizeof (address));
    assert (ret >= 0);

    // listen
    ret = listen (m_listenfd, 8);
    assert (ret >= 0);

    utils.init (TIMESLOT);

    // create epoll kernel event table
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create (5);
    assert (m_epollfd != -1);

    /*----------------------------------------------------------------*/
    utils.addfd (m_epollfd, m_listenfd, false, m_LISTENTRgimode);
    http_conn::m_epollfd = m_epollfd;
    /*---------------------------------------------------------------*/

    ret = socketpair (PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert (ret != -1);
    utils.setnonblocking (m_pipefd[1]);
    utils.addfd (m_epollfd, m_pipefd[0], false, 0);

    utils.addsig (SIGPIPE, SIG_IGN);
    utils.addsig (SIGALRM, utils.sig_handler, false);
    utils.addsig (SIGTERM, utils.sig_handler, false);

    alarm (TIMESLOT);

    // utils class fundation operation(工具类、信号、描述符基本操作)
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

void
Webserver::eventLoop ()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait (m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR ("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            // deal listen---> accept()
            if (sockfd == m_listenfd)
            {
                bool flag = deal_clientdata ();
                if (false == flag)
                    continue;
            }
            // server close that conn, remove its timer
            else if (events[i].events
                     & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
            {
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer (timer, sockfd);
            }
            // handler signal
            else if ((sockfd == m_pipefd[0])
                     && (events[i].events & EPOLLIN))
            {
                bool flag
                    = deal_with_signal (timeout, stop_server);
                if (false == flag)
                    LOG_ERROR ("%s", "dealcientdata failure");
            }
            // deal data that client recv
            else if (events[i].events & EPOLLIN)
            {
                deal_with_read (sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                deal_with_write (sockfd);
            }
        }
        if (timeout)
        {
            utils.timer_handler ();

            LOG_INFO ("%s", "timer tick");

            timeout = false;
        }
    }
}

void
Webserver::timer (int connfd, struct sockaddr_in client_address)
{
    users[connfd].init (connfd, client_address, m_root, m_CONNTRigmode,
                        m_close_log, m_user, m_passsword, m_databaseName);

    // init client_data data
    // Create timer, Set handler and timeout , Bind users' info, Add timer to
    // link list
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;

    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time (nullptr);
    timer->expire = cur + 3 * TIMESLOT;

    users_timer[connfd].timer = timer;
    utils.m_timer_list.add_timer (timer);
}

// when transfer data, delay timer 3 units
// then adjust the position of new tiemr on link list
void
Webserver::adjust_timer (util_timer *timer)
{
    time_t cur = time (nullptr);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_list.adjust_tiemr (timer);

    LOG_INFO ("%s", "adjust timer once");
}

// remove timer?
void
Webserver::deal_timer (util_timer *timer, int sockfd)
{
    timer->cb_func (&users_timer[sockfd]);
    if (timer)
    {
        utils.m_timer_list.del_timer (timer);
    }

    LOG_INFO ("close fd %d", users_timer[sockfd].sockfd);
}

bool
Webserver::deal_clientdata ()
{
    struct sockaddr_in client_addres;
    socklen_t          client_addr_len = sizeof (client_addres);

    // ET
    if (0 == m_LISTENTRgimode)
    {
        int connfd = accept (m_listenfd, (struct sockaddr *)&client_addres,
                             &client_addr_len);
        if (connfd < 0)
        {
            LOG_ERROR ("%s:erron is:%s", "accept error", errno);
            return false;
        }
        if (http_conn::m_user_count >= MAX_FD)
        {
            utils.show_erroer (connfd, "Internal server busy");
            LOG_ERROR ("%s", "Internal server busy");
            return false;
        }
        timer (connfd, client_addres);
    }

    // LT
    else
    {
        while (1)
        {
            int connfd = accept (m_listenfd,
                                 (struct sockaddr *)&client_addres,
                                 &client_addr_len);
            if (connfd < 0)
            {
                LOG_ERROR ("%s:errno is: %d", "accept error",
                           errno);
                break;
                ;
            }
            if (http_conn::m_user_count >= MAX_FD)
            {
                utils.show_erroer (connfd, "Internal server busy");
                LOG_ERROR ("%s", "Internal server busy");
                break;
            }
            timer (connfd, client_addres);
        }
        return false;
    }
    return false;
}

bool
Webserver::deal_with_signal (bool &timeout, bool &stop_server)
{
    int  ret = 0;
    int  sig;
    char signals[1024];
    ret = recv (m_pipefd[0], signals, sizeof (signals), 0);
    if (-1 == ret)
    {
        return false;
    }
    else if (0 == ret)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGALRM:
                timeout = true;
                break;
            case SIGTERM:
                stop_server = true;
                break;
            }
        }
    }

    return true;
}

void
Webserver::deal_with_read (int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;

    // reactor
    if (1 == m_actormodel)
    {
        if (timer)
        {
            adjust_timer (timer);
        }

        // if detect read event, push it to requeset queue
        m_pool->append (users + sockfd, 0);

        while (1)
        {
            if (1 == users[sockfd].timer_flag)
            {
                deal_timer (timer, sockfd);
                users[sockfd].timer_flag = 0;
            }

            users[sockfd].improv = 0;
            break;
        }
    }
    // proactor
    else
    {
        if (users[sockfd].read_once ())
        {
            LOG_INFO (
                "deal with the client(%s)",
                inet_ntoa (users[sockfd].get_address ()->sin_addr));

            // if detect read event, append to request queue
            m_pool->append_p (users + sockfd);

            if (timer)
            {
                adjust_timer (timer);
            }
        }
        else
        {
            deal_timer (timer, sockfd);
        }
    }
}

void
Webserver::deal_with_write (int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;

    // reactor
    if (1 == m_actormodel)
    {
        if (timer)
        {
            adjust_timer (timer);
        }
        m_pool->append (users + sockfd, 1);

        while (1)
        {
            if (1 == users[sockfd].improv)
            {
                if (1 == users[sockfd].timer_flag)
                {
                    deal_timer (timer, sockfd);
                    users[sockfd].improv = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }

    // proactor
    else
    {
        if (users[sockfd].write ())
        {
            LOG_INFO (
                "send data to the client(%s)",
                inet_ntoa (users[sockfd].get_address ()->sin_addr));

            if (timer)
            {
                adjust_timer (timer);
            }
        }
        else
        {
            deal_timer (timer, sockfd);
        }
    }
}
