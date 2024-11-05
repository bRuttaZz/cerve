
#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

struct Server {
    int address_family;
    int socket_type;
    int protocol;
    int port;
    int backlog;
    unsigned long interface;
    struct sockaddr_in address;
    int socket;
    int (*launch)(struct Server *);
    void (*close)(struct Server *);
};

enum ListenerEvent {
    SERVER_EVENT_UNDEFINED,             // not initialised state
    SERVER_EVENT_STARTING,              // server starting
    SERVER_EVENT_STARTUP_COMPLETED,     // startup completed
    SERVER_EVENT_CLOSED,                // on server closure
};


/**
@brief get server state
*/
enum ListenerEvent get_server_state(void);

/**
@brief wait for server state change (will block, until state change)
@returns current server state
*/
enum ListenerEvent wait_server_state_change(void);

/**
@brief get the number of currently active jobs being done
*/
int get_active_job_count(void);

/**
@brief get the number of currently active jobs being processed
after it changes
*/
int wait_active_job_count_change(void);

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
    struct Server *server, int address_family, int socket_type, int protocol, int port, int backlog,
    unsigned long interface, int (*launch)(struct Server *)
);

/**
@brief garbage collection for the server
*/
void server_destructor(struct Server * server);

/**
@brief check if the server is alive or not
@param server - the server pointer
@returns 1 if server is alive else returns 0;
*/
int is_server_alive(struct Server * server);

/**
@brief The entry point for cerve system. Start the webserver and listen for connections
@return the status code. 0 for success
*/
int start_listener(void);

/**
@brief close running listeners and worker threads
*/
void close_listener(void);


#endif /* SERVER_H */
