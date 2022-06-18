#include"webserver.h"

Webserver::Webserver()
{
    // object of http_conn
    users = new http_conn[MAX_FD];

    // dir of root
    char server_path[200];
    getcwd(server_path , 200);
    char root[6] = "/root";
    m_root = (char*)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root , server_path);
    strcat(m_root , root);

    // thread_pools


    // timer 
    users_timer = new client_data[MAX_FD];
}
Webserver::~Webserver()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[]users;
    delete[] users_timer;
    delete m_pool;
}


void Webserver::init(int port , std::string user , std::string passwd , std::string databaseName , int log_write , int opt_linger ,
    int trigmode , int sql_num , int thread_num , int close_log , int actor_model)
{
    m_port = port;
    m_user = user;
    m_databaseName = databaseName;
    m_sql_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

void Webserver::thread_pool()
{
    m_pool = new threadpool<http_conn>(m_actormodel , m_connPool , m_thread_num);
}
void Webserver::sql_pool()
{
    // init connection
    m_connPool = sql_conn_pool::GetSingleton();
    m_connPool->init("localhost" , m_user , m_password , m_databaseName , 3306 , m_sql_num , m_close_log);

    // init users table map
    users->initmysql_result(m_connPool);
}

void Webserver::log_write()
{
    if (0 == m_close_log)
    {
        // init LOG
        if (1 == m_log_write)
            Log::get_singleton()->init("./ServerLog" , m_close_log , 2000 , 800000 , 800);
        else
            Log::get_singleton()->init("./ServerLog" , m_close_log , 2000 , 800000 , 0);
    }
}
void Webserver::trig_mode()
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

void Webserver::eventListen()
{
    // socket
    m_listenfd = socket(PF_INET , SOCK_STREAM , 0);
    assert(m_listenfd >= 0);

    // close conn gracefully(优雅关闭链接)
    if (0 == m_OPT_LINGER)
    {
        struct linger temp = { 0 , 1 };
        setsockopt(m_listenfd , SOL_SOCKET , SO_LINGER , &temp , sizeof(temp));
    }
    else if (1 == m_OPT_LINGER)
    {
        struct linger temp = { 1,1 };
        setsockopt(m_listenfd , SOL_SOCKET , SO_LINGER , &temp , sizeof(temp));
    }

    int ret = 0;
    struct sockaddr_in address;
    memset(&address , 0 , sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);

    // reuse port
    int flag = 1;
    setsockopt(m_listenfd , SOL_SOCKET , SO_REUSEADDR , &flag , sizeof(flag));

    // bind 
    ret = bind(m_listenfd , (struct sockaddr*)&address , sizeof(address));
    assert(ret >= 0);

    //listen
    ret = listen(m_listenfd , 8);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    // create epoll kernel event table
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    /*----------------------------------------------------------------*/
    utils.addfd(m_epollfd , m_listenfd , false , m_LISTENTRgimode);
    http_conn::m_epollfd = m_epollfd;
    /*---------------------------------------------------------------*/

    ret = socketpair(PF_UNIX , SOCK_STREAM , 0 , m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd , m_pipefd[0] , false , 0);

    utils.addsig(SIGPIPE , SIG_IGN);
    utils.addsig(SIGALRM , utils.sig_handler , false);
    utils.addsig(SIGTERM , utils.sig_handler , false);

    alarm(TIMESLOT);

    // utils class fundation operation(工具类、信号、描述符基本操作)
    Utils::u_epollfd = m_epollfd;
    Utils::u_epollfd = m_epollfd;
}

