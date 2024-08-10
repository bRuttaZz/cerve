#include "../../../include/utils.h"
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>


/**
Function to convert AVFrame from one color format to aother
return zero on success, otherwise return error

NB: should set out_frame->format before hand

*/
int convert_avframe_format(AVFrame *input_frame, AVFrame *output_frame) {
    struct SwsContext *sws_ctx;
    int width = input_frame->width;
    int height = input_frame->height;
    int ret;

    // Set output frame parameters
    output_frame->width = width;
    output_frame->height = height;

    // Allocate buffer for output frame
    ret = av_image_alloc(output_frame->data, output_frame->linesize, width, height, output_frame->format, 32);
    if (ret < 0) {
        g_logger.error("[convert_argb_to_rgba] could not allocate RGBA image\n");
        av_frame_free(&output_frame);
        return -1;
    }
    // Set up the scaling context
    sws_ctx = sws_getContext(width, height, input_frame->format,
                             width, height, output_frame->format,
                             SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        g_logger.error("[convert_argb_to_rgba] could not initialize scaling context\n");
        av_freep(&output_frame->data[0]);
        av_frame_free(&output_frame);
        return -1;
    }
    // Perform the conversion
    sws_scale(sws_ctx, (const uint8_t * const *)input_frame->data, input_frame->linesize,
              0, height, output_frame->data, output_frame->linesize);

    sws_freeContext(sws_ctx);
    return 0;
}

/**
@brief save avframe (for testing)
@param frame - avframe to be saved as webp
@param filename - path to pnf file
*/
int test_save_av_frame(AVFrame *input_frame, const char* filename) {
    // Open the codec
    int ret;
    FILE *file;
    AVPacket *pkt;
    AVCodecContext *codec_ctx;
    AVFrame *rgba888_frame;
    int new_frame = 0;

    // check and convert frame if needed
    if (input_frame->format != AV_PIX_FMT_RGBA) {
        // Allocate the output frame
        rgba888_frame = av_frame_alloc();
        if (!rgba888_frame) {
            g_logger.error("[test_save_av_frame] could not allocate output new frame for convertion");
            return -1;
        }
        rgba888_frame->format = AV_PIX_FMT_RGBA;
        if (convert_avframe_format(input_frame, rgba888_frame)){
            g_logger.error("[test_save_av_frame] error converting argb to rgb frame");
            av_frame_unref(rgba888_frame);
            return -1;
        };
        new_frame = 1;
    } else {
        rgba888_frame = input_frame;
    }

    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!codec) {
        g_logger.error("[test_save_av_frame] PNG codec not found");
        if (new_frame) av_frame_unref(rgba888_frame);
        return -1;
    }
    // Allocate codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        g_logger.error("[test_save_av_frame]Could not allocate video codec context\n");
        if (new_frame) av_frame_unref(rgba888_frame);
        return -1;
    }

    // Set codec parameters
    codec_ctx->width = rgba888_frame->width;
    codec_ctx->height = rgba888_frame->height;
    codec_ctx->pix_fmt = rgba888_frame->format;
    codec_ctx->time_base = (AVRational){1, 25};
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        g_logger.error("[test_save_av_frame] could not open codec");
        if (new_frame) av_frame_unref(rgba888_frame);
        return -1;
    }

    // Allocate the packet for encoding
    pkt = av_packet_alloc();
    pkt->data = NULL; // packet data will be allocated by the encoder
    pkt->size = 0;

    // Encode the image
    ret = avcodec_send_frame(codec_ctx, rgba888_frame);
    if (ret < 0) {
        g_logger.error("[test_save_av_frame] error sending frame to encoder");
        avcodec_free_context(&codec_ctx);
        if (new_frame) av_frame_unref(rgba888_frame);
        return -1;
    }

    ret = avcodec_receive_packet(codec_ctx, pkt);
    if (ret < 0) {
        g_logger.error("[test_save_av_frame] error receiving packet from encoder\n");
        avcodec_free_context(&codec_ctx);
        if (new_frame) av_frame_unref(rgba888_frame);
        return -1;
    }

    file = fopen(filename, "wb");
    if (!file) {
        char msg[100];
        sprintf(msg, "[test_save_av_frame] could not open %s\n", filename);
        g_logger.error(msg);
        av_packet_unref(pkt);
        avcodec_free_context(&codec_ctx);
        if (new_frame) av_frame_unref(rgba888_frame);
        return -1;
    }
    fwrite(pkt->data, 1, pkt->size, file);
    fclose(file);

    // Clean up
    av_packet_unref(pkt);
    avcodec_free_context(&codec_ctx);
    if (new_frame) av_frame_unref(rgba888_frame);
    return 0;
}
