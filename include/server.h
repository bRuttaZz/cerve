// config.h
// the general configurations of the server


#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

struct Server {
    int address_family;
    int socket_type;
    int protocol;
    int port;
    int backlog;
    unsigned long interface;
    struct sockaddr_in address;
    int socket;
    void (*launch)(struct Server *);
    void (*close)(struct Server *);
};

// the globals
extern struct Server * g_running_server;

// functions
struct Server server_constructor(
    int address_family, int socket_type, int protocol, int port, int backlog,
    unsigned long interface, int global_server, void (*launch)(struct Server *)
);
void server_destructor(struct Server * server);
int is_server_alive(struct Server * server);

#endif /* SERVER_H */
