
#include "buffer.h"
#include <algorithm>
#include <bits/types/struct_iovec.h>
#include <cassert>
#include <stddef.h>
#include <string.h>
#include <string>
#include <strings.h>
#include <sys/types.h>
#include <sys/uio.h>

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

// move read pos by len
void
Buffer::Retrieve (size_t len)
{
    assert (len <= ReadableBytes ());
    readPos_ += len;
}
// move read pos to end
void
Buffer::RetrieveUntil (const char *end)
{
    assert (Peek () <= end);
    Retrieve (end - Peek ());
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


ssize_t
Buffer::ReadFd (int fd, int *saveErrno)
{
    char         buff[65535];
    struct iovec iov[2];
    const size_t writeable = WritableBytes ();

    // Distributively read, ensure completely read out;
    iov[0].iov_base = BeginPtr_ () + writePos_;
    iov[0].iov_len  = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len  = sizeof (buff);

    const ssize_t len = readv (fd, iov, 2);
    if (len < 0) {
        *saveErrno = errno;
    }
    else if (static_cast<size_t> (len) < writeable) {
        writePos_ += len;
    }
    else {
        // over buffer_ size, append local buff to buffer_, extend buffer_ size;
        writePos_ = buffer_.size ();
        Append (buff, len - writeable);
    }
    return len;
}