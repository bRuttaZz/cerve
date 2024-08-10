// here we will grab some wayland

#include "../../../include/utils.h"
#include "../../../include/screen_cap.h"
#include "../../../include/screen_grabbers.h"
#include "libavutil/frame.h"
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


struct {
    // global wayland objects
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    uint32_t wl_registry_id;
    struct wl_compositor *wl_compositor;
    struct wl_shm *wl_shm;
    uint32_t wl_shm_id;
    struct wl_surface *wl_surface;
    struct wl_buffer *wl_buffer;

    // some calculation params
    int frame_height;
    int frame_width;
    int frame_stride;
    int frame_size;
    int frame_shm_fd;
    struct wl_shm_pool *shm_pool;
    uint32_t *wayland_data;         // wl memory mapped place to store data

    int is_format_supported;
} typedef WayLandFrameCaptureContext ;


// global wayland client session object
WayLandFrameCaptureContext g_wayland_context = {.is_format_supported=0, .wl_shm_id=99999, .wl_registry_id=99999};


// wayland registry handlers
static void _registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void _registry_remover(void *data, struct wl_registry *registry, uint32_t id);
static void _shm_format_handler(void *data, struct wl_shm *wl_shm, uint32_t format);

static const struct wl_registry_listener registry_listener = {
    .global=_registry_handler,
    .global_remove=_registry_remover
};

static struct wl_shm_listener shm_listener = {
    .format = _shm_format_handler
};

// Callback functions for registry events
// registry handler
static void _registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        g_wayland_context.wl_compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        g_wayland_context.wl_shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        g_wayland_context.wl_shm_id = id;
    }
}
static void _registry_remover(void *data, struct wl_registry *registry, uint32_t id) {
    // do garbage collection things
    if (id == g_wayland_context.wl_registry_id || id == g_wayland_context.wl_shm_id) {
        g_logger.error("[wayland_screen_grabber] wayland screen capture interrupted!");
        exit(-1);
    }
}
// Callback functions for wl_shm events
static void _shm_format_handler(void *data, struct wl_shm *wl_shm, uint32_t format) {
    if (WL_SHM_FORMAT_ARGB8888 == format)
        g_wayland_context.is_format_supported = 1;
}




// setup wayland screen capture
void wayland_setup_screen_cap(const int width, const int height) {

    g_logger.info("Initializing Wayland screen grabber..");

    // To initialize wayland client (binding registry)
    errno = 0;
    g_wayland_context.wl_display = wl_display_connect(NULL);
    if (g_wayland_context.wl_display == NULL) {
        char err[200];
        sprintf(err, "[wayland_screen_grabber] Error connecting to Wayland socket! [errno: %d]\n", errno);
        g_logger.error(err);
        exit(-1);
    }
    g_wayland_context.wl_registry = wl_display_get_registry(g_wayland_context.wl_display);
    wl_registry_add_listener(g_wayland_context.wl_registry, &registry_listener, NULL);
    wl_display_roundtrip(g_wayland_context.wl_display);
    wl_shm_add_listener(g_wayland_context.wl_shm, &shm_listener, NULL);
    wl_display_roundtrip(g_wayland_context.wl_display);
    if (!g_wayland_context.is_format_supported) {
        g_logger.error("ARGB is not supported by wayland compositor! (currently I'm too lazy to support other pix_fmts");
        wayland_cleanup_screen_cap();
        exit(-1);
    }
    shm_listener.format = NULL;

    // setting up other listeners
    g_wayland_context.frame_height = height;
    g_wayland_context.frame_width = width;
    g_wayland_context.frame_stride = width * DEF_VIDEO_STRIDE_CHANNELS;
    g_wayland_context.frame_size = g_wayland_context.frame_stride * height;

    // create an shared file descriptor (to work with wayland)
    g_wayland_context.frame_shm_fd = memfd_create("wayland-shm", MFD_ALLOW_SEALING);
    if (g_wayland_context.frame_shm_fd == -1) {
        g_logger.error("[wayland_screen_grabber] memfd_create failed!");
        exit(-1);
    }
    if (ftruncate(g_wayland_context.frame_shm_fd, g_wayland_context.frame_size) == -1) {
        g_logger.error("[wayland_screen_grabber] ftruncate failed");
        close(g_wayland_context.frame_shm_fd);
        exit(-1);
    }

    // map memory buffer to that shared memory fd
    g_wayland_context.wayland_data = mmap(
        NULL, g_wayland_context.frame_size, PROT_READ | PROT_WRITE, MAP_SHARED,
        g_wayland_context.frame_shm_fd, 0
    );
    if (g_wayland_context.wayland_data == MAP_FAILED) {
        g_logger.error("[wayland_screen_grabber] error creating wayland databuffer");
        close(g_wayland_context.frame_shm_fd);
        exit(-1);
    }

    // binding wl_shm with newly created shm_fd
    g_wayland_context.shm_pool = wl_shm_create_pool(
        g_wayland_context.wl_shm, g_wayland_context.frame_shm_fd, g_wayland_context.frame_size
    );

    // use that binding to create wl_buffer
    g_wayland_context.wl_buffer = wl_shm_pool_create_buffer(g_wayland_context.shm_pool, 0, width,
        height, g_wayland_context.frame_stride, WL_SHM_FORMAT_ARGB8888);

    // creating sruface
    g_wayland_context.wl_surface = wl_compositor_create_surface(g_wayland_context.wl_compositor);
    if (g_wayland_context.wl_surface <=0) {
        g_logger.error("Failed to create Wayland surface");
        exit(-1);
    }
    g_logger.info("WayLand screen grabber initialized successfully!");
}

