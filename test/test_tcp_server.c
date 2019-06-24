
#ifdef WINNT
    #include "tinylib/windows/net/tcp_server.h"
    #include <winsock2.h>
#elif defined(__linux__)
    #include "tinylib/linux/net/tcp_server.h"
#endif

#include "tinylib/util/log.h"

#include <stdio.h>
#include <assert.h>

int g_run = 10;
static loop_t *g_loop = NULL;

static 
void on_data(tcp_connection_t* connection, buffer_t* buffer, void* userdata)
{
    const inetaddr_t* addr = tcp_connection_getpeeraddr(connection);
    log_info("%u bytes recevied from %s:%u\n", buffer_readablebytes(buffer), addr->ip, addr->port);
	
    tcp_connection_send(connection, buffer_peek(buffer), buffer_readablebytes(buffer));
    buffer_retrieveall(buffer);

    return;
}

static 
void on_close(tcp_connection_t* connection, void* userdata)
{
    const inetaddr_t* addr = tcp_connection_getpeeraddr(connection);
    log_info("connectionto %s:%u will be closed\n", addr->ip, addr->port);

    tcp_connection_destroy(connection);

    g_run--;
    if (0 == g_run)
    {
        loop_quit(g_loop);
    }

    return;
}

static 
void on_conn(tcp_connection_t* connection, void* userdata, const inetaddr_t* addr)
{
    log_info("new connection from %s:%u\n", addr->ip, addr->port);
    tcp_connection_setcalback(connection, on_data, on_close, NULL);

    return;
}

int main(int argc, char *argv[])
{
    tcp_server_t *server;
    const char *ip;

    #ifdef WINNT
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
    #endif    

    g_loop = loop_new(1);
    assert(g_loop);

    ip = "0.0.0.0";
    if (argc > 2)
    {
        ip = argv[1];
    }
    server = tcp_server_new(g_loop, on_conn, NULL, 16889, ip);
    assert(server);
    tcp_server_start(server);

    loop_loop(g_loop);

    tcp_server_stop(server);    
    tcp_server_destroy(server);
    loop_destroy(g_loop);

    #ifdef WINNT
    WSACleanup();
    #endif    

    return 0;
}
