// here we talk to the portal (let me make it readable as possible (As i never found such a one))
#include "../../../include/utils.h"
#include "../../../include/screencast.h"
#include "dbus/dbus-protocol.h"
#include <string.h>
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// send and get reply from dbus
DBusMessage* _sc_dbus_call(_DbusConnectionHandle *conn, DBusMessage *msg, DBusMessageIter *resp_args) {
    DBusPendingCall* pending;
    char msgs[300];
    DBusMessage *reply;

    // Send the message and wait for a reply
    dbus_connection_send_with_reply(conn->conn, msg, &pending, -1);
    dbus_message_unref(msg);
    dbus_pending_call_block(pending);
    reply = dbus_pending_call_steal_reply(pending);
    dbus_pending_call_unref(pending);
    // check reply
    if (!reply) {
        g_logger.error("sc dbus request reply NULL");
        return NULL;
    }
    if (dbus_error_is_set(&conn->error)) {
        sprintf(msgs, "sc dbus request, DBus Error: %s", conn->error.message);
        g_logger.error(msgs);
        return NULL;
    }
    // get the response from the reply
    if (!dbus_message_iter_init(reply, resp_args)) {
        g_logger.error("sc dbus Reply has no arguments");
        return NULL;
    }
    if (DBUS_MESSAGE_TYPE_ERROR == dbus_message_iter_get_arg_type(resp_args)) {
        g_logger.error("Session request reply argument is not an object path!");
        return NULL;
    }
    int arg_type = dbus_message_iter_get_arg_type(resp_args);
    sprintf(msgs, "SC dbus response: [%c]\n", arg_type);
    g_logger.debug(msgs);
    if (DBUS_TYPE_STRING == arg_type){
        char *some[300];
        dbus_message_iter_get_basic(resp_args, some);
        sprintf(msgs, "DBUS > '%s'", some[0]);
        g_logger.debug(msgs);
    }

    return reply;
}

// make connection to
int sc_make_dbus_connection(_DbusConnectionHandle *conn_handle) {
    dbus_error_init(&conn_handle->error);   // pointing dbus error handler

    conn_handle->conn = dbus_bus_get(DBUS_BUS_SESSION, &conn_handle->error);
    if (!conn_handle->conn) {
        char msg[300];
        sprintf(msg, "[sc_make_dbus_connection] connection Null: %s\n", conn_handle->error.message);
        g_logger.error(msg);
        dbus_error_free(&conn_handle->error);
        return 1;
    }
    const char *raw_sender_name = dbus_bus_get_unique_name(conn_handle->conn);
    char *sender_name = (char*) malloc(strlen(raw_sender_name) * sizeof(char));
    strcpy(sender_name, raw_sender_name+1); // omit the first character
    replace_char(sender_name, '.', '_');
    conn_handle->sender_name = sender_name;

    // setting up the objectDBusConnection *connection
    conn_handle->req_token_counter = 0;
    conn_handle->ses_token_counter = 0;
    conn_handle->session_handle = NULL;

    return 0;
}

// close connection
void sc_close_dbus_connection(_DbusConnectionHandle *conn_handle) {
    DBusMessage *msg;
    DBusPendingCall* pending;
    msg = dbus_message_new_method_call(SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "Close");
    if (!msg) {
        g_logger.error("error formatting dbus method call");
    } else {
        // Send the message and wait for a reply
        if (!dbus_connection_send_with_reply(conn_handle->conn, msg, &pending, -1)) {
            g_logger.error("failed to send CLOSE message\n");
            dbus_message_unref(msg);
            return;
        }
        dbus_pending_call_block(pending);
    }

    free(conn_handle->sender_name);
    dbus_error_free(&conn_handle->error);
    dbus_connection_unref(conn_handle->conn);
}

