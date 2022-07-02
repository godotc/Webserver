#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <sys/stat.h>

class HttpResponse
{
  public:
    HttpResponse ();
    ~HttpResponse ();

  public:

    void UnmapFile ();

  private:
    int  code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char       *mmFile_;
    struct stat mmFileStat_;
};


#endif