#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "../log/log.h"
#include "httprequest.h"
#include "httpresponse.h"
#include <arpa/inet.h>
#include <atomic>
#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <netinet/in.h>
#include <sys/types.h>

class HttpConn
{
  public:
    HttpConn ();
    ~HttpConn ();

  public:
    void init (int sockFd, const sockaddr_in &addr);

    ssize_t read (int *saveErrno);
    ssize_t write (int *saveErrno);

    bool process ();
    void Close ();

    int         GetFd () const;
    int         GetPort () const;
    const char *GetIP () const;
    sockaddr_in GetAddr () const;

    int
    ToWriteBytes ()
    {
        return iov_[0].iov_len + iov_[1].iov_len;
    }
    bool
    IsKeepAlive () const
    {
        return request_.IsKeepAlive ();
    }


  public:
    /* see in .cpp head init      */
    static std::atomic<int>
                       userCount;
    static const char *srcDir;
    static bool        isET;
    /********************************/

  private:
    bool isClose_;

    int                fd_;
    struct sockaddr_in addr_;

    int          iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest  request_;
    HttpResponse response_;
};

#endif