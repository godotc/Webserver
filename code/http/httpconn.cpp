#include "httpconn.h"
#include <arpa/inet.h>
#include <atomic>
#include <sys/fcntl.h>
#include <unistd.h>


const char      *HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool             HttpConn::isET;


HttpConn::HttpConn ()
{
    fd_      = -1;
    addr_    = { 0 };
    isClose_ = true;
}
HttpConn::~HttpConn ()
{
    Close ();
}

void
HttpConn::Close ()
{
    response_.UnmapFile ();
    if (isClose_ == false)
    {
        isClose_ = true;
        userCount--;
        close (fd_);
        LOG_INFO ("CLient[%d](%s:%d] quit, UserCount:%d", fd_, GetIP (), GetPort (), (int)userCount);
    }
}

int
HttpConn::GetFd () const
{
    return fd_;
}
int
HttpConn::GetPort () const
{
    return addr_.sin_port;
}
const char *
HttpConn::GetIP () const
{
    return inet_ntoa (addr_.sin_addr);
}
sockaddr_in
HttpConn::GetAddr () const
{
    return addr_;
}
