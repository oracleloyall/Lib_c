
#include "tinylib/linux/net/buffer.h"
#include "tinylib/util/log.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/uio.h>
#include <errno.h>

struct buffer
{
    unsigned char* data;
    unsigned len;

    unsigned read_index;
    unsigned write_index;
};

static 
void ensure_space(buffer_t* buffer, unsigned size)
{
    unsigned expand;
    unsigned char* data;
    
    assert(NULL != buffer);

    /* buffer尾部的空闲空间已经足够 */
    if ((buffer->len - buffer->write_index) >= size)
    {
        return;
    }

    if ((buffer->len - (buffer->write_index - buffer->read_index)) > size)
    {
        /* buffer中整体的空间足够，但需要做一下整理 ，
         * 将数据全部移到首部，空闲空间合并
         */
        memmove(buffer->data, (buffer->data+buffer->read_index), (buffer->write_index - buffer->read_index));
        buffer->write_index -= buffer->read_index;
        buffer->read_index = 0;
    }
    else
    {
        /* buffe中整体的空闲空间也不够，需要扩容 */
        expand = ((size+1023)>>10)<<10;
        data = (unsigned char*)realloc(buffer->data, (buffer->len + expand));
        /* 若此处空间分配若失败，则无力回天了! */
        assert(NULL != data);
        buffer->data = data;
        buffer->len += expand;
    }

    return;
}

buffer_t* buffer_new(unsigned size)
{
    buffer_t* buffer;

    if (size == 0)
    {
        log_error("buffer_new: bad size");
        return NULL;
    }

    buffer = (buffer_t*)malloc(sizeof(buffer_t));
    memset(buffer, 0, sizeof(buffer_t));

    buffer->data = (unsigned char*)malloc(size);
    memset(buffer->data, 0, size);
    buffer->len = size;
    buffer->read_index = 0;
    buffer->write_index = 0;

    return buffer;
}

void buffer_destory(buffer_t* buffer)
{
    if (NULL != buffer)
    {
        free(buffer->data);
        free(buffer);
    }

    return;
}

void* buffer_peek(buffer_t* buffer)
{
    if (NULL == buffer || NULL == buffer->data || 0 == buffer->len)
    {
        log_error("buffer_peek: bad buffer");
        return NULL;
    }

    return (buffer->data + buffer->read_index);
}

unsigned buffer_readablebytes(buffer_t* buffer)
{
    if (NULL == buffer)
    {
        return 0;
    }

    return (buffer->write_index - buffer->read_index);
}

unsigned buffer_append(buffer_t* buffer, const void* data, unsigned size)
{
    if (NULL == buffer || NULL == data || 0 == size)
    {
        log_error("buffer_append: bad buffer(%p) or bad data(%p) or bad size(%u)", buffer, data, size);
        return 0;
    }

    ensure_space(buffer, size);

    memcpy((buffer->data + buffer->write_index), data, size);
    buffer->write_index += size;

    return 0;
}

unsigned buffer_readFd(buffer_t* buffer, int fd)
{
    char extra[4096];
    unsigned extra_bytes;
    struct iovec vecs[2];
    int n;

    if (NULL == buffer || fd < 0)
    {
        log_error("buffer_readFd: bad buffer(%p) or bad fd(%d)", buffer, fd);
        return 0;
    }

    memset(vecs, 0, sizeof(vecs));
    vecs[0].iov_base = buffer->data + buffer->write_index;
    vecs[0].iov_len = buffer->len - buffer->write_index;
    vecs[1].iov_base = extra;
    vecs[1].iov_len = sizeof(extra);

    n = readv(fd, vecs, 2);
    if (n < 0)
    {
        log_error("buffer_readFd: readv() failed, fd(%d), errno(%d)", fd, errno);
        return 0;
    }
    else if (n <= (buffer->len - buffer->write_index))
    {
        buffer->write_index += n;
    }
    else
    {
        extra_bytes = n - (buffer->len - buffer->write_index);
        buffer->write_index = buffer->len;
        buffer_append(buffer, extra, extra_bytes);
    }

    return n;
}

void buffer_retrieve(buffer_t *buffer, unsigned size)
{
    unsigned readablebytes = buffer->write_index - buffer->read_index;

    assert(size <= readablebytes);
    
    if (size < readablebytes)
    {
        buffer->read_index += size;
    }
    else
    {
        buffer->read_index = 0;
        buffer->write_index = 0;
    }

    return;
}

void buffer_retrieveall(buffer_t *buffer)
{
    if (NULL != buffer)
    {
        buffer->read_index = 0;
        buffer->write_index = 0;
    }
    
    return;
}

