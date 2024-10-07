#include "../../include/server.h"
#include "../../include/server-configs.h"
#include "../../include/utils.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <errno.h>


pthread_mutex_t g_job_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_job_cond_new = PTHREAD_COND_INITIALIZER;
pthread_cond_t g_worker_cond_available = PTHREAD_COND_INITIALIZER;

char g_server_name[INET_ADDRSTRLEN];
int g_server_port = 8000;
int g_worker_count = 4;

enum ListenerEvent _g_server_state = SERVER_EVENT_UNDEFINED;         // server state variable
pthread_mutex_t _g_server_state_mut = PTHREAD_MUTEX_INITIALIZER;     // server state access mutex
pthread_cond_t _g_server_state_sig = PTHREAD_COND_INITIALIZER;       // will be trigered on changin server state

int _g_job_sock = 0;
struct Server *_g_server = NULL;


/**
@brief get server state
*/
enum ListenerEvent get_server_state(void) {
    enum ListenerEvent state;
    pthread_mutex_lock(&_g_server_state_mut);
    state = _g_server_state;
    pthread_mutex_unlock(&_g_server_state_mut);
    return state;
}

/**
@brief wait for server state change (will block, until state change)
@returns current server state
*/
enum ListenerEvent wait_server_state_change(void) {
    enum ListenerEvent state;
    pthread_cond_wait(&_g_server_state_sig, &_g_server_state_mut);
    state = _g_server_state;
    pthread_mutex_unlock(&_g_server_state_mut);
    return state;
}

/**
@brief set new server state
@param state - new state to set
*/
void _set_server_state(enum ListenerEvent state) {
    pthread_mutex_lock(&_g_server_state_mut);
    _g_server_state = state;
    pthread_mutex_unlock(&_g_server_state_mut);
    pthread_cond_signal(&_g_server_state_sig);
}


/**
@brief http worker (client socket handler)
*/
void* worker(void* _) {
    int job;
    int bytes_read;
    char buffer[1024];
    char sys_msgs[200];
    char response_msg[] = "ok";

    while (1) {
        // look for new job
        pthread_mutex_lock(&g_job_mutex);
        while (_g_job_sock == 0) {
            pthread_cond_wait(&g_job_cond_new, &g_job_mutex);
        }
        // look for process termination
        if (get_server_state() == SERVER_EVENT_CLOSED) {
            // process termination sign
            pthread_mutex_unlock(&g_job_mutex);
            g_logger.debug("[worker] Termination signal! worker terminated!");
            break;
        }
        job = _g_job_sock;
        _g_job_sock = 0;
        pthread_mutex_unlock(&g_job_mutex);

        // do the work with the sock
        bytes_read = read(job, buffer, 1024);
        if (bytes_read < 0) {
            g_logger.error("[worker] Error reading data from socket. Connection skipped!");
            continue;;
        }
        sprintf(sys_msgs, "[worker] got message : (%d)", bytes_read);
        g_logger.debug(sys_msgs);
        write(job, response_msg, strlen(response_msg));

        close(job);
        g_logger.debug("[worker] reponse wrote and connection closed!");

        pthread_mutex_lock(&g_job_mutex);
        pthread_cond_signal(&g_worker_cond_available);
        pthread_mutex_unlock(&g_job_mutex);
    }
    return NULL;
}

/**
@brief To close all the listener workers
@param thread_ids - array of thread ids to be closed
@param thread_count - number of thread ids
@returns - number of error cases . 0 for success
*/
int _close_all_workers(pthread_t *thread_ids, int thread_count) {
    char msg[100];
    int errors = 0;

    g_logger.debug("[listener] killing all workers!");
    _set_server_state(SERVER_EVENT_CLOSED);
    pthread_mutex_lock(&g_job_mutex);
    _g_job_sock = -5;
    pthread_cond_broadcast(&g_job_cond_new);
    pthread_mutex_unlock(&g_job_mutex);

    for (int i=0; i<thread_count; i++) {
        sprintf(msg, "[listener] closing worker [%d]", i);
        g_logger.info(msg);
        // pthread_cancel(thread_ids[i]);
        if (pthread_join(thread_ids[i], NULL) !=0) {
            sprintf(msg, "[listener] error closing worker thread [%d]", i);
            g_logger.error(msg);
            errors++;
        }
    }
    errno = 125;
    return errors;
}

