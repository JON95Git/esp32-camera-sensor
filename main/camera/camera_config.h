#ifndef _CAMERA_CONFIG_H_
#define _CAMERA_CONFIG_H_

/**
 * PIXFORMAT_RGB565,    // 2BPP/RGB565
 * PIXFORMAT_YUV422,    // 2BPP/YUV422
 * PIXFORMAT_GRAYSCALE, // 1BPP/GRAYSCALE
 * PIXFORMAT_JPEG,      // JPEG/COMPRESSED
 * PIXFORMAT_RGB888,    // 3BPP/RGB888
 */
/* Camera Pixel format parameters from menuconfig */
#ifdef CONFIG_PIXEL_FORMAT_RGB565
    #define CAMERA_PIXEL_FORMAT PIXFORMAT_RGB565
#elif CONFIG_PIXEL_FORMAT_YUV422
    #define CAMERA_PIXEL_FORMAT PIXFORMAT_YUV422
#elif CONFIG_PIXEL_FORMAT_GRAYSCALE
    #define CAMERA_PIXEL_FORMAT PIXFORMAT_GRAYSCALE
#elif CONFIG_PIXEL_FORMAT_JPEG
    #define CAMERA_PIXEL_FORMAT PIXFORMAT_JPEG
#elif CONFIG_PIXEL_FORMAT_RGB888
    #define CAMERA_PIXEL_FORMAT PIXFORMAT_RGB888
#else
    #error Unsupported choice setting
#endif

/*
 * FRAMESIZE_QQVGA,    // 160x120
 * FRAMESIZE_QQVGA2,   // 128x160
 * FRAMESIZE_QCIF,     // 176x144
 * FRAMESIZE_HQVGA,    // 240x176
 * FRAMESIZE_QVGA,     // 320x240
 * FRAMESIZE_CIF,      // 400x296
 * FRAMESIZE_VGA,      // 640x480
 * FRAMESIZE_SVGA,     // 800x600
 * FRAMESIZE_XGA,      // 1024x768
 * FRAMESIZE_SXGA,     // 1280x1024
 * FRAMESIZE_UXGA,     // 1600x1200
 */
/* Camera Resolution parameters from menuconfig */
#ifdef CONFIG_FRAME_SIZE_QVGA
    #define CAMERA_FRAME_SIZE FRAMESIZE_QVGA
    #define CAM_WIDTH  320
    #define CAM_HEIGHT 240
    #define GRID_W 8
    #define GRID_H 10
#elif CONFIG_FRAME_SIZE_VGA
    #define CAMERA_FRAME_SIZE FRAMESIZE_VGA

    #define CAM_WIDTH  640
    #define CAM_HEIGHT 480

    #define GRID_W 16
    #define GRID_H 12

    #define BLOCK_SIZE_W (CAM_WIDTH/GRID_W)
    #define BLOCK_SIZE_H (CAM_HEIGHT/GRID_H)

#elif CONFIG_FRAME_SIZE_SVGA
    #define CAMERA_FRAME_SIZE FRAMESIZE_SVGA
    #define CAM_WIDTH  800
    #define CAM_HEIGHT 600
    #define GRID_W 10
    #define GRID_H 10
#elif CONFIG_FRAME_SIZE_XGA
    #define CAMERA_FRAME_SIZE FRAMESIZE_XGA
    #define CAM_WIDTH  1024
    #define CAM_HEIGHT 768
    #define GRID_W 16
    #define GRID_H 12
#elif CONFIG_FRAME_SIZE_UXGA
    #define CAMERA_FRAME_SIZE FRAMESIZE_UXGA
    #define CAM_WIDTH  1600
    #define CAM_HEIGHT 1200
    #define GRID_W 20
    #define GRID_H 16
#else
    #error Unsupported choice setting
#endif

/* LilyGo Cam with Mic v1.6.2 */
#ifdef CONFIG_LILYGO_CAM
    #define PWDN_GPIO_NUM     26
    #define RESET_GPIO_NUM    -1
    #define XCLK_GPIO_NUM     4
    #define SIOD_GPIO_NUM     18
    #define SIOC_GPIO_NUM     23

    #define Y9_GPIO_NUM       36
    #define Y8_GPIO_NUM       37
    #define Y7_GPIO_NUM       38
    #define Y6_GPIO_NUM       39
    #define Y5_GPIO_NUM       35
    #define Y4_GPIO_NUM       14
    #define Y3_GPIO_NUM       13
    #define Y2_GPIO_NUM       34
    #define VSYNC_GPIO_NUM    5
    #define HREF_GPIO_NUM     27
    #define PCLK_GPIO_NUM     25
    #define XCLK_FREQ         20000000

/* T-Camera Mini */
#elif CONFIG_T_CAMERA_MINI
    #define PWDN_GPIO_NUM    -1
    #define RESET_GPIO_NUM   -1
    #define XCLK_GPIO_NUM    32
    #define SIOD_GPIO_NUM    13
    #define SIOC_GPIO_NUM    12

    #define Y9_GPIO_NUM      39
    #define Y8_GPIO_NUM      36
    #define Y7_GPIO_NUM      38
    #define Y6_GPIO_NUM      37
    #define Y5_GPIO_NUM      15
    #define Y4_GPIO_NUM      4
    #define Y3_GPIO_NUM      14
    #define Y2_GPIO_NUM      5
    #define VSYNC_GPIO_NUM   27
    #define HREF_GPIO_NUM    25
    #define PCLK_GPIO_NUM    19
    #define XCLK_FREQ        20000000
#else
    #error Unsupported choice setting
#endif

#endif /* _CAMERA_CONFIG_H_ */
