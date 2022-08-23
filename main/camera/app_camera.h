#ifndef _APP_CAMERA_H_
#define _APP_CAMERA_H_

void app_camera_init(void);
void app_camera_run(void *pvArgs);
void app_camera_create_thread(void);

int get_people_counter(void);
void get_camera_status(char *buffer, uint16_t buff_len);

#endif
