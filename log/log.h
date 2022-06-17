#ifndef LOG_H
#define LOG_H

#include "block_queue.h"
#include <string>

class Log
{
  private:
    Log();
    virtual ~Log();

    void *async_write_log()
    {
        std::string singel_log;
        // pop one log string from block_queue, write to file
        while (m_log_queue->pop(singel_log))
        {
            m_mutex.lock();
            fputs(singel_log.c_str(), m_fp);
            m_mutex.unlock();
        }
        return nullptr;
    }

  private:
    block_queue<std::string> *m_log_queue; // block queue
    locker m_mutex;

    FILE *m_fp;         // the ptr of log file
    char dir_name[128]; // dir name of log
    char log_name[128]; // file name of log

    char *m_buf;
    int m_log_buf_size;
    int m_today;
    int m_split_lines; // the max lines of log;
    long long m_count; // the lines counter

    bool m_is_async; // switch off/on of async flag bit
    int m_close_log; // switch of/on log

  public:
    // After C++11, no need to lock in lazy mode while using local varable
    static Log *get_singleton()
    {
        static Log instance;
        return &instance;
    }

    static void *flush_log_thread(void *args)
    {
        Log::get_singleton()->async_write_log();
        return nullptr;
    }

    // Modifiable: log file, log buf size, max lines, max number of log queue
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000,
              int max_queue_size = 0);

    void write_log(int level, const char *format, ...);

    void flush(void);
};

#define LOG_DEBUG(format, ...)                                                                                         \
    if (0 == m_close_log)                                                                                              \
    {                                                                                                                  \
        Log::get_singleton()->write_log(0, format, ##__VA_ARGS__);                                                     \
        Log::get_singleton()->flush();                                                                                 \
    }
#define LOG_INFO(format, ...)                                                                                          \
    if (0 == m_close_log)                                                                                              \
    {                                                                                                                  \
        Log::get_singleton()->write_log(1, format, ##__VA_ARGS__);                                                     \
        Log::get_singleton()->flush();                                                                                 \
    }
#define LOG_WARN(format, ...)                                                                                          \
    if (0 == m_close_log)                                                                                              \
    {                                                                                                                  \
        Log::get_singleton()->write_log(2, format, ##__VA_ARGS__);                                                     \
        Log::get_singleton()->flush();                                                                                 \
    }
#define LOG_ERROR(format, ...)                                                                                         \
    if (0 == m_close_log)                                                                                              \
    {                                                                                                                  \
        Log::get_singleton()->write_log(3, format, ##__VA_ARGS__);                                                     \
        Log::get_singleton()->flush();                                                                                 \
    }

#endif
