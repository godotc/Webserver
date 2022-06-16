#ifndef WEBSERVER_H
#define WEBSERVER_H

class Webserver
{
private:
    int m_port;
    char* m_root;
    int m_log_write;
    int m_close_log;
    int m_actormodel;

    int m_pipefd[2];
    int m_epollfd;
    

public:
    Webserver ();
    ~Webserver ();
};

#endif