// create dbus session
int sc_make_dbus_session(_DbusConnectionHandle * conn) {

    DBusMessage *msg, *reply;
    DBusMessageIter args;
    char msgs[300];
    char session_token[strlen(SC_BUS_TOKEN_FMT_STRING)+12], *session_token_ptr;
    char session_path[
        strlen(SC_BUS_PATH_FMT_STRING_S)+strlen(SC_BUS_TOKEN_FMT_STRING)+12+strlen(conn->sender_name) + 2
    ];
    sc_get_new_dbus_req_path(conn, SC_DBUS_REQ_TYPE_SESSION, session_token, session_path);
    session_token_ptr = session_token;

    // format session message
    msg = dbus_message_new_method_call(SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "CreateSession");
    if (!msg) {
        g_logger.error("error formatting dbus method call");
        return 1;
    }

    DBusMessageIter options, dict_iter, variant_iter;

    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &options);

    // Add session_handle_token
    const char* key_session_handle_token = "session_handle_token";
    dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, NULL, &dict_iter);
    dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &key_session_handle_token);
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &session_token_ptr);
    dbus_message_iter_close_container(&dict_iter, &variant_iter);
    dbus_message_iter_close_container(&options, &dict_iter);
    dbus_message_iter_close_container(&args, &options);

    reply = _sc_dbus_call(conn, msg, &args);
    if (!reply) return 2;

    if (DBUS_TYPE_OBJECT_PATH != dbus_message_iter_get_arg_type(&args)) {
        g_logger.error("Session request reply argument is not an object path!");
        dbus_message_unref(reply);
        return 2;
    }
    dbus_message_unref(reply);

    conn->session_handle = strdup(session_path);        // TODO: stop using this hack and have to get the request right out from dbus 'Response' signal
    sprintf(msgs, "SC session created: %s\n", conn->session_handle);
    g_logger.debug(msgs);

    return 0;
}

int sc_request_screen_cast(_DbusConnectionHandle *conn) {
    DBusMessage *msg, *reply;
    DBusMessageIter args;
    const char *session_handle = conn->session_handle;

    // format new message
    msg = dbus_message_new_method_call(SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "SelectSources");
    if (!msg) {
        g_logger.error("error formatting dbus method call : SelectSources");
        return 1;
    }
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_OBJECT_PATH, &session_handle);

    // Start creating the options dictionary
    DBusMessageIter dict_iter;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);

    // Add types
    DBusMessageIter entry_iter, variant_iter;
    const char* key1 = "types";
    dbus_uint32_t type = 1|2;
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry_iter);
    dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key1);
    dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "u", &variant_iter);
    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_UINT32, &type);
    dbus_message_iter_close_container(&entry_iter, &variant_iter);
    dbus_message_iter_close_container(&dict_iter, &entry_iter);

    // Add types
    // const char* key2 = "multiple";
    // dbus_bool_t multiple = FALSE;
    // dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry_iter);
    // dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key2);
    // dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
    // dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &multiple);
    // dbus_message_iter_close_container(&entry_iter, &variant_iter);
    // dbus_message_iter_close_container(&dict_iter, &entry_iter);

    dbus_message_iter_close_container(&args, &dict_iter);

    reply = _sc_dbus_call(conn, msg, &args);
    if (!reply) return 2;
    g_logger.debug("sources selected!");
    dbus_message_unref(reply);


    // Start the screencast
    msg = dbus_message_new_method_call(SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "Start");
    if (!msg) {
        g_logger.error("error formatting dbus method call : Start");
        return 1;
    }
    if (!msg) {
        fprintf(stderr, "Failed to create message\n");
        return 1;
    }
    char *parent_window = "";
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_OBJECT_PATH, &session_handle);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parent_window);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
    // empty options for now
    dbus_message_iter_close_container(&args, &dict_iter);

    reply = _sc_dbus_call(conn, msg, &args);
    if (!reply) return 2;
    g_logger.debug("stream started!");
    dbus_message_unref(reply);

    sleep(10);
    return 0;
};


// helper defenitions
// don't forget to release memory space (ooh! i think this one is a bad implementation)
void sc_get_new_dbus_req_path(_DbusConnectionHandle *conn, enum _DbusReqType req_type, char* res_token, char* res_path){
    int counter_pos;
    char *fmt_str;
    switch (req_type) {
        case SC_DBUS_REQ_TYPE_REQUEST:
            conn->ses_token_counter += 1;
            counter_pos = conn->ses_token_counter;
            fmt_str = SC_BUS_PATH_FMT_STRING_R;
            break;
        case SC_DBUS_REQ_TYPE_SESSION:
            conn->req_token_counter += 1;
            counter_pos = conn->req_token_counter;
            fmt_str = SC_BUS_PATH_FMT_STRING_S;
            break;
        default:
            return ;
    }

    sprintf(res_token, SC_BUS_TOKEN_FMT_STRING, counter_pos);
    sprintf(res_path, fmt_str, conn->sender_name, res_token);
}
