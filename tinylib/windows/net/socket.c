
#include "tinylib/windows/net/socket.h"
#include "tinylib/util/log.h"

#include <string.h>

SOCKET create_server_socket(unsigned short port, const char* ip)
{
    SOCKET fd;
    struct sockaddr_in addr;
    BOOL flag = TRUE;
    unsigned long value = 1;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == fd)
    {
        log_error("create_server_socket: socket() failed, errno: %d, addr: %s:%u", WSAGetLastError(), ip, port);
        return INVALID_SOCKET;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
    ioctlsocket(fd, FIONBIO, &value);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = (NULL != ip ? inet_addr(ip) : INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        log_error("create_server_socket: bind() failed, errno: %d, addr: %s:%u", WSAGetLastError(), (NULL==ip?"0.0.0.0":ip), port);
        closesocket(fd);
        return INVALID_SOCKET;
    }

    return fd;
}

SOCKET create_client_socket(void)
{
    SOCKET fd;
    unsigned long value = 1;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == fd)
    {
        log_error("create_client_socket: socket() failed, errno: %d", WSAGetLastError());
        return INVALID_SOCKET;
    }

    ioctlsocket(fd, FIONBIO, (unsigned long*)&value);

    return fd;
}

void set_socket_reuseaddr(SOCKET fd, int on)
{
    int value = on ? 1 :0;
    if (INVALID_SOCKET == fd)
    {
        log_error("set_socket_reuseaddr: bad socket handle");
        return;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value)) == SOCKET_ERROR)
    {
        log_error("set_socket_reuseaddr: setsockopt() failed, errno: %d", WSAGetLastError());
    }

    return;
}

void set_socket_onblock(SOCKET fd, int on)
{
    unsigned long value = on ? 1 : 0;
    if (INVALID_SOCKET == fd)
    {
        log_error("set_socket_onblock: bad socket handle");
        return;
    }

    ioctlsocket(fd, FIONBIO, (unsigned long*)&value);

    return;
}

void set_socket_nodelay(SOCKET fd, int on)
{
    BOOL value = on ? TRUE : FALSE;

    if (INVALID_SOCKET == fd)
    {
        log_error("set_socket_nodelay: bad socket handle");
        return;
    }

    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value));

    return;
}

SOCKET create_udp_socket(unsigned short port, const char *ip)
{
    SOCKET fd;
    struct sockaddr_in addr;
    unsigned long value = 1;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (INVALID_SOCKET == fd)
    {
        log_error("create_udp_socket: socket() failed, errno: %d", WSAGetLastError());
        return INVALID_SOCKET;
    }

    ioctlsocket(fd, FIONBIO, &value);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = (NULL != ip ? inet_addr(ip) : INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        log_error("create_udp_socket: bind() failed, erron: %d, addr: %s:%u", WSAGetLastError(), (NULL==ip?"0.0.0.0":ip), port);
        closesocket(fd);
        return INVALID_SOCKET;
    }

    return fd;
}

int socketpair(SOCKET fds[2])
{
    SOCKET fd_s;
    struct sockaddr_in addr;
    struct sockaddr_in c_addr;
    int addr_len;
    
    BOOL flag;
    unsigned long value;
    
    unsigned short port_begin = 65535;

    SOCKET fd_client;
    SOCKET fd_accept;

    do
    {
        fd_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        
        flag = TRUE;
        setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = 0x100007f;
        addr.sin_port = htons(port_begin);
        if (bind(fd_s, (struct sockaddr*)&addr, sizeof(addr)))
        {
            closesocket(fd_s);

            port_begin--;
            if (port_begin < 2)
            {
                return -1;
            }

            continue;
        }

        listen(fd_s, 1);

        fd_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        flag = TRUE;
        setsockopt(fd_client, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

        memset(&c_addr, 0, sizeof(c_addr));
        c_addr.sin_family = AF_INET;
        c_addr.sin_addr.s_addr = 0x100007f;
        c_addr.sin_port = htons(port_begin-1);
        if (bind(fd_client, (struct sockaddr*)&c_addr, sizeof(c_addr)))
        {
            closesocket(fd_s);
            closesocket(fd_client);

            port_begin -= 2;
            if (port_begin < 2)
            {
                return -1;
            }

            continue;
        }

        addr_len = (int)sizeof(addr);
        getsockname(fd_s, (struct sockaddr*)&addr, &addr_len);
        if (connect(fd_client, (struct sockaddr*)&addr, addr_len) != 0)
        {
            closesocket(fd_s);
            closesocket(fd_client);

            port_begin -= 2;
            if (port_begin < 2)
            {
                return -1;
            }

            continue;
        }

        fd_accept = accept(fd_s, (struct sockaddr*)&addr, &addr_len);
        if (fd_accept == INVALID_SOCKET)
        {
            closesocket(fd_s);
            closesocket(fd_client);

            port_begin -= 2;
            if (port_begin < 2)
            {
                return -1;
            }

            continue;
        }

        closesocket(fd_s);

        break;
    } while(1);
    
    value = 1;
    ioctlsocket(fd_client, FIONBIO, &value);
    value = 1;
    ioctlsocket(fd_accept, FIONBIO, &value);

    fds[0] = fd_client;
    fds[1] = fd_accept;

    return 0;
}
