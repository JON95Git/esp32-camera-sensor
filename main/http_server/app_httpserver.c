#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include <esp_http_server.h>
#include "esp_timer.h"
#include "esp_camera.h"
#include "app_wifi.h"
#include "app_httpserver.h"
#include "app_camera.h"
#include "motion.h"

#define MAX_COUNTER_STRING_SIZE 20
#define JSON_RESPONSE_SIZE 1024

static const char *TAG = "httpSERVER";

static char json_response[JSON_RESPONSE_SIZE];

/* Camera HTTP stream parameters*/
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

/* HTML Page in binary format*/
extern const char page_start[] asm("_binary_index_html_start");
extern const char page_end[] asm("_binary_index_html_end");

extern const char js_start[] asm("_binary_index_js_start");
extern const char js_end[] asm("_binary_index_js_end");

/* WEB SOCKET */
/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

/*
 * async send function, which we put into the httpd work queue
 */
static void ws_async_send(void *arg)
{
    static const char * data = "Async data";
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    return httpd_queue_work(handle, ws_async_send, resp_arg);
}

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
static esp_err_t echo_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
        strcmp((char*)ws_pkt.payload,"Trigger async") == 0) {
        free(buf);
        return trigger_async_send(req->handle, req);
    }

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
    free(buf);
    return ret;
}

static const httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = echo_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};




esp_err_t show_all_uris_handler(httpd_req_t *req)
{
    /* Send a simple response */
    char resp[] = "Available URIs: /counter, /status, /ws and /stream, /page";

    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_show_all = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = show_all_uris_handler,
    .user_ctx = NULL
};

esp_err_t get_counter_handler(httpd_req_t *req)
{
    char resp[MAX_COUNTER_STRING_SIZE] = "Counter: ";

    int counter = get_people_counter();
    snprintf(resp + 9, MAX_COUNTER_STRING_SIZE, "%d ", counter);
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get_counter = {
    .uri      = "/counter",
    .method   = HTTP_GET,
    .handler  = get_counter_handler,
    .user_ctx = NULL
};

static esp_err_t get_camera_status_handler(httpd_req_t *req)
{
    /* JSON Example
     * {
     *       "framesize": 8,
     *       "quality": 12,
     *       "brightness": 0,
     *       "contrast": 0,
     *       "saturation": 0,
     *       "sharpness": 0,
     *       "special_effect": 0,
     *       "wb_mode": 0,
     *  }
     */
    memset(json_response, 0, sizeof(json_response));
    get_camera_status(json_response, JSON_RESPONSE_SIZE);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, (const char* )json_response, strlen(json_response));
}

httpd_uri_t uri_status = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = get_camera_status_handler,
    .user_ctx = NULL
};

static esp_err_t camera_stream_handler(httpd_req_t *req)
{
    esp_err_t ret = ESP_OK;
    camera_fb_t *fb = NULL;
    char *part_buf[64];
    
    ret = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (ret != ESP_OK) {
        return ret;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    do {
        fb = esp_camera_fb_get();
        if (!fb) {
            printf("Camera capture failed");
            continue;
        }
        printf("Frame size:  %d\n", fb->len);

        size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
        ret = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        if (ret == ESP_OK) {
            ret = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        }
        if (ret == ESP_OK) {
            ret = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        esp_camera_fb_return(fb);
    } while(ret == ESP_OK);

    return ret;
}

static esp_err_t jpg_stream_httpd_handler(httpd_req_t *req)
{
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while (true) {
        // fb = esp_camera_fb_get();
        fb = get_local_frame_buffer();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if (fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if (!jpeg_converted) {
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (fb->format != PIXFORMAT_JPEG) {
            free(_jpg_buf);
        }
        // esp_camera_fb_return(fb);
        if (res != ESP_OK) {
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}

httpd_uri_t uri_stream = {
    .uri = "/stream",
    .method = HTTP_GET,
    // .handler = camera_stream_handler,
    .handler = jpg_stream_httpd_handler,
    .user_ctx = NULL
};

esp_err_t get_html_page_handler(httpd_req_t *req)
{
    const uint32_t page_len = page_end - page_start;
    const uint32_t js_len = js_end - js_start;

    ESP_LOGI(TAG, "HTML page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, page_start, page_len);

    httpd_resp_send(req, js_start, js_len);
    httpd_resp_set_status(req, "200 OK");

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

httpd_uri_t page = {
    .uri = "/index.html",
    .method = HTTP_GET,
    .handler = get_html_page_handler,
    .user_ctx = NULL
};

/**
 * @brief  Start HTTP server
 * @param void
 * @return Server handle or NULL
 */
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &uri_show_all);
        httpd_register_uri_handler(server, &uri_get_counter);
        httpd_register_uri_handler(server, &uri_status);
        httpd_register_uri_handler(server, &uri_stream);
        httpd_register_uri_handler(server, &ws);
        httpd_register_uri_handler(server, &page);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void app_httpserver_init(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    /* Start the server for the first time */
    server = start_webserver();
}
