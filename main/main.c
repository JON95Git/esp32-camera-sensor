#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_camera.h"
#include "app_wifi.h"
#include "app_httpserver.h"

#ifdef CONFIG_LILYGO_CAM
#include "app_display.h"
#endif

static void app_create_threads(void);

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(100));
#ifdef CONFIG_LILYGO_CAM
    // app_display_init();
#endif
    app_camera_init();
    app_wifi_init();
    app_httpserver_init();
    app_create_threads();
}

static void app_create_threads(void)
{
#ifdef CONFIG_LILYGO_CAM
    // app_display_create_thread();
#endif
    // app_camera_create_thread();
}
