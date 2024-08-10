#include "../../../include/screen_cap.h"
#include "../../../include/screen_grabbers.h"
#include "../../../include/utils.h"
#include <stdio.h>
#include <stdlib.h>

void _setup_screen_cap(const int width, const int height);
void _capture_screen();
void _cleanup_screen_cap();


// initiation with default value
ScreenGrabber g_screen_grabber = {
    .type=0,                                // initializing with default value (prefereably wayland)
    .setup_screen_cap=_setup_screen_cap,
    .capture_screen=_capture_screen,
    .cleanup_screen_cap=_cleanup_screen_cap,
};


void _panic_not_implemented_error (char* prefix) {
    char err[200];
    sprintf(err, "%s Specified screen grabber not implemented! : '%d'\n", prefix,  g_screen_grabber.type);
    g_logger.error(err);
    exit(1);
}


void _setup_screen_cap(const int width, const int height) {
    switch (g_screen_grabber.type) {
        case SCREEN_GRABBER_WAYLAND:
            wayland_setup_screen_cap(width, height);
            break;

        default:
            _panic_not_implemented_error("[setup_screen_cap]");
            break;
    }
}

// TODO has to implement some hashtable lookup or something (maybe) high utility fun
void _capture_screen() {
    switch (g_screen_grabber.type) {
        case SCREEN_GRABBER_WAYLAND:
            wayland_capture_screen();
            break;

        default:
            _panic_not_implemented_error("[capture_screen]");
            break;

    }
}

void _cleanup_screen_cap() {
    switch (g_screen_grabber.type) {
        case SCREEN_GRABBER_WAYLAND:
            wayland_cleanup_screen_cap();
            break;

        default:
            _panic_not_implemented_error("[cleanup_screen_cap]");
            break;
    }
}
