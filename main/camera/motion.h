#ifndef _MOTION_H_
#define _MOTION_H_

#include "camera_config.h"

typedef enum {
    RIGHT = 0,
    LEFT = 1,
    NO_MOTION = 0xFFFFFFFF
} direction_t;

typedef enum {
    UNCHANGED_BLOCK = 0,
    CHANGED_BLOCK = 0xFFFF,
    _SIZE_ENUM = 0xFFFFFFFF
} block_t;

typedef struct {
    direction_t previous;
    direction_t current;
} directions_st;

/**
 * CONFIG_BLOCK_SIZE:
 * Size of each sensor block on the display
 * (reduced granularity for speed).
 * Pixels per block side
 * For example, if \ref BLOCK_SIZE is 10,
 * the block will be 10x10 pixels
 * default CONFIG_BLOCK_SIZE ie 10
 */

/*
 * Width and Height in blocks
 * W = 640 / 10 = 64
 * H = 480 / 10 = 48
 */
#define W (CAM_WIDTH / CONFIG_BLOCK_SIZE)
#define H (CAM_HEIGHT / CONFIG_BLOCK_SIZE)

/* Thresholds -  convert to float */
#define BLOCK_DIFF_THRESHOLD CONFIG_BLOCK_DIFF_THRESHOLD / 100.0
#define IMAGE_DIFF_THRESHOLD CONFIG_BLOCK_DIFF_THRESHOLD / 100.0

/**
 * @brief  Capture image and do down-sampling
 * @param void
 * @return void
 */
bool capture_frame(void);

/**
 * @brief  Compute the number of different blocks
 * If there are enough, then motion happened
 * @param void
 * @return void
 */
direction_t motion_detect(void);

/**
 * @brief Copy current frame to previous
 * @param void
 * @return void
 */
void update_frame(void);


void update_frame_custom(void);

direction_t motion_detect_custom(void);

camera_fb_t *get_local_frame_buffer(void);

uint16_t *get_down_sample_frame_buffer(void);

#endif /* _MOTION_H_ */
