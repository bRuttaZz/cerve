// config.h
// the general configurations of the server


#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

struct Server {
    int domain;
    int service;
    int protocol;
    int port;
    int backlog;
    unsigned long interface;
    struct sockaddr_in address;
    int socket;
    void (*launch)(struct Server *);
};

struct Server server_constructor(
    int domain, int service, int protocol,
    int port, int backlog,
    unsigned long interface, void (*launch)(struct Server *)
);

#endif /* SERVER_H */
