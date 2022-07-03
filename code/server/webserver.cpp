#include "webserver.h"
#include "epoller.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>


WebServer::WebServer (int port, int trigMode, int timeoutMS,
                      bool Optlinger,
                      int sqlPort, const char *sqlUser, const char *sqlPawd, const char *dbName,
                      int connPoolNum, int threadNum,
                      int openLog, int logLevel, int logQueSize)
    : port_ (port), openLinger_ (Optlinger),
      timeoutMS_ (timeoutMS), isClose_ (false),
      timer_ (new HeapTimer),
      threadpool_ (new ThreadPool (threadNum)),
      epoller_ (new Epoller)
{
    srcDir_ = getcwd (nullptr, 256);
    assert (srcDir_);
    strncat (srcDir_, "/resoures/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir    = srcDir_;
    SqlConnPool::Instance ()->Init ("localhost", sqlPort,
                                    sqlUser, sqlPawd, dbName, connPoolNum);

    InitEventMode_ (trigMode);
    if (!InitSocket_ ())
    {
        isClose_ = true;
    }

    if (openLog)
    {
        Log::Instance ()->init (logLevel, "./log", ".log", logQueSize);
        if (isClose_)
        {
            LOG_ERROR ("==============Server init error!======================");
        }
        else
        {
            LOG_INFO ("===============Server init==================");
            LOG_INFO ("Port:%d, OpenLinger: %s", port_, Optlinger ? "true" : "false");
            LOG_INFO ("Listen Mode:%s, OpenLinger:%s",
                      (listenEvent_ & EPOLLET ? "ET" : "LT"),
                      (connEvent_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO ("LogSys level: %d", logLevel);
            LOG_INFO ("srcDir: %s", HttpConn::srcDir);
            LOG_INFO ("SqlConnPool num:%d,ThreadPool num:%d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer ()
{
    close (listenFd_);
    isClose_ = true;
    free (srcDir_);
    SqlConnPool::Instance ()->ClosePool ();
}

void
WebServer::InitEventMode_ (int trigMode)
{
    listenEvent_ = EPOLLRDHUP;
    connEvent_   = EPOLLONESHOT | EPOLLRDHUP;

    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
    case 2:
        listenEvent_ |= EPOLLET;
    case 3:
        connEvent_ |= EPOLLET;
        listenEvent_ |= EPOLLET;
        break;
    default:
        connEvent_ |= EPOLLET;
        listenEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

bool
WebServer::InitSocket_ ()
{
    int                ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024)
    {
        LOG_ERROR ("Port:%d error!", port_);
        return false;
    }
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
    addr.sin_port        = htons (port_);

    struct linger optLinger = { 0 };
    if (openLinger_)
    {
        // 优雅关闭：直到所生数据发送完毕或超时
        optLinger.l_linger = 1;
        optLinger.l_onoff  = 1;
    }
    listenFd_ = socket (AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
    {
        LOG_ERROR ("Create socket error!", port_);
        return false;
    }
    ret = setsockopt (listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof (optLinger));
    if (ret < 0)
    {
        close (listenFd_);
        LOG_ERROR ("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    // Reuse port
    ret = setsockopt (listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof (optval));
    if (-1 == ret)
    {
        LOG_ERROR ("Set socket setsockopt error !");
        close (listenFd_);
        return false;
    }

    ret = bind (listenFd_, (struct sockaddr *)&addr, sizeof (addr));
    if (ret < 0)
    {
        LOG_ERROR ("Bind Port:%d error!", port_);
        close (listenFd_);
        return false;
    }

    ret = listen (listenFd_, 6);
    if (ret < 0)
    {
        LOG_ERROR ("Listen prot:%d error!", port_);
        close (listenFd_);
        return false;
    }

    ret = epoller_->AddFd (listenFd_, listenEvent_ | EPOLLIN);
    if (0 == ret)
    {
        LOG_ERROR ("Add listen error!");
        close (listenFd_);
        return false;
    }

    SetFdNonBlock (listenFd_);
    LOG_INFO ("Server port:%d", port_);
    return true;
}

int
WebServer::SetFdNonBlock (int fd)
{
    assert (fd > 0);
    return fcntl (fd, F_SETFL, fcntl (fd, F_GETFD, 0) | O_NONBLOCK);
}