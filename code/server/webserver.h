#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "../http/httpconn.h"
#include "../pool/threadpool.h"
#include "../timer/timer.h"
#include "epoller.h"
#include <memory>
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
    void InitEventMode_ ();

  private:
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