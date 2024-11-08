#include "../include/server.h"
#include "../include/server-configs.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


void* _listener_thread(void * _) {
    int status = start_listener();
    if (status != -3) {
        g_logger.error("[TEST] error in listner [%d] . Current state : %d !", status, get_server_state());
        exit(-3);
    };
    g_logger.info("[TEST] server closed successfully on close signal!");
    return  NULL;
}

int test_listener() {
    g_logger.info("[TEST] testing SERVER LISTENER..\n");
    char resp[4];
    char port[5];
    int active_jobs = 0;
    pthread_t thread_id;
    enum ListenerEvent server_event;

    g_server_port = 0;
    g_worker_count = 2;
    g_logger.level = 99;
    g_logger.info("[TEST] initial configs set");

    int thread_resp = pthread_create(&thread_id, NULL, _listener_thread, NULL);
    if (thread_resp != 0) {
        g_logger.error("[TEST] error spin server thread! [%d]", thread_resp);
        exit(-1);
    }

    g_logger.debug("[TEST] waiting for server startup complete");
    server_event = get_server_state();
    while (server_event<2) {
        server_event = wait_server_state_change();
    }
    g_logger.debug("[TEST] server startup complete");

    sprintf(port, "%d", g_server_port);
    g_logger.info("[TEST] sending test message to server..\n");
    int resp_status = raise_http_request("localhost", port, "/", "", "GET", resp, 4); // raise request
    if (resp_status != 0) {
        close_listener();
        g_logger.error("[TEST] error getting request reponse (client failure) : %d", resp_status);
        return -1;
    }

    g_logger.info("[TEST] waiting for the server thread to finish doing work!");
    active_jobs = get_active_job_count();
    while (active_jobs) {
        active_jobs = wait_active_job_count_change();
    }

    g_logger.info("[TEST] closing server thread..");
    close_listener();

    if (strcmp(resp, "HTTP")) {
        g_logger.error("[TEST] unexpected response from server : %s\n", resp);
        return -1;
    }
    g_logger.info("[TEST] message received from thread : %s\n", resp);
    server_event = get_server_state();
    while (server_event != SERVER_EVENT_CLOSED) {
        server_event = wait_server_state_change();
    }

    thread_resp = pthread_join(thread_id, NULL);
    if (thread_resp !=0) {
        g_logger.error("[TEST] error closing server thread!\n");
        return -1;
    }
    g_logger.info("[TEST] SERVER LISTENER ✅\n\n");
    return 0;
}
