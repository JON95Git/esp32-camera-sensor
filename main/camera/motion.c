#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_camera.h"
#include "motion.h"

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

static uint16_t direc_left[H][W] = {0};
static uint16_t direc_right[H][W] = {0};
static uint16_t direc[H][W] = {0};

static uint16_t prev_frame[H][W] = {0};
static uint16_t current_frame[H][W] = {0};

/* CUSTOM BUFFERS */
#define UINT_CUSTOM_T uint32_t

static UINT_CUSTOM_T *current_frame_custom_ptr = NULL;
static UINT_CUSTOM_T previous_frame_custom[GRID_H][GRID_W] = {0};
static UINT_CUSTOM_T current_frame_custom[GRID_H][GRID_W] = {0};
static UINT_CUSTOM_T changed_frame_custom[GRID_H][GRID_W] = {0};

/* TZ - transition zone indexes */
#define TZ_INDEX_HIGH ((GRID_H/2) - 1)
#define TZ_INDEX_LOW  ((GRID_H/2))

static camera_fb_t *local_frame_buffer_ptr = NULL;
static camera_fb_t local_frame_buffer;

static direction_t direction_detect(uint16_t frame[H][W]);
// static void print_frame(uint16_t frame[H][W]);

/**
 * @brief Print frame buffer for debugging purpose
 * @param frame
 * @return void
 */
static void print_frame(UINT_CUSTOM_T frame[GRID_H][GRID_W], const char *frame_name)
{
    printf("\n%s \n\n", frame_name);
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            if ((y == TZ_INDEX_HIGH) || (y == TZ_INDEX_LOW)) {
                printf(RED "%.4X " RESET, frame[y][x]);
                // printf("\t");
            } else if (frame[y][x] == CHANGED_BLOCK){
                printf(BLU "%.4X " RESET, frame[y][x]);
                // printf("\t");
            } else {
                printf("%.4X ", frame[y][x]);
            }
        }
        printf("\n");
    }
    printf("\n======================================\n\n");
}

/**
 * @brief  Calculates the frequency of a number in a HxW matrix
 * @param matrix matrix to be checked
 * @param num number to be found in the given matrix
 * @return Frequency
 */
static int get_num_freq_matrix(uint16_t matrix[H][W], uint16_t num)
{
   int freq = 0 ; 
   for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (matrix[y][x] == num) {
                freq++;
            }
        }
    }
    return freq;
}

/**
 * @brief Set a buffer to zero
 * @param buffer pointer to buffer
 * @param buff_len buffer size
 */
static void cleanup_buffer(uint16_t buffer[H][W], uint16_t buff_len)
{
    memset(buffer, 0, buff_len);
}

/**
 * @brief Set a buffer to zero
 * @param buffer pointer to buffer
 * @param buff_len buffer size
 */
static void cleanup_buffer_custom(UINT_CUSTOM_T buffer[GRID_H][GRID_W], uint16_t buff_len)
{
    memset(buffer, 0, buff_len);
}


/**
 * @brief  Down-sample image in blocks
 * (Down sample, acumulate and rescale image) and
 * save in \ref current_frame
 * @param frame_buffer double pointer to frame buffer
 * @return void
 */
static void down_sample(camera_fb_t **frame_buffer)
{
    cleanup_buffer(current_frame, sizeof(current_frame));

    /* Down-sample image in blocks */
    for (uint32_t i = 0; i < CAM_WIDTH * CAM_HEIGHT; i++) {
        const uint16_t x = i % CAM_WIDTH;
        const uint16_t y = floor(i / CAM_WIDTH);
        const uint8_t block_x = floor(x / CONFIG_BLOCK_SIZE);
        const uint8_t block_y = floor(y / CONFIG_BLOCK_SIZE);
        const uint8_t pixel = (*frame_buffer)->buf[i];

        /* average pixels in block (accumulate) */
        current_frame[block_y][block_x] += pixel;
    }

    /* average pixels in block (rescale) */
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            current_frame[y][x] /= CONFIG_BLOCK_SIZE * CONFIG_BLOCK_SIZE;
        }
    }
}

