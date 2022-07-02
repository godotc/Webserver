#include "httpresponse.h"
#include <cstddef>
#include <sys/mman.h>

HttpResponse::HttpResponse ()
{
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_    = false;
    mmFile_         = nullptr;
    mmFileStat_     = { 0 };
}
HttpResponse::~HttpResponse ()
{
    UnmapFile ();
}


void
HttpResponse::UnmapFile ()
{
    if (mmFile_)
    {
        munmap (mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}