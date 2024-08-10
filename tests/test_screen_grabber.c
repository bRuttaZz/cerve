#include "../include/utils.h"
#include "../include/screen_cap.h"
#include <stdio.h>

void test_screen_grabber(enum ScreenGrabberType grabber) {
    // g_logger.level = 3
    int width = 1920;
    int height = 1080;
    char msg[200];
    const char out_frame_filename[] = "test.screen-shot.png";


    sprintf(msg, "testing SCREEN GRABBER 🮕 : %d", grabber);
    g_logger.info(msg);

    sprintf(msg, "initializing av encoder : WxH -> %dx%d", width, height);
    g_logger.info(msg);
    init_video_encoder(width, height);

    g_screen_grabber.type = grabber;

    g_logger.info("initializing scren grabber..");
    g_screen_grabber.setup_screen_cap(width, height);

    g_logger.info("capturing screen..");
    for (int i=0; i<3; i++){
        g_screen_grabber.capture_screen();
    }

    sprintf(msg, "saving last captured frame : %s", out_frame_filename);
    g_logger.info(msg);
    int resp = test_save_av_frame(g_avframe, out_frame_filename);
    if (resp) {
        sprintf(msg, "error saving captured frame : %s", out_frame_filename);
        g_logger.error(msg);

    }

    g_logger.info("garbage collecting screen capture..");
    g_screen_grabber.cleanup_screen_cap();

    g_logger.info("garbage collecting video encoder..");
    dest_video_encoder();

    g_logger.info("SCREEN GRABBER ✅\n\n");
}
