#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "../log/log.h"
#include "httprequest.h"
#include "httpresponse.h"
#include <arpa/inet.h>
#include <atomic>
#include <netinet/in.h>

class HttpConn
{
  public:
    HttpConn ();
    ~HttpConn ();

  public:
    void Close ();

    int         GetFd () const;
    int         GetPort () const;
    const char *GetIP () const;
    sockaddr_in GetAddr () const;

  public:
    // see in .cpp head init
    static std::atomic<int> userCount;
    static const char      *srcDir;
    static bool             isET;

  private:
    int                fd_;
    struct sockaddr_in addr_;

    bool isClose_;

    HttpRequest  request_;
    HttpResponse response_;
};

#endif