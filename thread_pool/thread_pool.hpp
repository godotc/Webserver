#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "../http_conn/http_conn.h"
#include "../lock/lock.h"
#include "../mysql_conn/sql_connection_pool.h"
#include <list>
#include <pthread.h>

template <class T> class threadpool
{
  private:
    int m_thread_numbers = 8; // number of threads in pool
    int m_max_requests;       // max requests that request queue aviliable
    pthread_t *m_threads; // the array represent pools, Size: m_thread_number
    std::list<T *> m_workqueue; // requset queue(list?)
    locker m_queuelocker;       // queue's mutex locker
    semaphore m_queuestat;      // semaphore number for work
    sql_conn_pool *m_connPool;  // DB_conns
    int m_actor_model;          // swtich reactor/proactor model

  private:
    // the function that work thread run, It constantly fetch work from work
    // queue then deal
    static void *worker (void *arg);
    void run ();

  public:
    // thread_number is the num of thread in pool
    threadpool (int actor_model, sql_conn_pool *connPool, int thread_numbers,
                int max_request = 10000);
    ~threadpool ();

    bool append (T *request, int state);
    bool append_p (T *request);
};

template <typename T>
void
threadpool<T>::run ()
{
    while (1)
        {
            m_queuestat.wait ();
            m_queuelocker.lock ();
            if (m_workqueue.empty ())
                {
                    m_queuelocker.unlock ();
                    continue;
                }

            T *request = m_workqueue.front ();
            m_workqueue.pop_front ();
            m_queuelocker.unlock ();
            if (!request)
                continue;

            if (1 == m_actor_model)
                {
                    if (0 == request->m_rw_state)
                        {
                            if (request->read_once ())
                                {
                                    request->improv = 1;
                                    sql_conn_RAII mysqlcon (&request->mysql,
                                                            m_connPool);
                                    request->process ();
                                }
                            else
                                {
                                    request->improv = 1;
                                    request->timer_flag = 1;
                                }
                        }
                    else
                        {
                            if (request->write ())
                                request->improv = 1;
                            else
                                {
                                    request->improv = 1;
                                    request->timer_flag = 1;
                                }
                        }
                }
            else
                {
                    sql_conn_RAII mysqlconn (&request->mysql, m_connPool);
                    request->process ();
                }
        }
}
template <typename T>
void *
threadpool<T>::worker (void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run ();
    return pool;
}

template <typename T>
threadpool<T>::threadpool (int actor_model, sql_conn_pool *connPool,
                           int thread_number, int max_request)
    : m_actor_model (actor_model), m_max_requests (max_request),
      m_threads (nullptr), m_connPool (connPool)
{
    // m_thread_numbers = thread_number;

    if (m_thread_numbers <= 0 || max_request <= 0)
        throw std::exception ();

    m_threads = new pthread_t[m_thread_numbers];
    if (!m_threads)
        throw std::exception ();
    for (int i = 0; i < m_thread_numbers; ++i)
        {
            if (pthread_create (m_threads + i, nullptr, worker, this) != 0)
                {
                    delete[] m_threads;
                    throw std::exception ();
                }
            if (pthread_detach (m_threads[i]))
                {
                    delete[] m_threads;
                    throw std::exception ();
                }
        }
}
template <typename T> threadpool<T>::~threadpool () { delete[] m_threads; }

template <typename T>
bool
threadpool<T>::append (T *request, int state)
{
    m_queuelocker.lock ();
    if (m_workqueue.size () >= m_max_requests)
        {
            m_queuelocker.unlock ();
            return false;
        }

    request->m_rw_state = state;
    m_workqueue.push_back (request);
    m_queuelocker.unlock ();
    m_queuestat.post ();
    return true;
}
template <typename T>
bool
threadpool<T>::append_p (T *request)
{
    m_queuelocker.lock ();
    if (m_workqueue.size () >= m_max_requests)
        {
            m_queuelocker.unlock ();
            return false;
        }

    m_workqueue.push_back (request);
    m_queuelocker.unlock ();
    m_queuestat.post ();
    return true;
}

#endif