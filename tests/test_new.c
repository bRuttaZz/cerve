
#include <gio/gio.h>

int main() {
    GError *error = NULL;
    GDBusConnection *connection;
    // GVariant *result;

    // Connect to the session bus
    // connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    // if (error) {
    //     g_printerr("Error connecting to the bus: %s\n", error->message);
    //     g_error_free(error);
    //     return -1;
    // }


    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    // connection = g_application_get_dbus_connection (g_application_get_default ());
    g_dbus_connection_call_sync (connection,
                                "org.gnome.Shell.Screenshot",
                                "/org/gnome/Shell/Screenshot",
                                "org.gnome.Shell.Screenshot",
                               "Screenshot",
                               g_variant_new ("(bbs)",
                                   TRUE,    /* pointer */
                                    TRUE, /* flash */
                                    "/home/bruttazz/test.png,"),
                               NULL,
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);




    // // Call the Screenshot method on the org.gnome.Shell.Screenshot interface
    // result = g_dbus_connection_call_sync(connection,
    //                                      "org.gnome.Shell.Screenshot", // bus name
    //                                      "/org/gnome/Shell/Screenshot", // object path
    //                                      "org.gnome.Shell.Screenshot", // interface name
    //                                      "Screenshot", // method name
    //                                      g_variant_new("(bb)", TRUE, FALSE), // parameters (include cursor, include window borders)
    //                                      G_VARIANT_TYPE("(bs)"), // expected reply type
    //                                      G_DBUS_CALL_FLAGS_NONE,
    //                                      -1, // timeout (default)
    //                                      NULL, // cancellable
    //                                      &error);

    if (error) {
        g_printerr("Error calling method: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    // // Retrieve the result
    // gboolean success;
    // const gchar *filename;
    // g_variant_get(result, "(bs)", &success, &filename);

    // if (success) {
    //     g_print("Screenshot saved to %s\n", filename);
    // } else {
    //     g_print("Screenshot failed\n");
    // }

    // g_variant_unref(result);
    // g_object_unref(connection);

    return 0;
}
