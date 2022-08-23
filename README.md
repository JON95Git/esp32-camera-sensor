# ESP32 Camera sensor

Camera sensor

### Dependencies
 - [ESP32-Camera](https://github.com/espressif/esp32-camera) - added as submodule
 - ESP32-Cam Board, LiLyGo CAM or T-mini board

### How to get code

```
$ git clone [$REPO_LINK]
```

Update submodule (get ESP32-Camera repository)

```
$ git submodule update --init --recursive
```

### Prepare environment
Export variables to PATH (mandatory for ESP-IDF use):
```bash
$ . $HOME/esp/esp-idf/export.sh'
```
### Change Wi-Fi SSID and PASSWORD to run HTTP Server

You need to change the SSID and PASSWORD to connect to your network.

Run
```bash
idf.py menuconfig
```

Navigate to `esp32-cam-sensor Configuration`->`Network parameters` and edit parameters


### How to build

Build:
```bash
$ idf.py build
```

### How to flash

Flash (example on `Ubuntu Linux`, board device is `/dev/ttyACM0`, will be different in `Windows` or `MacOS`):
```bash
$ idf.py -p /dev/ttyACM0 flash
```

### How to run

You can see the log using a terminal emulator or connecting via Wi-Fi and send GET requests.

#### Serial emulator mode

Connect to a monitor emulator (`Minicom`, `Picocom` or something like that):

```bash
picocom -b 115200 -r -l /dev/ttyACM0
```

Observe the log:

```bash
I (1585) cam_hal: cam init ok
I (1595) sccb: pin_sda 13 pin_scl 12
I (1605) camera: Detected camera at address=0x30
I (1605) camera: Detected OV2640 camera
I (1605) camera: Camera PID=0x26 VER=0x42 MIDL=0x7f MIDH=0xa2
I (1705) esp32 ll_cam: node_size: 2560, nodes_per_line: 1, lines_per_node: 1, dma_half_buffer_min:  2560, dma_half_buffer: 15360, lines_per_half_buffer:  6, dma_buffer_size: 30720, image_size: 153600
I (1705) cam_hal: buffer_size: 30720, half_buffer_size: 15360, node_buffer_size: 2560, node_cnt: 12, total_cnt: 10
I (1725) cam_hal: Allocating 76800 Byte frame buffer in PSRAM
I (1725) cam_hal: cam config ok
I (1735) ov2640: Set PLL: clk_2x: 0, clk_div: 3, pclk_auto: 1, pclk_div: 8
Camera init finished
I (1865) wifi station: ESP_WIFI_MODE_STA
I (1885) wifi:wifi driver task: 3ffd2e5c, prio:23, stack:6656, core=0
I (1885) system_api: Base MAC address is not set
I (1885) system_api: read default base MAC address from EFUSE
I (1915) wifi:wifi firmware version: 3aa4137
I (1915) wifi:wifi certification version: v7.0
I (1915) wifi:config NVS flash: enabled
I (1915) wifi:config nano formating: disabled
I (1915) wifi:Init data frame dynamic rx buffer num: 32
I (1925) wifi:Init management frame dynamic rx buffer num: 32
I (1925) wifi:Init management short buffer num: 32
I (1935) wcam_hal: EV-EOF-OVF
ifi:Init static tx buffer num: 16
I (1935) wifi:Init tx cache buffer num: 32
I (1945) wifi:Init static rx buffer size: 1600
I (1945) wifi:Init static rx buffer num: 10
I (1945) wifi:Init dynamic rx buffer num: 32
I (1955) wifi_init: rx ba win: 6
I (1955) wifi_init: tcpip mbox: 32
I (1965) wifi_init: udp mbox: 6
I (1965) wifi_init: tcp mbox: 6
I (1965) wifi_init: tcp tx win: 5744
I (1975) wifi_init: tcp rx win: 5744
I (1975) wifi_init: tcp mss: 1440
I (1985) wifi_init: WiFi IRAM OP enabled
I (1985) wifi_init: WiFi RX IRAM OP enabled
I (1995) phy_init: phy_version 4670,719f9f6,Feb 18 2021,17:07:07
cam_hal: EV-EOF-OVF
cam_hal: EV-EOF-OVF
I (2095) wifi:mode : sta (08:3a:f2:47:3e:f4)
I (2095) wifi:enable tsf
I (2105) wifi station: wifi_init_sta finished.
I (2105) wifi:new:<1,0>, old:<1,0>, ap:<255,255>, sta:<1,0>, prof:1
I (2105) wifi:state: init -> auth (b0)
I (2135) wifi:state: auth -> assoc (0)
I (2145) wifi:state: assoc -> run (10)
I (2255) wifi:connected with my_ssid, aid = 1, channel 1, BW20, bssid = d8:c6:78:be:33:18
I (2255) wifi:security: WPA2-PSK, phy: bgn, rssi: -48
I (2255) wifi:pm start, type: 1

I (2355) wifi:AP's beacon interval = 102400 us, DTIM period = 3
W (2415) wifi:<ba-add>idx:0 (ifx:0, d8:c6:78:be:33:18), tid:0, ssn:3, winSize:64
I (3365) esp_netif_handlers: sta ip: 192.168.15.143, mask: 255.255.255.0, gw: 192.168.15.1
I (3365) wifi station: got ip:192.168.15.143
I (3365) wifi station: connected to ap SSID:my_ssid password:my_password
I (3375) httpSERVER: Starting server on port: '80'
I (3385) httpSERVER: Registering URI handlers
counter: -1
counter: -1

```

#### HTTP server mode

After changing the SSID and PASSWORD, the board will get and dynamic IP (see the logs).

For example, supose the board get the IP `192.168.15.143`, use this IP in your browser. The responde will be:

```
Available URIs: /counter, /status and /stream
```

A message showing the available URI's will be showed.

There are three options here:
- Counter: show the people counter (a simple GET operation)
- Status: show the status of the camera parameters (a simple GET operation)
- Stream: show the stream of the camera (GET in STREAM mode). In this mode, Counter is not available

