#include "dbus/dbus-protocol.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SCREENCAST_BUS_NAME "org.freedesktop.portal.Desktop"
#define SCREENCAST_OBJECT_PATH "/org/freedesktop/portal/desktop"

#define SCREENCAST_BUS_IF_REQUEST "org.freedesktop.portal.Request"
#define SCREENCAST_BUS_IF_SCREENCAST "org.freedesktop.portal.ScreenCast"

// #define SESSION_INTERFACE "org.freedesktop.portal.Session"
// #define SCREENCAST_SERVICE "org.freedesktop.portal.ScreenCast"
// #define SCREENCAST_OBJECT "/org/freedesktop/portal/desktop"
// #define SCREENCAST_INTERFACE "org.freedesktop.portal.ScreenCast"

#define APP_OBJECT_PATH "/site/brutt/CerveM"

// Function to handle D-Bus errors
void check_dbus_error(DBusError *err) {
    if (dbus_error_is_set(err)) {
        fprintf(stderr, "DBus Error: %s\n", err->message);
        dbus_error_free(err);
        exit(-1);
    }
}

// Function to start the screen cast session
void start_screencast(DBusConnection *conn, const char *session_handle) {
    DBusMessage *msg, *reply;
    DBusError err;
    dbus_error_init(&err);

    msg = dbus_message_new_method_call(SCREENCAST_SERVICE, SCREENCAST_OBJECT, SCREENCAST_INTERFACE, "SelectSources");
    if (!msg) {
        fprintf(stderr, "Message Null\n");
        return;
    }

    // Add session handle as the first argument
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    const char *variant_signature = "s";
    DBusMessageIter variant;
    dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, variant_signature, &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_OBJECT_PATH, &session_handle);
    dbus_message_iter_close_container(&args, &variant);

    // Add options as the second argument (could be empty for now)
    DBusMessageIter options;
    dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, "a{sv}", &options);
    dbus_message_iter_close_container(&args, &options);

    reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT_INFINITE, &err);
    check_dbus_error(&err);

    if (reply) {
        printf("ScreenCast session started.\n");
        dbus_message_unref(reply);
    } else {
        fprintf(stderr, "No reply received\n");
    }
    dbus_message_unref(msg);
}

// Function to create a session
char *create_session(DBusConnection *conn) {
    DBusMessage *msg, *reply;
    DBusError err;
    dbus_error_init(&err);

    msg = dbus_message_new_method_call("org.freedesktop.portal.Desktop",
                                           "/org/freedesktop/portal/desktop",
                                           "org.freedesktop.portal.Session",
                                           "CreateSession");
    if (!msg) {
        fprintf(stderr, "Message Null\n");
        return NULL;
    }

    // Add options as the first argument (should have the basic things)
    DBusMessageIter args;
    DBusMessageIter options, dict_iter, variant_iter;

    dbus_message_iter_init_append(msg, &args);

    // Create the options dictionary (a{sv})
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &options);

    // adding handle_token
    // Add handle_token to the dictionary
    const char *key_handle_token = "handle_token";
    const char *handle_token_value = APP_OBJECT_PATH;
    dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, NULL, &dict_iter);
    dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &key_handle_token);
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &handle_token_value);
    dbus_message_iter_close_container(&dict_iter, &variant_iter);
    dbus_message_iter_close_container(&options, &dict_iter);

    // Add session_handle_token to the dictionary
    const char *key_session_handle_token = "session_handle_token";
    const char *session_handle_token_value = APP_OBJECT_PATH;
    dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, NULL, &dict_iter);
    dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &key_session_handle_token);
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &session_handle_token_value);
    dbus_message_iter_close_container(&dict_iter, &variant_iter);
    dbus_message_iter_close_container(&options, &dict_iter);

    // closing the options
    dbus_message_iter_close_container(&args, &options);


    reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT_INFINITE, &err);
    printf("gothere\n");
    check_dbus_error(&err);

    if (reply) {
        DBusMessageIter reply_args;
        dbus_message_iter_init(reply, &reply_args);

        const char *session_handle;
        if (dbus_message_iter_get_arg_type(&reply_args) == DBUS_TYPE_OBJECT_PATH) {
            dbus_message_iter_get_basic(&reply_args, &session_handle);
            printf("Session created: %s\n", session_handle);
            char *session_handle_copy = strdup(session_handle);  // Copy the session handle
            dbus_message_unref(reply);
            dbus_message_unref(msg);
            return session_handle_copy;  // Return the copied session handle
        } else {
            fprintf(stderr, "Failed to get session handle\n");
        }
        dbus_message_unref(reply);
    } else {
        fprintf(stderr, "No reply received\n");
    }
    dbus_message_unref(msg);
    return NULL;
}

int main_() {
    DBusConnection *conn;
    DBusError err;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!conn) {
        fprintf(stderr, "Connection Null: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }

    char *session_handle = create_session(conn);
    if (session_handle) {
        start_screencast(conn, session_handle);
        free(session_handle);  // Free the session handle when done
    }

    dbus_connection_unref(conn);

    return 0;
}
