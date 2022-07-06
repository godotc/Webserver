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
    int             id;
    TimeStamp       expires;
    TimeoutCallBack cb;
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
    void add (int id, int timeOut, const TimeoutCallBack &cb);

    void adjust (int id, int newExpires);

    void pop ();

    void tick (); // tick!! erase expire(timeout) node before this tick
    int  GetNextTick ();


  private:
    void del_ (size_t i);
    void siftup_ (size_t i);                 //上滤
    bool siftdown_ (size_t index, size_t n); // 下滤
    void SwapNode_ (size_t i, size_t j);

  private:
    std::vector<TimerNode>          heap_;
    std::unordered_map<int, size_t> ref_;
};

#endif