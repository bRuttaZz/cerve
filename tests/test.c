#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SESSION_INTERFACE "org.freedesktop.portal.Session"
#define SCREENCAST_SERVICE "org.freedesktop.portal.ScreenCast"
#define SCREENCAST_OBJECT "/org/freedesktop/portal/desktop"
#define SCREENCAST_INTERFACE "org.freedesktop.portal.ScreenCast"

// Function to handle D-Bus errors
void check_dbus_error(DBusError *err) {
    if (dbus_error_is_set(err)) {
        fprintf(stderr, "DBus Error: %s\n", err->message);
        dbus_error_free(err);
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

    msg = dbus_message_new_method_call(SCREENCAST_SERVICE, SCREENCAST_OBJECT, SCREENCAST_INTERFACE, "CreateSession");
    if (!msg) {
        fprintf(stderr, "Message Null\n");
        return NULL;
    }

    // Add options as the first argument (could be empty for now)
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    DBusMessageIter options;
    dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, "a{sv}", &options);
    dbus_message_iter_close_container(&args, &options);

    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
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

int main() {
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
