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

enum ListenerEvent _g_server_state = SERVER_EVENT_UNDEFINED;         // server state variable
pthread_mutex_t _g_server_state_mut = PTHREAD_MUTEX_INITIALIZER;     // server state access mutex
pthread_cond_t _g_server_state_sig = PTHREAD_COND_INITIALIZER;       // will be trigered on changin server state

int _g_job_sock = 0;
struct Server *_g_server = NULL;
char _g_server_name[INET_ADDRSTRLEN];

int _g_thread_identity = 0;
pthread_mutex_t _g_thread_identity_mut = PTHREAD_MUTEX_INITIALIZER;     // to access thread identity
pthread_cond_t _g_thread_identity_sig = PTHREAD_COND_INITIALIZER;



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
    int thread_id;
    int job;
    int bytes_read;
    char buffer[1024];
    char response_msg[] = "ok";

    pthread_mutex_lock(&_g_thread_identity_mut);
    _g_thread_identity++;
    thread_id = _g_thread_identity;

    g_logger.info("[worker %d] startup complete!", thread_id);
    pthread_cond_signal(&_g_thread_identity_sig);
    pthread_mutex_unlock(&_g_thread_identity_mut);

    while (1) {
        // look for new job
        pthread_mutex_lock(&g_job_mutex);
        while (_g_job_sock == 0) {
            pthread_cond_wait(&g_job_cond_new, &g_job_mutex);
        }
        // look for process termination
        if (get_server_state() == SERVER_EVENT_CLOSED) {
            // process termination sign
            pthread_cond_signal(&g_worker_cond_available);
            pthread_mutex_unlock(&g_job_mutex);
            g_logger.debug("[worker %d] Termination signal! worker terminated!", thread_id);
            break;
        }
        job = _g_job_sock;
        _g_job_sock = 0;
        pthread_cond_signal(&g_worker_cond_available);
        pthread_mutex_unlock(&g_job_mutex);

        g_logger.debug("[worker %d] Got new sock", thread_id);

        // do the work with the sock
        bytes_read = read(job, buffer, 1024);
        if (bytes_read < 0) {
            g_logger.error("[worker %d] Error reading data from socket. Connection skipped!", thread_id);
            continue;;
        }
        g_logger.debug("[worker %d] got message : (%d)", thread_id, bytes_read);
        write(job, response_msg, strlen(response_msg));

        close(job);
        g_logger.debug("[worker %d] reponse wrote and connection closed!", thread_id);

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
    int errors = 0;

    g_logger.debug("[listener] killing all workers!");
    _set_server_state(SERVER_EVENT_CLOSED);
    pthread_mutex_lock(&g_job_mutex);
    _g_job_sock = -5;
    pthread_cond_broadcast(&g_job_cond_new);
    pthread_mutex_unlock(&g_job_mutex);

    for (int i=0; i<thread_count; i++) {
        g_logger.info("[listener] closing worker [%d]", i);
        // pthread_cancel(thread_ids[i]);
        if (pthread_join(thread_ids[i], NULL) !=0) {
            g_logger.error("[listener] error closing worker thread [%d]", i);
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
    int address_len = sizeof(server->address);

    pthread_t thread_ids[g_worker_count];
    int threads[g_worker_count];

    // spin up all threads
    for (int i=0; i<g_worker_count; i++) {
        threads[i] = i;
        int thread_resp = pthread_create(&thread_ids[i], NULL, worker, &threads[i]);
        if (thread_resp != 0) {
            g_logger.error("[listener] error spin server thread! [%d]", thread_resp);
            return -1;
        }
        g_logger.debug("created worker [%d]", i+1);
    }

    g_logger.debug("[listener] waiting for worker startup!");
    pthread_mutex_lock(&_g_thread_identity_mut);
    if (_g_thread_identity < g_worker_count) {
        pthread_cond_wait(&_g_thread_identity_sig, &_g_thread_identity_mut);
    }
    pthread_mutex_unlock(&_g_thread_identity_mut);

    if (inet_ntop(AF_INET, &(server->address.sin_addr), _g_server_name, sizeof(_g_server_name)) == NULL) {
            g_logger.error("[listener] error getting ip expansion: inet_ntop\n");
            _close_all_workers(thread_ids, g_worker_count);
            g_logger.error("[listener] stoped!");
            return -2;
    }
    // listen for request
    g_logger.info("listening at \e]8;;http://%s:%d\e\\http://%s:%d\e]8;;\e\\ \n",
        _g_server_name, server->port, _g_server_name, server->port);
    _set_server_state(SERVER_EVENT_STARTUP_COMPLETED);

    while (1) {
        pthread_mutex_lock(&g_job_mutex);
        while (_g_job_sock > 0) {
            // look for server closure (even if jobs are there yet to be resolved)
            if (get_server_state() == SERVER_EVENT_CLOSED) {
                pthread_mutex_unlock(&g_job_mutex);
                // process termination signal
                _close_all_workers(thread_ids, g_worker_count);
                g_logger.error("[listener] terminated!");
                return -3;
            }
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
            g_logger.error("[listener] server socket connection interrupted. access error : %d", _conn_sock);
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
        server_constructor(&cerve, AF_INET, SOCK_STREAM, 0, g_server_port, g_worker_count, INADDR_ANY, listener)
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
