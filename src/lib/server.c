
#include "../../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

/**
@brief create a server to start with

@param domain
@param service
@param protocol
@param port
@param backlog
@param interface
@param launch
@
*/
struct Server server_constructor(
    int domain, int service, int protocol,
    int port, int backlog,
    unsigned long interface, void (*launch)(struct Server *)
) {
    struct Server server;

    server.domain = domain;
    server.service = service;
    server.protocol = protocol;
    server.port = port;
    server.backlog = backlog;
    server.interface = interface;

    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);

    server.socket = socket(domain, service, protocol);
    if (server.socket <= 0) {
        char error_message[50];
        sprintf(error_message, "Failed to create socket : %d \n", server.socket);
        perror(error_message);
        exit(1);
    }

    int bind_resp = bind(server.socket, (struct sockaddr *)&server.address, sizeof(server.address));
    if (bind_resp < 0){
        char error_message[75];
        sprintf(error_message, "Failed to bind socket (%d) on port %d! \nerror : %d\n", server.socket, port, bind_resp);
        perror(error_message);
        exit(2);
    };
    // that long awaited listen part
    int listen_resp = listen(server.socket, server.backlog);
    if (listen_resp < 0) {
        char error_message[75];
        sprintf(error_message, "Error listening to address (port: %d)\nerror : %d\n", port, listen_resp);
        perror(error_message);
        exit(3);
    }

    server.launch = launch;

    return server;
}
