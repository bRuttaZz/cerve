#include "../include/utils.h"
#include "./tests.h"



int main() {

    g_logger.info("[TEST] testing SERVER CONSTRUCT 🮕\n");
    test_server_constructor();
    g_logger.info("[TEST] SERVER CONSTRUCT ✅\n\n");

    g_logger.info("[TEST] testing DEBUS CONNECTION MAKER 🮕\n");
    test_dbus_connection_maker();
    g_logger.info("[TEST] DEBUS CONNECTION MAKER ✅\n\n");


    return 0;
}
