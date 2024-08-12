// here we talk to the portal (let me make it readable as possible (As i never found such a one))
#include "../../../include/utils.h"
#include "../../../include/screencast.h"
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void _get_first_stream(GVariant *response);

// helpers
void _on_signal_callback (GDBusConnection *connection, const char *sender_name, const char *object_path,
            const char  *interface_name, const char *signal_name, GVariant  *parameters, void*  conn_manager){
    _DbusConnectionHandle *conn_man = (_DbusConnectionHandle*) conn_manager;

    char msg[500];
    sprintf(msg, "[ResponseSignal] Response signal recieved @ %s", object_path);
    g_logger.debug(msg);
    if (conn_man->event_lookup_path) {
        if(!strcmp(object_path, conn_man->event_lookup_path)){
            // _get_first_stream(parameters);
            conn_man->event_lookup_path = NULL;
            g_main_loop_quit(conn_man->loop);
        };
    }
}

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

    conn_handle->event_subs_id = g_dbus_connection_signal_subscribe(
            conn_handle->conn,
            SC_BUS_NAME,
            SC_BUS_IF_REQUEST,
            "Response",
            NULL,
            NULL,
            G_DBUS_SIGNAL_FLAGS_NONE,
            _on_signal_callback,
            conn_handle,
            NULL
    );
    conn_handle->loop = g_main_loop_new(NULL, TRUE);

    // setting up the objectDBusConnection *connection
    conn_handle->req_token_counter = 0;
    conn_handle->ses_token_counter = 0;
    conn_handle->session_handle = NULL;
    conn_handle->event_lookup_path = "";
    // conn_handle->dbus_response_msg = NULL;

    return 0;
}

// close connection
void sc_close_dbus_connection(_DbusConnectionHandle *conn_handle) {
    g_free(conn_handle->sender_name);
    g_dbus_connection_signal_unsubscribe(conn_handle->conn, conn_handle->event_subs_id);
    g_object_unref(conn_handle->conn);
    g_main_loop_unref(conn_handle->loop);
}


int _sc_create_session(_DbusConnectionHandle *conn) {
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

    // const gchar *object_path;
    // g_variant_get(reply, "(&o)", &object_path);
    conn->session_handle = strdup(session_path);                // TODO : has to listen for 'Response' event in dbus and get it
    char msgs[256];
    sprintf(msgs, "SC session created: %s", conn->session_handle);
    g_logger.debug(msgs);
    g_variant_unref(reply);
    return  0;
}

int _sc_select_sources(_DbusConnectionHandle *conn) {
    GError *error = NULL;
    GVariantBuilder options_builder;
    GVariant *reply;

    // now ask for source selection & screencast start
    g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&options_builder, "{sv}", "types", g_variant_new_uint32(1|2));
    g_variant_builder_add(&options_builder, "{sv}", "multiple", g_variant_new_boolean(FALSE));

    reply = g_dbus_connection_call_sync(
        conn->conn, SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "SelectSources",
        g_variant_new("(o@a{sv})", conn->session_handle, g_variant_builder_end(&options_builder)),
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
    return 0;
}

int _sc_start(_DbusConnectionHandle *conn) {
    GError *error = NULL;
    GVariantBuilder options_builder;
    GVariant *reply;

    char response_token[strlen(SC_BUS_TOKEN_FMT_STRING)+12];
    char response_path[
        strlen(SC_BUS_PATH_FMT_STRING_R)+strlen(SC_BUS_TOKEN_FMT_STRING)+12+strlen(conn->sender_name) + 2
    ];
    sc_get_new_dbus_req_path(conn, SC_DBUS_REQ_TYPE_REQUEST, response_token, response_path);

    g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&options_builder, "{sv}", "handle_token", g_variant_new_string(response_token));

    conn->event_lookup_path = response_path;
    reply = g_dbus_connection_call_sync(
        conn->conn, SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "Start",
        g_variant_new("(os@a{sv})", conn->session_handle, "", g_variant_builder_end(&options_builder)),
        G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error
    );
    if (error) {
        char msgs[256];
        sprintf(msgs, "Error formatting D-Bus method call : Start : %s\n", error->message);
        g_logger.error(msgs);
        g_error_free(error);
        return 1;
    }
    g_variant_unref(reply);
    g_main_loop_run(conn->loop);        // wait for stream selection Response to come
    g_logger.debug("Stream started!");
    return 0;
}

int _sc_bind_pipewire(_DbusConnectionHandle *conn) {
    GError *error = NULL;
    GVariantBuilder options_builder;
    GVariant *reply;

    g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);

    reply = g_dbus_connection_call_sync(
        conn->conn, SC_BUS_NAME, SC_BUS_OBJECT_PATH, SC_BUS_IF_SCREENCAST, "OpenPipeWireRemote",
        g_variant_new("(o@a{sv})", conn->session_handle, g_variant_builder_end(&options_builder)),
        G_VARIANT_TYPE("(h)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error
    );
    if (error) {
        char msgs[256];
        sprintf(msgs, "Error formatting D-Bus method call : OpenPipeWireRemote : %s\n", error->message);
        g_logger.error(msgs);
        g_error_free(error);
        return 1;
    }

    g_variant_get(reply, "(h)", &conn->pipewire_fd);
    g_variant_unref(reply);
    char msgs[256];
    sprintf(msgs, "SC pipewire fd created: %d", conn->pipewire_fd);
    g_logger.debug(msgs);
    return 0;
}

// create dbus session
int sc_request_screen_cast(_DbusConnectionHandle * conn) {
    int resp_status;

    // create session
    resp_status = _sc_create_session(conn);
    if (resp_status) return 1;

    // select screen sources
    resp_status = _sc_select_sources(conn);
    if (resp_status) return 2;

    // request stream start
    resp_status = _sc_start(conn);
    if (resp_status) return 3;



    // reply = (GVariant *) conn->dbus_response_msg;
    // _get_first_stream(reply);
    // g_variant_get_child()
    // while(g_variant_iter_next(GVariantIter *iter, const gchar *format_string, ...))
    // conn->dbus_response_msg=NULL;

    resp_status = _sc_bind_pipewire(conn);


    sleep(5);

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
