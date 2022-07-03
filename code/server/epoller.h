#ifndef EPOLLER_H
#define EPOLLER_H

#include <cstddef>
#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>

class Epoller
{
  public:
    explicit Epoller (int maxEvent = 1024);
    ~Epoller ();

  public:
    bool AddFd (int fd, u_int32_t events);
    bool ModFd (int fd, u_int32_t events);
    bool DelFd (int fd);

    int Wait (int timeoutMs = -1);

    int      GetEventFd (size_t i) const;
    uint32_t GetEvents (size_t i) const;


  private:
    int epollFd_;
    
    std::vector<struct epoll_event> events_;

};


#endif