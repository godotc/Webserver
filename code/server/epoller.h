
#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <vector>

class Epoller
{
  public:
    explicit Epoller (int maxEvent = 1024);
    ~Epoller ();

  private:
    int epollFd_;

    std::vector<struct epoll_event> events_;
};


#endif