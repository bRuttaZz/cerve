
#include "../../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

// global definition
struct Server * g_running_server = NULL;

// internal functions
void _handle_sigint(int sig);
void _handle_sigterm(int sig);


/**
@brief create a server to start with

@param address_family - address family to be used (AF_INET / AF_INET6)
@param socket_type - type of socket to be used (SOCK_STREAM , SOCK_DGRAM, etc)
@param protocol - Protocol to be used (if zero the OS will select the default one
based on you domain and service preference)
@param port - local port to listen for (set zero to get an ephemeral port)
@param backlog - socket backlog
@param interface - network interface to bind the socket
@params global_server - if set to non-zero the created server will be referenced globally and
have proper garbage collection on SITERM and SIGINT. if zero the server will not be globally registered
@param launch - a function handle the server instance after binding the server to
local network port
@returns the created server instance.
*/
struct Server server_constructor(
    int address_family, int socket_type, int protocol,
    int port, int backlog, unsigned long interface,
    int global_server, void (*launch)(struct Server *)
) {
    if (global_server && is_server_alive(g_running_server)) {
        perror("Attempt to create new server, while one is already running..");
        exit(-1);
    }

    struct Server server;

    server.address_family = address_family;
    server.socket_type = socket_type;
    server.protocol = protocol;
    server.port = port;
    server.backlog = backlog;
    server.interface = interface;

    server.address.sin_family = address_family;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);

    server.socket = socket(address_family, socket_type, protocol);
    if (server.socket <= 0) {
        char error_message[50];
        sprintf(error_message, "Failed to create socket : %d \n", server.socket);
        perror(error_message);
        exit(1);
    }

    int bind_resp = bind(server.socket, (struct sockaddr *)&server.address, sizeof(server.address));
    if (bind_resp < 0){
        char error_message[75];
        sprintf(error_message, "Failed to bind socket (%d) on port %d! error : %d\n", server.socket, port, bind_resp);
        perror(error_message);
        exit(2);
    };
    if (server.port == 0) {
        // Get the assigned port number
        int addr_len = sizeof(server.address);
        if (getsockname(server.socket, (struct sockaddr *)&server.address, (socklen_t *)&addr_len) < 0) {
            perror("failed to get the ephemeral port number assigned!");
            close(server.socket);
            exit(1);
        }
        server.port = ntohs(server.address.sin_port);
    }

    // that listen part
    int listen_resp = listen(server.socket, server.backlog);
    if (listen_resp < 0) {
        char error_message[75];
        sprintf(error_message, "Error listening to address (port: %d)\nerror : %d\n", port, listen_resp);
        perror(error_message);
        exit(3);
    }

    server.launch = launch;
    server.close = server_destructor;

    // adding SIGINT and SIGTERM handlers
    if (global_server) {
        if (signal(SIGTERM, _handle_sigterm) == SIG_ERR) {
            perror("Unable to catch SIGTERM");
            exit(-1);
        }

        if (signal(SIGINT, _handle_sigint) == SIG_ERR) {
            perror("Unable to catch SIGINT");
            exit(-1);
        }
    }
    return server;
}

/**
@brief garbage collection for the server
*/
void server_destructor(struct Server * server) {
    if (is_server_alive(server)) {
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

void _handle_sigterm(int sig) {
    printf("Recieving SIGTERM signal. Exiting gracefully...\n");
    server_destructor(g_running_server);
    exit(0);
}

void _handle_sigint(int sig) {
    printf("Receiving SIGINT signal (Ctrl+C). Exiting gracefully...\n");
    server_destructor(g_running_server);
    exit(0);
}
