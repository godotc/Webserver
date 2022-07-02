#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <algorithm>
#include <assert.h>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class ThreadPool
{
  private:
    struct Pool
    {
        std::mutex                         mtx;
        std::condition_variable            cond;
        bool                               isClosed;
        std::queue<std::function<void ()>> tasks;
    };
    std::shared_ptr<Pool> pool_;

  public:
    ThreadPool () = default;
    ThreadPool (ThreadPool &&) = default;

    explicit ThreadPool (size_t threadCount = 8) /* explicit:no allowed implicate convertion for an element in class */
        : pool_ (std::make_shared<Pool> ())
    {
        assert (threadCount > 0);

        for (size_t i = 0; i < threadCount; ++i)
        {
            std::thread (
                [pool = pool_]
                {
                    std::unique_lock<std::mutex> locker (pool->mtx);
                    while (true)
                    {
                        if (!pool->tasks.empty ())
                        {
                            auto task = std::move (pool->tasks.front ());
                            pool->tasks.pop ();
                            locker.unlock ();
                            task ();
                            locker.lock ();
                        }
                        else if (pool->isClosed)
                            break;
                        else
                            pool->cond.wait (locker);
                    }
                })
                .detach ();
        }
    }

    ~ThreadPool ()
    {
        if (static_cast<bool> (pool_))
        {
            std::lock_guard<std::mutex> locker (pool_->mtx);
            pool_->isClosed = true;
        }
        pool_->cond.notify_all ();
    }

  public:

    template <class F>
    void
    AddTask (F &&task)
    {
        {
            std::lock_guard<std::mutex> locker (pool_->mtx);
            pool_->tasks.emplace (std::forward<F> (task));
        }
        pool_->cond.notify_all ();
    }
};

#endif