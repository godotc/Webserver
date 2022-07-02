#include "log.h"

#include <bits/types/struct_timeval.h>
#include <bits/types/timer_t.h>
#include <string>
#include <sys/time.h>

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>

Log::Log ()
{
    lineCount_   = 0;
    isAsync_     = false;
    writeThread_ = nullptr;
    deque_       = nullptr;
    toDay_       = 0;
    fp_          = nullptr;
}
Log::~Log ()
{
    if (writeThread_ && writeThread_->joinable ())
    {
        while (!deque_->empty ())
        {
            deque_->flush ();
        }
        deque_->Close ();
        writeThread_->join ();
    }
    if (fp_)
    {
        std::lock_guard<std::mutex> locker (mtx_);
        flush ();
        fclose (fp_);
    }
}


void
Log::write (int level, const char *format, ...)
{
    struct timeval now = { 0, 0 };
    gettimeofday (&now, nullptr);

    time_t     tSec    = now.tv_sec;
    struct tm *sysTime = localtime (&tSec);
    struct tm  t       = *sysTime;
    va_list    vaList;

    // Log date & line, IF not today, change toDay_ then open new Log file
    if (this->toDay_ != t.tm_mday || (this->lineCount_ && (lineCount_ % MAX_LINES == 0)))
    {
        std::unique_lock<std::mutex> locker (mtx_);
        locker.unlock ();

        char newFile[LOG_NAME_LEN];
        char tail[36] = { 0 };
        snprintf (tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (toDay_ != t.tm_mday)
        {
            snprintf (newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_     = t.tm_mday;
            lineCount_ = 0;
        }
        else
        {
            snprintf (newFile, LOG_NAME_LEN, "%s/%s-%d%s", path_, tail, (lineCount_ / MAX_LINES), suffix_);
        }

        locker.lock ();
        flush ();
        fclose (fp_);
        fp_ = fopen (newFile, "a");
        assert (fp_ != nullptr);
    }

    // Write
    {
        // Date + Warn Level + Log(format)
        std::unique_lock<std::mutex> locker (mtx_);
        lineCount_++;
        int n = snprintf (buff_.BeginWrite (), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld",
                          t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);

        buff_.HasWritten (n);
        AppendLogLevelTitle_ (level);

        va_start (vaList, format);
        int m = vsnprintf (buff_.BeginWrite (), buff_.WritableBytes (), format, vaList);
        va_end (vaList);

        buff_.HasWritten (m);
        buff_.Append ("\n\0", 2);

        if (isAsync_ && deque_ && !deque_->full ())
        {
            deque_->push_back (buff_.RetrieveAllToStr ());
        }
        else
        {
            fputs (buff_.Peek (), fp_);
        }
        buff_.RetrieveAll ();
    }
}

void
Log::AppendLogLevelTitle_ (int level)
{
    switch (level)
    {
    case 0:
        buff_.Append ("[debug]:", 9);
        break;
    case 1:
        buff_.Append ("[info]:", 9);
        break;
    case 2:
        buff_.Append ("[warn]:", 9);
        break;
    case 3:
        buff_.Append ("[error]:", 9);
        break;
    default:
        buff_.Append ("[info]:", 9);
        break;
    }
}

int
Log::GetLevel ()
{
    std::lock_guard<std::mutex> locker (mtx_);
    return level_;
}
void
Log::SetLevel (int level)
{
    std::lock_guard<std::mutex> locker (mtx_);
    level_ = level;
}

void
Log::flush ()
{
    if (isAsync_)
        deque_->flush ();
    fflush (fp_);
}

Log *
Log::Instance ()
{
    static Log inst;
    return &inst;
}

bool
Log::IsOpen ()
{
    return isOpen_;
}