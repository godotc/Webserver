#ifndef WEBSERVER_H
#define WEBSERVER_H

#include"mysql_conn/sql_connection_pool.h"
#include"http_conn/http_conn.h"

const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 5;     // min timeout units


class Webserver
{
private:
    // Fundations
    int m_port;
    char* m_root;
    int m_log_write;
    int m_close_log;
    int m_actormodel;

    int m_pipefd[2];
    int m_epollfd;
    http_conn* users;

    // DBs

    // Timers
    client_data* users_timer;

public:
    Webserver();
    ~Webserver();


};

#endif
