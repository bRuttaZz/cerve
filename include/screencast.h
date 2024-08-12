#ifndef SCREENCAST_H
#define SCREENCAST_H

#include "glib.h"
#include <gio/gio.h>
// #include <stdio.h>

// implements screencast using freedesktop's desktop portals

// desktop portal params
#define SC_BUS_NAME "org.freedesktop.portal.Desktop"
#define SC_BUS_OBJECT_PATH "/org/freedesktop/portal/desktop"
// interfaces
#define SC_BUS_IF_REQUEST "org.freedesktop.portal.Request"
#define SC_BUS_IF_SCREENCAST "org.freedesktop.portal.ScreenCast"

// helper constants
#define SC_BUS_TOKEN_FMT_STRING "u%d"
#define SC_BUS_PATH_FMT_STRING_R "/org/freedesktop/portal/desktop/request/%s/%s"
#define SC_BUS_PATH_FMT_STRING_S "/org/freedesktop/portal/desktop/session/%s/%s"

// dbus connection handles
struct {
    GDBusConnection *conn;
    GMainLoop *loop;
    char *sender_name;
    char *session_handle;
    unsigned int event_subs_id;
    guint32 pipewire_fd;

    int req_token_counter;
    int ses_token_counter;          // lemme make it clear (ses -> session)

    char *event_lookup_path;
} typedef _DbusConnectionHandle;


// dbus management methods
int sc_make_dbus_connection(_DbusConnectionHandle *);
void sc_close_dbus_connection(_DbusConnectionHandle *);

// screen cast request
int sc_request_screen_cast(_DbusConnectionHandle *);


// dbus helper methods

enum _DbusReqType {
  SC_DBUS_REQ_TYPE_REQUEST,
  SC_DBUS_REQ_TYPE_SESSION
};
/**
method to generate request path. provide correctly sized res_token and res_path
*/
void sc_get_new_dbus_req_path (_DbusConnectionHandle*, enum _DbusReqType, char* res_token, char* res_path);


#endif /** SCREENCAST_H */
