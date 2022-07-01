#include "list_timer.h"
#include "../http_conn/http_conn.h"

// preinit static varable
int *Utils::u_pipefd = nullptr;
int Utils::u_epollfd = 0;

// class Utils;    // dont know

// callback function
void
cb_func (client_data *userdata)
{
    epoll_ctl (Utils::u_epollfd, EPOLL_CTL_DEL, userdata->sockfd, 0);
    assert (userdata);
    close (userdata->sockfd);
    http_conn::m_user_count--;
}

sort_timer_list::~sort_timer_list ()
{
    util_timer *temp = head;
    while (temp)
        {
            head = temp->next;
            delete temp;
            temp = head;
        }
    // head = nullptr;
}

void
sort_timer_list::add_timer (util_timer *timer, util_timer *list_head)
{
    util_timer *prev = list_head;
    util_timer *temp = prev->next;
    while (temp)
        {
            // insert
            if (timer->expire < temp->expire)
                {
                    prev->next = timer;
                    timer->next = temp;
                    temp->prev = timer;
                    timer->prev = prev;
                }
            prev = temp;
            temp = temp->next;
        }
    if (!temp)
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = nullptr;
            tail = timer;
        }
}

void
sort_timer_list::add_timer (util_timer *timer)
{
    if (!timer)
        return;
    if (!head)
        {
            head = tail = timer;
            return;
        }
    // add to head node's prev
    if (timer->expire < head->expire)
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
    add_timer (timer, head);
}
void
sort_timer_list::adjust_tiemr (util_timer *timer)
{
    if (!timer)
        return;

    util_timer *temp = timer->next;
    if (!temp || (timer->expire < temp->expire))
        return;
    if (timer == head)
        {
            head = head->next;
            head->prev = nullptr;
            timer->next = nullptr;
            add_timer (timer, head);
        }
    else
        {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer (timer, timer->next);
        }
}
void
sort_timer_list::del_timer (util_timer *timer)
{
    if (!timer)
        return;
    if ((timer == head) && (timer == tail))
        {
            delete timer;
            head = tail = nullptr;
            return;
        }
    if (timer == head)
        {
            head = head->next;
            head->prev = nullptr;
            delete timer;
            return;
        }
    if (timer == tail)
        {
            tail = tail->prev;
            tail->next = nullptr;
            delete timer;
            return;
        }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
void
sort_timer_list::tick ()
{
    if (!head)
        return;

    time_t cur = time (nullptr);
    util_timer *temp = head;
    while (temp)
        {
            if (cur < temp->expire)
                break;

            temp->cb_func (temp->user_data);
            head = temp->next;

            if (head)
                {
                    head->prev = nullptr;
                }
            delete temp;
            temp = head;
        }
}

void
Utils::init (int timeslot)
{
    m_TIMESLOT = timeslot;
}

int
Utils::setnonblocking (int fd)
{
    int old_opt = fcntl (fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl (fd, F_SETFL, new_opt);
    return old_opt;
}

// register read event, Mode： ET, Switch: EPOLLONESHOT
void
Utils::addfd (int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;

    epoll_ctl (epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking (fd);
}

void
Utils::sig_handler (int sig)
{
    // save origin error for reenterability of function
    int save_errno = errno;
    int msg = sig;
    send (u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

// set signal function(callback)
/* void
addsig (int sig, void (handler) (int), bool restart)
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
} */

// handle task regularly, then retime to trigger SIGALARM constantly
void
Utils::timer_handler ()
{
    m_timer_list.tick ();
    alarm (m_TIMESLOT);
}

void
Utils::show_erroer (int connfd, const char *info)
{
    send (connfd, info, strlen (info), 0);
    close (connfd);
}