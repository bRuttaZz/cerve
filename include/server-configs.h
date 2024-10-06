#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <arpa/inet.h>


extern int g_server_port;
extern char g_server_name[INET_ADDRSTRLEN];
extern int g_worker_count;

#endif /* SERVER_CONFIG_H */
