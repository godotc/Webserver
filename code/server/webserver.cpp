#include "webserver.h"
#include "epoller.h"
#include <cassert>
#include <cstring>
#include <unistd.h>


WebServer::WebServer (int port, int trigMode, int timeoutMS,
                      bool Optlinger,
                      int sqlPort, const char *sqlUser, const char *sqlPawd, const char *dbName,
                      int connPoolNum, int threadNum,
                      int openLog, int logLevel, int logQueSize)
    : port_ (port), openLinger_ (Optlinger), timeoutMS_ (timeoutMS), isClose_ (false),
      timer_ (new HeapTimer), threadpool_ (new ThreadPool (threadNum)),
      epoller_ (new Epoller)
{
    srcDir_ = getcwd (nullptr, 256);
    assert (srcDir_);
    strncat (srcDir_, "/resoures/", 16);
}