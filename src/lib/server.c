
#include "../../include/server.h"
#include "../../include/server-configs.h"
#include "../../include/utils.h"
#include <stdio.h>
// #include <stdlib.h>
// #include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

int g_enable_socket_reuse = 0;

/**
@brief create a server to start with

@param server - address of server instance
@param address_family - address family to be used (AF_INET / AF_INET6)
@param socket_type - type of socket to be used (SOCK_STREAM , SOCK_DGRAM, etc)
@param protocol - Protocol to be used (if zero the OS will select the default one
based on you domain and service preference)
@param port - local port to listen for (set zero to get an ephemeral port)
@param backlog - socket backlog
@param interface - network interface to bind the socket
@param launch - a function handle the server instance after binding the server to
local network port
@returns the server creation status 0 for success other for error.
*/
int server_constructor(
    struct Server *server, int address_family, int socket_type, int protocol,
    int port, int backlog, unsigned long interface,
    int (*launch)(struct Server *)
) {
    server->address_family = address_family;
    server->socket_type = socket_type;
    server->protocol = protocol;
    server->port = port;
    server->backlog = backlog;
    server->interface = interface;

    server->address.sin_family = address_family;
    server->address.sin_port = htons(port);
    server->address.sin_addr.s_addr = htonl(interface);

    server->socket = socket(address_family, socket_type, protocol);
    if (server->socket <= 0) {
        g_logger.error("Failed to create socket : %d \n", server->socket);
        return 1;
    }
    if (g_enable_socket_reuse) {
        // enable socket reuse (to prevent socket not available issues on immediate reuse)
        g_logger.debug("enabling SO_REUSEADDR");
        const int _enable = 1;
        if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &_enable, sizeof(_enable)) < 0)
            g_logger.error("setsockopt(SO_REUSEADDR) failed");
    }

    int bind_resp = bind(server->socket, (struct sockaddr *)&server->address, sizeof(server->address));
    if (bind_resp < 0){
        g_logger.error("Failed to bind socket (%d) on port %d! error : %d\n", server->socket, port, bind_resp);
        return 2;
    };
    if (server->port == 0) {
        // Get the assigned port number
        int addr_len = sizeof(server->address);
        if (getsockname(server->socket, (struct sockaddr *)&server->address, (socklen_t *)&addr_len) < 0) {
            g_logger.error("failed to get the ephemeral port number assigned!");
            close(server->socket);
            return 1;
        }
        server->port = ntohs(server->address.sin_port);
        g_server_port = server->port;
    }

    // that listen part
    int listen_resp = listen(server->socket, server->backlog);
    if (listen_resp < 0) {
        char error_message[75];
        sprintf(error_message, "Error listening to address (port: %d)\nerror : %d\n", port, listen_resp);
        g_logger.error(error_message);
        return 3;
    }

    server->launch = launch;
    server->close = server_destructor;

    return 0;
}

/**
@brief garbage collection for the server
*/
void server_destructor(struct Server * server) {
    if (is_server_alive(server)) {
        shutdown(server->socket, SHUT_RD);
        close(server->socket);
    }
}

/**
@brief check if the server is alive or not
@param server - the server pointer
@returns 1 if server is alive else returns 0;
*/
int is_server_alive(struct Server * server) {
    if (server == NULL) return  0;
    if (server->socket <= 0) {
        // not a good socket number (considering the server to be dead)
        return 0;
    }
    char buffer[1];
    errno = 0;
    int resp = recv(server->socket, buffer, sizeof(buffer), MSG_PEEK);
    if (resp == 0) return 0;
    else if (resp <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOTCONN) {
            // No data available | the socket is not connected rn, but the socket is still open
            return 1;
        }
        return  0;
    }
    return 1;
}
