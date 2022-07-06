#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "../http/httpconn.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../timer/timer.h"
#include "epoller.h"
#include <memory>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_map>

class WebServer
{
  public:
    WebServer (
        int port, int trigMode, int timeoutMS, bool Optlinger,
        int sqlPort, const char *sqlUser, const char *sqlPawd, const char *dbName,
        int connPoolNum, int threadNum, int openLog, int logLevel, int logQueSize);

    ~WebServer ();
    void Start ();

  private:
    bool InitSocket_ ();
    void InitEventMode_ (int trigMode);
    void AddClient_ (int fd, sockaddr_in addr);

    void DealListen_ ();
    void DealWrite_ (HttpConn *client);
    void DealRead_ (HttpConn *client);

    void ExtentTime_ (HttpConn *client);
    void SendError_ (int fd, const char *info);
    void CloseConn_ (HttpConn *client);

    void OnRead_ (HttpConn *client);
    void OnWrite_ (HttpConn *client);
    void OnProcess (HttpConn *client);

    static int SetFdNonBlock (int fd);

  private:
    static const int MAX_FD = 65536;

    int   port_;
    bool  openLinger_;
    int   timeoutMS_;
    bool  isClose_;
    int   listenFd_;
    char *srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer>        timer_;
    std::unique_ptr<ThreadPool>       threadpool_;
    std::unique_ptr<Epoller>          epoller_;
    std::unordered_map<int, HttpConn> users_;
};


#endif