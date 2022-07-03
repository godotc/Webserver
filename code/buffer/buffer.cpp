
#include "buffer.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string.h>
#include <string>
#include <strings.h>

Buffer::Buffer (int initBuffSize)
    : buffer_ (initBuffSize), readPos_ (0), writePos_ (0)
{
}

// the len of has wrote - last read pos
size_t
Buffer::ReadableBytes () const
{
    return writePos_ - readPos_;
}
size_t
Buffer::WritableBytes () const
{
    return buffer_.size () - writePos_;
}
size_t
Buffer::PrependableBytes () const
{
    return readPos_;
}

// The read pos's ptr
const char *
Buffer::Peek () const
{
    return BeginPtr_ () + readPos_;
}

void
Buffer::EnsureWriteable (size_t len)
{
    if (WritableBytes () < len)
        MakeSpace_ (len);
    assert (WritableBytes () >= len);
}
void
Buffer::HasWritten (size_t len)
{
    writePos_ += len;
}

char *
Buffer::BeginPtr_ ()
{
    return &*buffer_.begin ();
}
const char *
Buffer::BeginPtr_ () const
{
    return &*buffer_.begin ();
}

void
Buffer::MakeSpace_ (size_t len)
{
    if (WritableBytes () + PrependableBytes () < len)
    {
        buffer_.resize (writePos_ + len + 1);
    }
    else
    {
        size_t readable = ReadableBytes ();
        std::copy (BeginPtr_ () + readPos_, BeginPtr_ () + writePos_, BeginPtr_ ());
        readPos_  = 0;
        writePos_ = readPos_ + readable;
        assert (readable == ReadableBytes ());
    }
}

const char *
Buffer::BeginWriteConst () const
{
    return BeginPtr_ () + writePos_;
}
char *
Buffer::BeginWrite ()
{
    return BeginPtr_ () + writePos_;
}


// Reset buff memory , read&write pos
void
Buffer::RetrieveAll ()
{
    bzero (&buffer_[0], buffer_.size ());
    readPos_  = 0;
    writePos_ = 0;
}
// Maintain data to a string, then reset buf memory, return string
std::string
Buffer::RetrieveAllToStr ()
{
    std::string str (Peek (), ReadableBytes ());
    RetrieveAll ();
    return str;
}


void
Buffer::Append (const char *str, size_t len)
{
    assert (str);
    EnsureWriteable (len);
    std::copy (str, str + len, BeginWrite ());
    HasWritten (len);
}
void
Buffer::Append (const std::string &str)
{
    Append (str.data (), str.length ());
}
void
Buffer::Append (const void *data, size_t len)
{
    assert (data);
    Append (static_cast<const char *> (data), len);
}
void
Buffer::Append (const Buffer &buf)
{
    // Beginpos + readpos -> Writepos - readpos
    Append (buf.Peek (), buf.ReadableBytes ());
}