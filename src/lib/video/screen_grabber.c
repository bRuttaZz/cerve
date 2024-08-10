#include "../../../include/screen_cap.h"
#include "../../../include/screen_grabbers.h"
#include "../../../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

void _setup_screen_cap(const int width, const int height);
void _capture_screen();
void _cleanup_screen_cap();

// initiation with default value
ScreenGrabber g_screen_grabber = {
    .type=0,                                // initializing with default value (prefereably wayland)
    .configured_screen_cap=999,             // give some out of index value
    .setup_screen_cap=_setup_screen_cap,
    .capture_screen=_capture_screen,
    .cleanup_screen_cap=_cleanup_screen_cap,
};

// implementaion of look up methods

// the implementation array for easy lookup
Screen_Grabber_Setup_Screen_Cap _setup_screen_caps[] = {
    wayland_setup_screen_cap,
};
Screen_Grabber_Capture_Screen _capture_screens[] = {
    wayland_capture_screen,
};
Screen_Grabber_Cleanup_Screen_Cap _cleanup_screen_caps[] = {
    wayland_cleanup_screen_cap,
};

// common integritty tester
void _panic_not_implemented_error (char* prefix) {

}

// actual factory methods (i mean product methods)
// ensuring that the g_screen_grabber.type can be changed at runtime
void _setup_screen_cap(const int width, const int height) {
    if (g_screen_grabber.type < 0 || g_screen_grabber.type >= SCREEN_GRABBER_MAX_VALUE) {
        char err[200];
        sprintf(err, "[setup_screen_cap] Specified screen grabber not implemented! : '%d'\n",  g_screen_grabber.type);
        g_logger.error(err);
        exit(1);
    }
    // calling the implementation
    _setup_screen_caps[g_screen_grabber.type](width, height);
    g_screen_grabber.configured_screen_cap = g_screen_grabber.type;
}

void _capture_screen() {
    _capture_screens[g_screen_grabber.configured_screen_cap]();
}

void _cleanup_screen_cap() {
    _cleanup_screen_caps[g_screen_grabber.configured_screen_cap]();
    g_screen_grabber.configured_screen_cap = 999;
}
