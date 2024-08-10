/**
header file to include all the available screen grabber factory compilent methods
*/

#ifndef SCREEN_GRABBERS_H
#define SCREEN_GRABBERS_H


// wayland grabber methods
void wayland_setup_screen_cap(const int width, const int height);
void wayland_capture_screen();
void wayland_cleanup_screen_cap();

// in future (and if everything works) other grabber instances can be filled over here


#endif /* SCREEN_GRABBERS_H */
