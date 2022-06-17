/*-------------------------------------------------------------------------
 *  a Block Queue implemented by loop array
 *       m_back = (m_back+1) % m_max_size;
 *  Safe of Thread:
 *       every operation need to be lock at first then unlock after it done.
 *--------------------------------------------------------------------------*/

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include "../lock/lock.h"
#include <stdlib.h>
#include <sys/time.h>

template<class T> class block_queue
{
private:
    locker m_mutex;
    cond   m_cond;

    T*  m_array;
    int m_size;
    int m_max_size;
    int m_front;
    int m_back;

public:
    block_queue(int max_size = 1000)
    {
        if (max_size <= 0) {
            exit(-1);
        }

        m_max_size = max_size;
        m_array    = new T[max_size];
        m_size     = 0;
        m_front    = -1;
        m_back     = -1;
    }
    ~block_queue()
    {
        m_mutex.lock();
        if (m_array != nullptr) {
            delete[] m_array;
            m_array = nullptr;
        }
        m_mutex.unlock();
    }

    void clear()
    {
        m_mutex.lock();
        m_size  = 0;
        m_front = -1;
        m_back  = -1;
        m_mutex.unlock();
    }

    bool full()
    {
        m_mutex.lock();
        if (m_size >= m_max_size) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool empty()
    {
        m_mutex.lock();

        if (0 == m_size) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool front(T& val)
    {
        m_mutex.lock();
        if (0 == m_size) {
            m_mutex.unlock();
            return false;
        }
        // change the reference/pointer of value
        val = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    bool back(T& val)
    {
        m_mutex.lock();
        if (0 == m_size) {
            m_mutex.unlock();
            return false;
        }
        val = m_array[m_back];
        m_mutex.unlock();
        return true;
    }

    int size()
    {
        int temp = 0;

        m_mutex.lock();
        temp = m_size;
        m_mutex.unlock();

        return temp;
    }

    int max_size()
    {
        int temp;
        m_mutex.lock();
        temp = m_max_size;
        m_mutex.unlock();
        return temp;
    }

    // Add element to queue, firstly need to wake up all thread which use this queue.
    // When push an element, simulate to producer that produce a merchandise.
    // If there is no thread who watting for conditon variable, non-sense of wake.
    bool push(const T& item)
    {
        m_mutex.lock();
        if (m_size >= m_max_size) {
            m_cond.boardcast();
            m_mutex.unlock();
            return false;
        }

        m_back          = (m_back + 1) % m_max_size;
        m_array[m_back] = item;

        m_size++;

        m_cond.boardcast();
        m_mutex.unlock();

        return true;
    }

    // If no element in cur queue, will wait for conditong varable
    // Get the head item while popping
    bool pop(T& item)
    {
        m_mutex.lock();

        // why use while loop ? no if else
        while (m_size <= 0) {
            if (!m_cond.wait(m_mutex.get())) {
                m_mutex.unlock();
                return false;
            }
        }

        m_front = (m_front + 1) % m_max_size;
        item    = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

    // add the handle of timeout wait pop
    bool pop(T& item, int ms_timeout)
    {
        struct timespec t   = {0, 0};
        struct timeval  now = {0, 0};
        gettimeofday(&now, nullptr);

        m_mutex.lock();
        if (m_size <= 0) {
            t.tv_sec  = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!m_cond.timewait(m_mutex.get(), t)) {
                m_mutex.unlock();
                return false;
            }
        }

        if (m_size <= 0) {
            m_mutex.unlock();
            return false;
        }

        m_front = (m_front + 1) % m_max_size;
        item    = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }
};

#endif