/**
@brief listen for new connection in the server
@param server - server instance
*/
int listener(struct Server * server) {
    int _conn_sock;
    char sys_msgs[200];
    int address_len = sizeof(server->address);

    pthread_t thread_ids[g_worker_count];
    int threads[g_worker_count];

    for (int i=0; i<g_worker_count; i++) {
        threads[i] = i;
        int thread_resp = pthread_create(&thread_ids[i], NULL, worker, &threads[i]);
        if (thread_resp != 0) {
            char error_msg[50] ;
            sprintf(error_msg, "[listener] error spin server thread! [%d]", thread_resp);
            g_logger.error(error_msg);
            return -1;
        }
        sprintf(sys_msgs, "created worker [%d]", i+1);
        g_logger.info(sys_msgs);
    }


    if (inet_ntop(AF_INET, &(server->address.sin_addr), g_server_name, sizeof(g_server_name)) == NULL) {
            g_logger.error("[listener] error getting ip expansion: inet_ntop\n");
            _close_all_workers(thread_ids, g_worker_count);
            g_logger.error("[listener] stoped!");
            return -2;
    }
    // listen for request
    sprintf(sys_msgs, "listening at http://%s:%d \n", g_server_name, server->port);
    g_logger.info(sys_msgs);
    _set_server_state(SERVER_EVENT_STARTUP_COMPLETED);

    while (1) {
        pthread_mutex_lock(&g_job_mutex);
        while (_g_job_sock > 0) {
            pthread_cond_wait(&g_worker_cond_available, &g_job_mutex);
        }
        pthread_mutex_unlock(&g_job_mutex);
        // look for server closure
        if (get_server_state() == SERVER_EVENT_CLOSED) {
            // process termination sign
            _close_all_workers(thread_ids, g_worker_count);
            g_logger.error("[listener] terminated!");
            return -3;
        }

        // listen for new socket connection
        g_logger.debug("looking for new connection..\n");
        _conn_sock = accept(server->socket, (struct sockaddr *)&server->address, (socklen_t *)&address_len);
        g_logger.debug("got new connection..\n");

        if (_conn_sock<=0) {
            sprintf(sys_msgs, "[listener] server socket connection interrupted. access error : %d", _conn_sock);
            g_logger.error(sys_msgs);
            _close_all_workers(thread_ids, g_worker_count);
            g_logger.error("[listener] terminated!");
            return -3;
        }

        pthread_mutex_lock(&g_job_mutex);
        _g_job_sock = _conn_sock;
        // signal new job availability
        pthread_cond_signal(&g_job_cond_new);
        pthread_mutex_unlock(&g_job_mutex);

    }
}

/**
@brief close running listeners and worker threads
*/
void close_listener() {
    server_destructor(_g_server);
    _set_server_state(SERVER_EVENT_CLOSED);
    pthread_cond_signal(&g_worker_cond_available);
}

// Signal handlers
void _handle_sigterm(int sig) {
    g_logger.error("Recieving SIGTERM signal. Exiting gracefully...\n");
    close_listener();
    exit(4);
}

void _handle_sigint(int sig) {
    g_logger.error("Receiving SIGINT signal (Ctrl+C). Exiting gracefully...\n");
    close_listener();
}

/**
@brief The entry point for cerve system. Start the webserver and listen for connections
@return the status code. 0 for success, -3 on closed after calling server-event-closed
*/
int start_listener() {
    _set_server_state(SERVER_EVENT_STARTING);
    struct Server cerve;
    if (
        server_constructor(&cerve, AF_INET, SOCK_STREAM, 0, g_server_port, 1, INADDR_ANY, listener)
    ) {
        g_logger.error("Error creating server construct!");
        return 1;
    }
    _g_server = &cerve;

    // binding sig handles
    if (signal(SIGTERM, _handle_sigterm) == SIG_ERR) {
        g_logger.error("Unable to bind SIGTERM listener");
        return errno;
    }

    if (signal(SIGINT, _handle_sigint) == SIG_ERR) {
        g_logger.error("Unable to bind SIGINT listener");
        return errno;
    }
    return _g_server->launch(_g_server);;
}
