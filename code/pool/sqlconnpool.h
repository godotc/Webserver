#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mutex>
#include <mysql/mysql.h>
#include <queue>
#include <semaphore.h>

class SqlConnPool
{
  private:
    SqlConnPool ();
    ~SqlConnPool ();

  public:
    static SqlConnPool *Instance ();

    MYSQL *GetConn ();

    void Init (const char *host, int port,
               const char *user, const char *passwd,
               const char *dbName, int connsize);
    void ClosePool ();

  private:
    int MAX_CONN_;
    int userCount_;
    int freeCount_;

    std::queue<MYSQL *> connQue_;
    std::mutex          mtx_;
    sem_t               semId_;
};


#endif