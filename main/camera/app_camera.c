#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_camera.h"
#include "app_camera.h"
#include "camera_config.h"
#include "motion.h"

static const char *TAG = "app_camera";

/** This struct allows us to select only one direction
 * when sereval directions are detected for just one person
 * entering or getting out of the building
 */
static directions_st directions = {NO_MOTION, NO_MOTION};
static int counter = -1; 

static void config_gpio_camera(void)
{
    /* IO13, IO14 is designed for JTAG by default,
     * to use it as generalized input,
     * firstly declair it as pullup input */
    gpio_config_t conf;
    conf.mode = GPIO_MODE_INPUT;
    conf.pull_up_en = GPIO_PULLUP_ENABLE;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.pin_bit_mask = 1LL << 13;
    gpio_config(&conf);
    conf.pin_bit_mask = 1LL << 14;
    gpio_config(&conf);
}

static camera_config_t get_camera_config(void)
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.xclk_freq_hz = XCLK_FREQ;
    config.pixel_format = CAMERA_PIXEL_FORMAT;
    config.frame_size = CAMERA_FRAME_SIZE;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    return config;
}

/**
 * @brief  Initialize camera hardware
 * @param void
 * @return void
 */
static void hardware_camera_init(void)
{
    config_gpio_camera();

    camera_config_t config = get_camera_config();

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_framesize(sensor, CAMERA_FRAME_SIZE);
    sensor->set_vflip(sensor, 1);
}

/**
 * @brief  Count detected motion
 * @param motion_detection RIGHT or LEFT
 * @return void
 */
static void count_detected_motion(direction_t motion_detection)
{
    switch(motion_detection) {
    case LEFT:
        /* When a person gets in that means that he is coming from the left side
         * (the motion is detected from the left side of the photo)
         */
        printf("Get in\n");
        directions.current = LEFT;
        break; 
    case RIGHT: 
        /* When a person gets out that means that he is coming from the right side
         * (the motion is detected from the right side of the photo)
         */
        printf("Get out\n");
        directions.current = RIGHT;
        break;
    default:
        directions.current = NO_MOTION;
        break;
    }

    /* 
     * If two directions or more are detected simultaneously (within milliseconds)
     * only the first one will be selected
     */
    if ((directions.previous == NO_MOTION) && (directions.current == LEFT)) {
        /* If the person is getting in the building +1 is added */
        counter++;
    } else if ((directions.previous == NO_MOTION) && (directions.current == RIGHT)) {
        /* If the person is getting out the building 1 is substracted */
        counter--;
    }
    printf("counter: %d\n", counter);

    /* The current value will be the preview value */
    directions.previous = directions.current;
}

static void app_camera_loop(void)
{
    if (!capture_frame()) {
        printf("Failed capture\n");
        return;
    }
    // count_detected_motion(motion_detect());
    motion_detect_custom();
    update_frame_custom();
}

void app_camera_init(void)
{
    hardware_camera_init();
    printf("Camera init finished\n");
}

void app_camera_create_thread(void)
{
    xTaskCreatePinnedToCore(app_camera_run, "camera", 4096, NULL, 1, NULL, tskNO_AFFINITY);
}

void app_camera_run(void *pvArgs)
{
    while (1) {
        app_camera_loop();
    }
}

int get_people_counter(void)
{
    return counter;
}

void get_camera_status(char *buffer, uint16_t buff_len)
{
    sensor_t *s = esp_camera_sensor_get();
    char *p = buffer;
    *p++ = '{';
    p += snprintf(p, buff_len, "\"framesize\":%u,", s->status.framesize);
    p += snprintf(p, buff_len, "\"quality\":%u,", s->status.quality);
    p += snprintf(p, buff_len, "\"brightness\":%d,", s->status.brightness);
    p += snprintf(p, buff_len, "\"contrast\":%d,", s->status.contrast);
    p += snprintf(p, buff_len, "\"saturation\":%d,", s->status.saturation);
    p += snprintf(p, buff_len, "\"sharpness\":%d,", s->status.sharpness);
    p += snprintf(p, buff_len, "\"special_effect\":%u,", s->status.special_effect);
    p += snprintf(p, buff_len, "\"wb_mode\":%u,", s->status.wb_mode);
    *p++ = '}';
    *p++ = 0;
}
