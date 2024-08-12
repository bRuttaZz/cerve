// here we talk to the portal (let me make it readable as possible (As i never found such a one))
#include "../../../include/utils.h"
#include "../../../include/screencast.h"
#include "dbus/dbus-protocol.h"
#include <string.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// send and get reply from dbus
// DBusMessage* _sc_dbus_call(_DbusConnectionHandle *conn, DBusMessage *msg, DBusMessageIter *resp_args) {
//     DBusPendingCall* pending;
//     char msgs[300];
//     DBusMessage *reply;

//     // Send the message and wait for a reply
//     dbus_connection_send_with_reply(conn->conn, msg, &pending, -1);
//     dbus_message_unref(msg);
//     dbus_pending_call_block(pending);
//     reply = dbus_pending_call_steal_reply(pending);
//     dbus_pending_call_unref(pending);
//     // check reply
//     if (!reply) {
//         g_logger.error("sc dbus request reply NULL");
//         return NULL;
//     }
//     if (dbus_error_is_set(&conn->error)) {
//         sprintf(msgs, "sc dbus request, DBus Error: %s", conn->error.message);
//         g_logger.error(msgs);
//         return NULL;
//     }
//     // get the response from the reply
//     if (!dbus_message_iter_init(reply, resp_args)) {
//         g_logger.error("sc dbus Reply has no arguments");
//         return NULL;
//     }
//     if (DBUS_MESSAGE_TYPE_ERROR == dbus_message_iter_get_arg_type(resp_args)) {
//         g_logger.error("Session request reply argument is not an object path!");
//         return NULL;
//     }
//     int arg_type = dbus_message_iter_get_arg_type(resp_args);
//     sprintf(msgs, "SC dbus response: [%c]\n", arg_type);
//     g_logger.debug(msgs);
//     if (DBUS_TYPE_STRING == arg_type){
//         char *some[300];
//         dbus_message_iter_get_basic(resp_args, some);
//         sprintf(msgs, "DBUS > '%s'", some[0]);
//         g_logger.debug(msgs);
//     }

//     return reply;
// }

// make connection to
int sc_make_dbus_connection(_DbusConnectionHandle *conn_handle) {
    GError *error = NULL;
    conn_handle->conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!conn_handle->conn) {
        char msg[300];
        sprintf(msg, "[sc_make_dbus_connection] Connection Null: %s\n", error->message);
        g_logger.error(msg);
        g_error_free(error);
        return 1;
    }

    const char *raw_sender_name = g_dbus_connection_get_unique_name(conn_handle->conn);
    conn_handle->sender_name = strdup(raw_sender_name + 1); // omit the first character
    g_strdelimit(conn_handle->sender_name, ".", '_');

    // setting up the objectDBusConnection *connection
    conn_handle->req_token_counter = 0;
    conn_handle->ses_token_counter = 0;
    conn_handle->session_handle = NULL;

    return 0;
}

// close connection
void sc_close_dbus_connection(_DbusConnectionHandle *conn_handle) {
    g_free(conn_handle->sender_name);
    g_object_unref(conn_handle->conn);
}

// create dbus session
int sc_make_dbus_session(_DbusConnectionHandle * conn) {
    GError *error = NULL;
    GVariantBuilder options_builder;
    GVariant *reply;

    char session_token[strlen(SC_BUS_TOKEN_FMT_STRING)+12];
    char session_path[
        strlen(SC_BUS_PATH_FMT_STRING_S)+strlen(SC_BUS_TOKEN_FMT_STRING)+12+strlen(conn->sender_name) + 2
    ];

    sc_get_new_dbus_req_path(conn, SC_DBUS_REQ_TYPE_SESSION, session_token, session_path);

    g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&options_builder, "{sv}", "session_handle_token", g_variant_new_string(session_token));

    reply = g_dbus_connection_call_sync(
        conn->conn, SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "CreateSession",
        g_variant_new("(a{sv})", &options_builder), G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE,
        -1, NULL,&error
    );
    if (error) {
        char msg[256];
        sprintf(msg, "error creating D-Bus session: %s", error->message);
        g_logger.error(msg);
        g_error_free(error);
        return 1;
    }

    const gchar *object_path;
    g_variant_get(reply, "(&o)", &object_path);
    conn->session_handle = strdup(session_path);                // TODO : has to listen for 'Response' event in dbus and get it
    char msgs[256];
    sprintf(msgs, "SC session created: %s", conn->session_handle);
    g_logger.debug(msgs);
    g_variant_unref(reply);

    return 0;
}

int sc_request_screen_cast(_DbusConnectionHandle *conn) {
    GError *error = NULL;
    GVariant *reply;
    const char *session_handle = conn->session_handle;

    GVariantBuilder options_builder;
    g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&options_builder, "{sv}",
        "types", g_variant_new_uint32(1 | 2), "multiple", TRUE);

    reply = g_dbus_connection_call_sync(
        conn->conn, SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "SelectSources",
        g_variant_new("(o@a{sv})", session_handle, g_variant_builder_end(&options_builder)),
        G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error
    );
    if (error) {
        char msgs[256];
        sprintf(msgs, "Error formatting D-Bus method call : SelectSources : %s\n", error->message);
        g_logger.error(msgs);
        g_error_free(error);
        return 1;
    }
    g_logger.debug("SC sources selected");
    g_variant_unref(reply);

    // request stream start
    g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
    reply = g_dbus_connection_call_sync(
        conn->conn, SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "Start",
        g_variant_new("(os@a{sv})", session_handle, "", g_variant_builder_end(&options_builder)),
        G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error
    );
    if (error) {
        char msgs[256];
        sprintf(msgs, "Error formatting D-Bus method call : Start : %s\n", error->message);
        g_logger.error(msgs);
        g_error_free(error);
        return 1;
    }
    g_logger.debug("Stream started!");
    g_variant_unref(reply);

    g_usleep(5 * G_USEC_PER_SEC);

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
