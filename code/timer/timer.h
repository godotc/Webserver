#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

using TimeoutCallBack = std::function<void ()>;
using Clock           = std::chrono::high_resolution_clock; // 纳秒struct
using MS              = std::chrono::milliseconds;
using TimeStamp       = Clock::time_point;

struct TimerNode
{
    int       id;
    TimeStamp expires;

    bool
    operator<(const TimerNode &t)
    {
        return expires < t.expires;
    }
};

class HeapTimer
{
  public:
    HeapTimer () { heap_.reserve (64); }
    ~HeapTimer () { heap_.clear (); }

  public:
    void adjust (int id, int newExpires);

  private:
    void siftdown_ (size_t index, size_t n);

  private:
    std::vector<TimerNode>          heap_;
    std::unordered_map<int, size_t> ref_;
};

#endif