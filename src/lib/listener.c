#include "../../include/server.h"
#include "../../include/server-configs.h"
#include "../../include/utils.h"
#include "../../include/messages.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

// globals
int g_server_port = 8000;
int g_worker_count = 4;

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
@brief load header file from given path
// TODO: headerfile format validator
*/
int _load_header_file(char **header_dat) {
    if (strlen(g_custom_resp_header_file_path)) {
        // there is a custom header file
        long file_size;
        char *raw_header_mem_pointer;
        char *header;
        FILE * file = fopen(g_custom_resp_header_file_path, "r");
        if (!file) {
            g_logger.error("Error reading response headerfile");
            return 2;
        }
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        raw_header_mem_pointer = (char *) malloc((file_size+1) * sizeof(char));
        if (!raw_header_mem_pointer) {
            g_logger.error("Not enough space to load the headerfile! : %d KB", file_size / 1000);
            fclose(file);
            return 12;
        }

        size_t read_size = fread(raw_header_mem_pointer, sizeof(char), file_size, file);
        if (read_size != file_size) {
            g_logger.error("Failed to read headerfile! : %d KB read", read_size/1000);
            free(raw_header_mem_pointer);
            fclose(file);
            return 5;
        }
        raw_header_mem_pointer[file_size] = '\0';
        fclose(file);
        header = raw_header_mem_pointer;

        // trim leading and trailing whitespaces
        for (int i=0; i < strlen(header)-1; i++) {
            if (isspace((unsigned char)*header))
                header++;
            else break;
        }

        // trim traling spaces
        char * end = header + strlen(header) - 1;
        while (end>header && isspace((unsigned char) *end)) end--;
        *(end + 1) = '\0';

        file_size = strlen(header);
        *header_dat = (char *) malloc((file_size+1) * sizeof(char));
        if (!*header_dat) {
            g_logger.error("Not enough space to load the headerfile ! : %d KB", file_size / 1000);
            return 12;
        }
        strncpy(*header_dat, header, file_size);
        *header_dat[file_size] = '\0';
        free(raw_header_mem_pointer);

        return 0;
    }
    *header_dat = (char *) malloc(sizeof(char));
    if (!*header_dat) {
        g_logger.error("Error loading headerfile (not enough mem)");
        return 12;
    }
    *header_dat[0] = '\0';
    return 0;
}

/**
@brief http worker (client socket handler)
*/
void* worker(void* _) {
    int thread_id;
    int job;
    Request request;
    int request_read_status;
    char *cust_response_headers = "";
    char response_msg[] = "HTTP/1.1 200 OK\n\nOK";

    request.location = NULL;
    request.method = NULL;
    request.version = NULL;

    pthread_mutex_lock(&_g_thread_identity_mut);
    // worker loadup
    _g_thread_identity++;
    thread_id = _g_thread_identity;

    if (_load_header_file(&cust_response_headers)) {
        g_logger.error("[worker %d] Error booting wroker thread!", thread_id);
        close_listener();
        return  NULL;
    }

    // worker loadup ends
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
        request_read_status = read_http_request(job, &request);
        if (request_read_status < 0) {
            g_logger.error("[worker %d] Error reading data from socket. Connection skipped! : %d", thread_id, request_read_status);
            close(job);
            continue;
        }
        if (request_read_status > 0) {
            if (request_read_status == 2) {
                g_logger.error("[worker %d] Invalid http method! Skipping connection!", thread_id, request_read_status);
            } else {
                g_logger.error("[worker %d] Malformmatted request! Skipping connection : %d", thread_id, request_read_status);
            }
            free_request(&request);
            close(job);
            continue;
        }
        g_logger.info("[worker %d] request: %s - %s @ %s", thread_id, request.version, request.method, request.location);
        write(job, response_msg, strlen(response_msg));

        free_request(&request);
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
        g_logger.info("[listener] closing worker [%d]", i+1);
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
        g_logger.debug("[listener] looking for new connection..");
        _conn_sock = accept(server->socket, (struct sockaddr *)&server->address, (socklen_t *)&address_len);
        if (_conn_sock<=0) {
            g_logger.error("[listener] server socket connection interrupted. access error : %d", _conn_sock);
            _close_all_workers(thread_ids, g_worker_count);
            g_logger.error("[listener] terminated!");
            return -3;
        }
        g_logger.debug("[listener] got new connection..");

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