/**
 * @brief  Down-sample image in blocks (custom version)
 * (Down sample, acumulate and rescale image) and
 * save in \ref current_frame
 * @param frame_buffer double pointer to frame buffer
 * @return void
 */
static void down_sample_custom(camera_fb_t **frame_buffer)
{
    cleanup_buffer_custom(current_frame_custom, sizeof(current_frame_custom));

    /* H = x
     * W = y
     */
    /* Down-sample image in blocks */
    for (uint32_t i = 0; i < CAM_WIDTH * CAM_HEIGHT; i++) {
        const uint16_t x = i % CAM_WIDTH;
        /* floor() rounds down a number */
        const uint16_t y = floor(i / CAM_WIDTH);
        const uint8_t block_x = floor(x / BLOCK_SIZE_H);
        const uint8_t block_y = floor(y / BLOCK_SIZE_W);
        const uint8_t pixel = (*frame_buffer)->buf[i];

        /* average pixels in block (accumulate) */
        current_frame_custom[block_y][block_x] += pixel;
    }

    /* average pixels in block (rescale) */
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            current_frame_custom[y][x] /= BLOCK_SIZE_W * BLOCK_SIZE_H;
        }
    }

    print_frame(current_frame_custom, "CURRENT FRAME");
}

/**
 * @brief This function is used to determine the direction of the movement
 * by dividing the direction matrix calculated in \ref motion_detect()
 * function into one right side matrix and one left side matrix,
 * the matrix with the higher frequency in \ref CHANGED_BLOCK ("99" value)
 * shows the direction rom which the movement is coming.
 * @param frame buffer that holds frame information
 * @return Direction RIGHT or LEFT
 */
static direction_t direction_detect(uint16_t frame[H][W])
{
    int freq_right = 0;
    int freq_left = 0;

    cleanup_buffer(direc_left, sizeof(direc_left));
    cleanup_buffer(direc_right, sizeof(direc_left));

    /* Determine the direction of the movement
     * by dividing the direction matrix calculated in \ref motion_detect()
     * function into one right side matrix and one left side matrix
     */
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (x < (W / 2)) {
                direc_left[y][x] = frame[y][x];
                direc_right[y][x] = 0;
            }  else {
                direc_right[y][x] = frame[y][x];
                direc_left[y][x] = 0;
            }
        }
    }

    freq_right = get_num_freq_matrix(direc_right, CHANGED_BLOCK);
    freq_left = get_num_freq_matrix(direc_left, CHANGED_BLOCK);

    if (freq_right > freq_left) {
        return RIGHT;
    } else if (freq_right < freq_left) {
        return LEFT;
    }

    return NO_MOTION;
}

camera_fb_t *get_local_frame_buffer(void)
{
    camera_fb_t *frame_buffer = esp_camera_fb_get();
    memcpy(&local_frame_buffer, frame_buffer, sizeof(camera_fb_t));
    esp_camera_fb_return(frame_buffer);
    local_frame_buffer_ptr = &local_frame_buffer;
    return local_frame_buffer_ptr;
}

uint16_t *get_down_sample_frame_buffer(void)
{
    current_frame_custom_ptr = &current_frame_custom;
    return current_frame_custom_ptr;
}

/**
 * @brief  Capture image and do down-sampling
 * @param void
 * @return void
 */
bool capture_frame(void)
{
    camera_fb_t *frame_buffer = esp_camera_fb_get();
    if (!frame_buffer) {
        return false;
    }
    down_sample_custom(&frame_buffer);
    esp_camera_fb_return(frame_buffer);
    return true;
}

/**
 * @brief  Compute the number of different blocks
 * If there are enough, then motion happened
 * @param void
 * @return Direction RIGHT or LEFT
 */
