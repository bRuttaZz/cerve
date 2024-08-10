
#ifndef SCREEN_CAP_H
#define SCREEN_CAP_H

#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>


extern AVCodecContext *g_codec_context; // ffmpg codec context

extern uint32_t *g_raw_screen_cap;      // current raw screen
extern AVFrame *g_avframe;              // current ffmpeg avframe
extern AVPacket *g_avpacket;            // current ffmpeg video packet


// Screen grabber

// screen grabber platform choices
enum ScreenGrabberType {
    SCREEN_GRABBER_WAYLAND,
    // more will come hopefully
};

// screen grabber factory methods (to give support for multiple screen grabber things like X11)
struct {
    enum ScreenGrabberType type;
    void (* setup_screen_cap) (const int width, const int height);   // to initialize things
    void (* capture_screen) (void);                      // to repeatedly capture new frames
    void (* cleanup_screen_cap) (void);                  // to cleanup garbage

} typedef ScreenGrabber;

// global screen grabber
extern ScreenGrabber g_screen_grabber;



#endif /* SCREEN_CAP_H */
