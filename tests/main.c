#include "../include/screen_cap.h"


void test_server_constructor(void);
void test_screen_grabber(enum ScreenGrabberType grabber);

int main_() {
    test_server_constructor();
    test_screen_grabber(0);
    return 0;
}