direction_t motion_detect(void)
{
    uint16_t changes = 0;
    const uint16_t blocks = (CAM_WIDTH * CAM_HEIGHT) / (CONFIG_BLOCK_SIZE * CONFIG_BLOCK_SIZE);

    cleanup_buffer(direc, sizeof(direc));

    /*
     * Compare the blocks of the current frame
     * with the blocks of the previous frame and
     * calculate the delta factor
     */
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float current = current_frame[y][x];
            float prev = prev_frame[y][x];
            float delta = fabs(current - prev) / prev;

            if (delta >= BLOCK_DIFF_THRESHOLD) {
                changes += 1;
                /* write "99" (CHANGED_BLOCK) in the direction
                 * matrix refering to the changed block
                 */
                direc[y][x] = CHANGED_BLOCK;
            }
        }
    }

    /* If the changes are greater than diff threshold
     * the motion happened
     */
    if ((1.0 * changes / blocks) > IMAGE_DIFF_THRESHOLD) {
        return direction_detect(direc);
    }

    return NO_MOTION;
}

/**
 * @brief  Compute the number of different blocks
 * If there are enough, then motion happened
 * @param void
 * @return Direction RIGHT or LEFT
 */
direction_t motion_detect_custom(void)
{
    uint16_t changes = 0;
    const uint16_t blocks = (CAM_WIDTH * CAM_HEIGHT) / (BLOCK_SIZE_W * BLOCK_SIZE_H);

    cleanup_buffer_custom(changed_frame_custom, sizeof(changed_frame_custom));

    /*
     * Compare the blocks of the current frame
     * with the blocks of the previous frame and
     * calculate the delta factor
     */
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            float current = current_frame_custom[y][x];
            float prev = previous_frame_custom[y][x];
            float delta = fabs(current - prev) / prev;

            if (delta >= BLOCK_DIFF_THRESHOLD) {
                changes += 1;
                /* write "99" (CHANGED_BLOCK) in the direction
                 * matrix refering to the changed block
                 */
                changed_frame_custom[y][x] = CHANGED_BLOCK;
            }
        }
    }

    /* If the changes are greater than diff threshold
     * the motion happened
     */
    print_frame(previous_frame_custom, "PREVIOUS FRAME");
    print_frame(changed_frame_custom, "CHANGED FRAME");

    printf("GRID_W: %d\n", GRID_W);
    printf("GRID_H: %d\n", GRID_H);
    printf("CAM_WIDTH: %d\n", CAM_WIDTH);
    printf("CAM_HEIGHT: %d\n", CAM_HEIGHT);
    printf("BLOCK_SIZE_W: %d\n", BLOCK_SIZE_W);
    printf("BLOCK_SIZE_H: %d\n", BLOCK_SIZE_H);
    printf("changes: %d\n", changes);
    printf("blocks: %d\n", blocks);
    printf("BLOCK_DIFF_THRESHOLD: %.2f\n", BLOCK_DIFF_THRESHOLD);
    printf("IMAGE_DIFF_THRESHOLD: %.2f\n", IMAGE_DIFF_THRESHOLD);

    if ((1.0 * changes / blocks) > IMAGE_DIFF_THRESHOLD) {
        // return direction_detect(direc);
        printf("ENOUGH CHANGES\n\n");
        return NO_MOTION;
    }

    printf("NOT ENOUGH CHANGES\n\n");
    return NO_MOTION;
}

/**
 * @brief Copy current frame to previous
 * @param void
 * @return void
 */
void update_frame(void)
{
    memcpy(prev_frame, current_frame, sizeof(prev_frame));
}

/**
 * @brief Copy current frame to previous
 * @param void
 * @return void
 */
void update_frame_custom(void)
{
    memcpy(previous_frame_custom, current_frame_custom, sizeof(previous_frame_custom));
}
