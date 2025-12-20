#include "config.h"

// 按钮引脚定义
const uint8_t BUTTON_PINS[] = {13, 12, 14, 27, 26, 25, 32};
const uint8_t NUM_BUTTONS = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);

// WiFi 配置
const char* WIFI_SSID = "LC_01";
const char* WIFI_PASSWORD = "12345678";

// MQTT 配置
const char* MQTT_SERVER = "192.168.10.80";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "ball";
const char* MQTT_PASSWORD = "";
const char* MQTT_TOPIC_SUB = "ball/triggered";
const char* MQTT_TOPIC_RESET = "ball/reset";