// capture wayland screen
void wayland_capture_screen() {
    wl_surface_attach(g_wayland_context.wl_surface, g_wayland_context.wl_buffer, 0, 0);
    wl_surface_damage(g_wayland_context.wl_surface, 0, 0,
        g_wayland_context.frame_width, g_wayland_context.frame_height);
    wl_surface_commit(g_wayland_context.wl_surface);
    // waiting for the frame to be rendered
    wl_display_roundtrip(g_wayland_context.wl_display);

    //========================================================================================================
    // for testing
    // printf("saving to file");
    // FILE *file = fopen("test.inter.png", "wb");
    // if (!file) {
    //     perror("Failed to open file for writing");
    //     return;
    // }

    // size_t written = fwrite(g_wayland_context.wayland_data, 1, g_wayland_context.frame_size, file);
    // if (written != g_wayland_context.frame_size) {
    //     perror("Failed to write all data to file");
    // }

    // fclose(file);
    // printf("Dumped raw memory to %s\n", "test.inter.png");

    for (int i = 0; i < 5; i++) { // Log first 10 pixels
        uint32_t pixel = ((uint32_t *)g_wayland_context.wayland_data)[i];
        printf("Pixel %d: 0x%08x\n", i, pixel);
    }
    //========================================================================================================


    // Copy the memory from the shared memory to the AVFrame
    for (int y = 0; y < g_wayland_context.frame_height; y++) {
        memcpy(g_avframe->data[0] + y * g_avframe->linesize[0],
                (uint8_t *)g_wayland_context.wayland_data + y * g_wayland_context.frame_stride,
                g_wayland_context.frame_stride
        );
    }
}

// cleanup wayland capture session
void wayland_cleanup_screen_cap() {
    g_logger.debug("cleaning up wayland registry garbage..");
    wl_buffer_destroy(g_wayland_context.wl_buffer);
    wl_shm_pool_destroy(g_wayland_context.shm_pool);
    munmap(g_wayland_context.wayland_data, g_wayland_context.frame_size);
    close(g_wayland_context.frame_shm_fd);
    wl_surface_destroy(g_wayland_context.wl_surface);
    wl_display_disconnect(g_wayland_context.wl_display);
    g_logger.debug("wayland session closed successfully!");
}

















// AVCodecContext *codec_context;
// AVFrame *frame;
// AVPacket *packet;

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

// void encode_av1_frame(uint8_t *data) {
//     // Convert the Wayland raw image data (BGRA) to YUV420P
//     // Populate the `frame` with converted data

//     int ret = avcodec_send_frame(codec_context, frame);
//     if (ret < 0) {
//         fprintf(stderr, "Error sending a frame for encoding\n");
//         exit(1);
//     }

//     while (ret >= 0) {
//         ret = avcodec_receive_packet(codec_context, packet);
//         if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//             break;
//         } else if (ret < 0) {
//             fprintf(stderr, "Error during encoding\n");
//             exit(1);
//         }

//         // Send the packet data over HTTP
//         // Example: send(client_socket, packet->data, packet->size, 0);

//         av_packet_unref(packet);
//     }
// }
