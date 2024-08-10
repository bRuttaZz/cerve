
#ifndef SCREEN_CAP_H
#define SCREEN_CAP_H

#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>

#define DEF_VIDEO_FRAMERATE  24
#define DEF_VIDEO_BITRATE  1000000
// Group of Pictures size) specifies the maximum number of frames between keyframes (I-frames) in a video stream.
// longer GOP -> less i-frames -> more compression -> more cpu
#define DEF_VIDEO_GOP_SIZE 10
// max_b_frames specifies the maximum number of consecutive B-frames (bidirectionally predicted frames) that can be used in a video sequence.
// more b frames -> more compression and predictiveness -> more cpu
#define DEF_VIDEO_MAX_B_FRAMES 1
#define DEF_VIDEO_PIX_FMT AV_PIX_FMT_ARGB   // (argb888) for the time being setting a standard


struct {
    int width;
    int height;
    int framerate;
    int bitrate;
    int gop_size;
    int max_b_frames;
    int pix_fmt;
} typedef VideoParams ;

extern VideoParams g_video_params;

extern AVCodecContext *g_codec_context; // ffmpg codec context
extern AVFrame *g_avframe;              // current ffmpeg avframe
extern AVPacket *g_avpacket;            // current ffmpeg video packet

// video encoder

/**
@brief initialize video encoder parameters
@param width - width of video
@param height - height of video
*/
void init_video_encoder(const int width, const int height);

/**
@brief garbage collection for video encoder init
*/
void dest_video_encoder();


// Screen grabber
// screen grabber platform choices
enum ScreenGrabberType {
    SCREEN_GRABBER_WAYLAND,
    // more will come hopefully

    SCREEN_GRABBER_MAX_VALUE,
};

// screen grabber factory methods (to give support for multiple screen grabber things like X11)

typedef void  (* Screen_Grabber_Setup_Screen_Cap) (const int width, const int height);     // to init screen cap  things
typedef void (* Screen_Grabber_Capture_Screen) (void);                      // to repeatedly capture new frames
typedef void (* Screen_Grabber_Cleanup_Screen_Cap) (void);                  // to cleanup garbage


struct {
    enum ScreenGrabberType type;
    enum ScreenGrabberType configured_screen_cap;           // get set on calling initializer
    Screen_Grabber_Setup_Screen_Cap setup_screen_cap;       // initializer
    Screen_Grabber_Capture_Screen capture_screen;           // doer (wont work without calling initializer)
    Screen_Grabber_Cleanup_Screen_Cap cleanup_screen_cap;   // gc

} typedef ScreenGrabber;

// global screen grabber
extern ScreenGrabber g_screen_grabber;



#endif /* SCREEN_CAP_H */
