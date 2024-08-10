#include "../../../include/screen_cap.h"
#include "../../../include/utils.h"
#include "libavutil/pixfmt.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdlib.h>

// void initialize_av1_encoder(int width, int height) {
//     avcodec_register_all();

//     AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AV1);
//     codec_context = avcodec_alloc_context3(codec);

//     codec_context->bit_rate = 400000;
//     codec_context->width = width;
//     codec_context->height = height;
//     codec_context->time_base = (AVRational){1, 30};
//     codec_context->framerate = (AVRational){30, 1};
//     codec_context->gop_size = 10;
//     codec_context->max_b_frames = 1;
//     codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

//     avcodec_open2(codec_context, codec, NULL);

//     frame = av_frame_alloc();
//     frame->format = codec_context->pix_fmt;
//     frame->width = codec_context->width;
//     frame->height = codec_context->height;
//     av_frame_get_buffer(frame, 0);

//     packet = av_packet_alloc();
// }

AVCodecContext *g_codec_context; // ffmpg codec context

AVFrame *g_avframe;              // current ffmpeg avframe
AVPacket *g_avpacket;            // current ffmpeg video packet

VideoParams g_video_params = {
    .framerate = DEF_VIDEO_FRAMERATE,
    .bitrate = DEF_VIDEO_BITRATE,
    .gop_size = DEF_VIDEO_GOP_SIZE,
    .max_b_frames = DEF_VIDEO_MAX_B_FRAMES,
    .pix_fmt = DEF_VIDEO_PIX_FMT,
};

void _initialize_av1_encoder(int width, int height) {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AV1);
    if (!codec) {
        g_logger.error("AV1 codec not found.\n");
        exit(-1);
    }
    // creating context
    g_codec_context = avcodec_alloc_context3(codec);
    if (!g_codec_context) {
        g_logger.error("Failed to allocate codec context.\n");
        exit(-1);
    }
}


/**
@brief initialize video encoder parameters
@param width - width of video
@param height - height of video
*/
void init_video_encoder(const int width, const int height) {
    g_video_params.width = width;
    g_video_params.height = height;

    // currently goinf with AV1
    g_logger.info("initializing video encoder : AV1");
    _initialize_av1_encoder(width, height);

    // setup encoding params
    g_logger.debug("setting up encoder params..");
    g_codec_context->bit_rate = g_video_params.bitrate;
    g_codec_context->width = g_video_params.width;
    g_codec_context->height = g_video_params.height;
    g_codec_context->time_base = (AVRational){1, g_video_params.framerate};
    g_codec_context->framerate = (AVRational){g_video_params.framerate, 1};
    g_codec_context->gop_size = g_video_params.gop_size;
    g_codec_context->max_b_frames = g_video_params.max_b_frames;
    g_codec_context->pix_fmt = g_video_params.pix_fmt; // argb888


    g_logger.debug("initializing global av frames and packets..");
    g_avframe = av_frame_alloc();
    g_avframe->format = g_codec_context->pix_fmt;
    g_avframe->width = g_codec_context->width;
    g_avframe->height = g_codec_context->height;
    av_frame_get_buffer(g_avframe, 0);

    g_avpacket = av_packet_alloc();
}

/**
@brief garbage collection for video encoder init
*/
void dest_video_encoder() {
    av_frame_free(&g_avframe);
    av_packet_unref(g_avpacket);
    avcodec_free_context(&g_codec_context);
}
