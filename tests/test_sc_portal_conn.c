// test screen cast portal connections
#include "../include/screencast.h"
#include "../include/utils.h"
#include "./tests.h"
#include <stdlib.h>
#include <stdio.h>


void test_dbus_connection_maker() {
    int resp;
    _DbusConnectionHandle conn_handler;

    resp = sc_make_dbus_connection(&conn_handler);
    if (resp) {
        g_logger.error("Error in 'sc_make_dbus_connection' ");
        exit(-1);
    }
    char msg[100];
    sprintf(msg, "new connection extablished: %s", conn_handler.sender_name);
    g_logger.debug(msg);

    resp = sc_request_screen_cast(&conn_handler);
    if (resp) {
        g_logger.error("Error in 'sc_request_screen_cast' ");
        sc_close_dbus_connection(&conn_handler);
        exit(-1);
    }
    g_logger.debug("connection closing..");
    sc_close_dbus_connection(&conn_handler);
}
