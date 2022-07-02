#ifndef BUFFER_H
#define BUFFER_H

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

class Buffer
{
  public:
    Buffer (int initBuffSize = 1024);
    ~Buffer () = default;

  public:
    size_t WritableBytes () const;
    size_t ReadableBytes () const;
    size_t PrependableBytes () const;

    const char *Peek () const;
    void        EnsureWriteable (size_t len);
    void        HasWritten (size_t len);

    // void Retrieve (size_t len);
    // void RetrieveUntil (const char *end);

    void        RetrieveAll ();
    std::string RetrieveAllToStr ();

    const char *BeginWriteConst () const;
    char       *BeginWrite ();

    void Append (const std::string &str);
    void Append (const char *str, size_t len);
    void Append (const void *data, size_t len);
    void Append (const Buffer &buf);

  private:
    char       *BeginPtr_ ();
    const char *BeginPtr_ () const;
    void        MakeSpace_ (size_t len);

  private:
    std::vector<char>        buffer_;
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};

#endif