#include "../include/server.h"
#include "../include/utils.h"
#include "./tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// a simple mutex to ensure the test server thread is closed only after client
// disconnects (otherwise the system will put the port in `TIME_WAIT` state)
// as well as to ensure the proper waiting before starting each services
pthread_mutex_t client_read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t client_read_cond = PTHREAD_COND_INITIALIZER;

int g_port = 0;

void _launch(struct Server * server) {
    int address_len = sizeof(server->address);
    int new_sock;
    char buffer[1024];
    char response_msg[] = "ok";
    int bytes_read;
    char msgs[200];

    // listen for request

    sprintf(msgs,"[launch thread] waiting for new socket connection at port %d ..\n", server->port);
    g_logger.info(msgs);

    pthread_mutex_lock(&client_read_mutex);
    g_port = server->port;
    pthread_cond_signal(&client_read_cond);
    pthread_mutex_unlock(&client_read_mutex);                   // unlock after server is ready to listen

    new_sock = accept(server->socket, (struct sockaddr *)&server->address, (socklen_t *)&address_len);
    g_logger.debug("[launch thread] got new connection..\n");

    bytes_read = read(new_sock, buffer, 1024);
    if (bytes_read < 0) {
        g_logger.error("[launch thread] Error reading data from socket\n");
        exit(-1);
    }
    sprintf(msgs, "[launch thread] got message : (%d)\n", bytes_read);
    g_logger.debug(msgs);
    write(new_sock, response_msg, strlen(response_msg));

    // wait to the acquire the lock (only for test server (otherwise the port enter into `TIME_WAIT` state))
    pthread_mutex_lock(&client_read_mutex);

    close(new_sock);
    server_destructor(server);
    g_logger.info("[launch thread] reponse wrote and connection closed!\n");
}

void* _listen_thread(void * _) {
    struct Server test_server = server_constructor(
        AF_INET, SOCK_STREAM, 0, 0, 1, INADDR_ANY, _launch
    );
    test_server.launch(&test_server);
    return  NULL;
}

void test_server_constructor() {
    // g_logger.level = 3;
    pthread_t thread_id;
    char msgs[200];

    int thread_resp = pthread_create(&thread_id, NULL, _listen_thread, NULL);
    if (thread_resp != 0) {
        char error_msg[50] ;
        sprintf(error_msg, "[TEST] error spin server thread! [%d]", thread_resp);
        g_logger.error(error_msg);
        exit(-1);
    }

    pthread_mutex_lock(&client_read_mutex);                     // lock the mutex
    while (g_port == 0) {
        pthread_cond_wait(&client_read_cond, &client_read_mutex);
    }
    char resp[3];
    char port[5];
    sprintf(port, "%d", g_port);
    g_logger.info("[TEST] sending test message to server..\n");
    raise_http_request("localhost", port, "/", "", "GET", resp, 3); // raise request
    sprintf(msgs, "[TEST] message received from thread : %s\n", resp);
    g_logger.info(msgs);
    pthread_mutex_unlock(&client_read_mutex);                   // unlock the mutex
    // (usage of mutex: only for removing the testserver issue related to port `TIME_WAIT` state)


    thread_resp = pthread_join(thread_id, NULL);
    if (thread_resp !=0) {
        g_logger.error("[TEST] error closing server thread!\n");
        exit(-1);
    }
}
