#ifndef LIST_TIMER
#define LIST_TIMER

#include "../http_conn/http_conn.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <time.h>

class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    struct util_timer *timer;
};

class util_timer
{
  public:
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
    void (*cb_func) (client_data *);

    time_t expire;

  public:
    util_timer () : prev (nullptr), next (nullptr) {}
};

class sort_timer_list
{
  private:
    util_timer *head;
    util_timer *tail;
    void add_timer (util_timer *timer, util_timer *list_head);

  public:
    sort_timer_list () : head (nullptr), tail (nullptr) {}
    ~sort_timer_list ();

    void add_timer (util_timer *timer);
    void adjust_tiemr (util_timer *timer);
    void del_timer (util_timer *timer);
    void tick ();
};

void cb_func (client_data *ueserdata);

class Utils
{
  public:
    Utils () {}
    ~Utils () {}

  public:
    static int *u_pipefd;
    sort_timer_list m_timer_list;

    static int u_epollfd;
    int m_TIMESLOT;

  public:
    void init (int timeslot);

    // set non-block for fd
    int setnonblocking (int fd);

    // registe read event in kernel event table, Mode: ET, EPOLLONESHOT:
    // true/false
    void addfd (int epollfd, int fd, bool one_shot, int TRIGMode);

    // signal handler
    static void sig_handler (int sig);

    // set sig function(callback?)
    void
    addsig (int sig, void (handler) (int), bool restart = true)
    {

        struct sigaction sa;
        memset (&sa, '\0', sizeof (sa));

        sa.sa_handler = handler;
        if (restart)
            {
                sa.sa_flags |= SA_RESTART;
            }
        sigfillset (&sa.sa_mask);
        assert (sigaction (sig, &sa, nullptr) != 1);
    }

    // regularly handle task, then retimer for trigger SIGALRM constantly
    void timer_handler ();

    void show_erroer (int connfd, const char *info);
};

void cb_func (client_data *ueserdata);

#endif