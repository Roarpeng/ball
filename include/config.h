#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <stdint.h>

// ==================== 硬件配置 ====================
#define LED_PIN 23
#define NUM_LEDS 144
#define LED_BRIGHTNESS 50
#define LED_TYPE WS2815
#define COLOR_ORDER RGB

// 按钮引脚定义
extern const uint8_t BUTTON_PINS[];
extern const uint8_t NUM_BUTTONS;

// ==================== 颜色配置 ====================
// RGB颜色定义 (R, G, B范围: 0-255)
#define COLOR_BREATHE_RED_R 255
#define COLOR_BREATHE_RED_G 0
#define COLOR_BREATHE_RED_B 0

#define COLOR_BREATHE_GREEN_R 0
#define COLOR_BREATHE_GREEN_G 255
#define COLOR_BREATHE_GREEN_B 0

#define COLOR_FLASH_YELLOW_R 255
#define COLOR_FLASH_YELLOW_G 255
#define COLOR_FLASH_YELLOW_B 0

// ==================== 时间配置 ====================
#define DEBOUNCE_DELAY 50
#define WEBSOCKET_UPDATE_INTERVAL 100
#define BLINK_INTERVAL 500
#define BREATHE_INTERVAL 30
#define BREATHE_STEP 5

// ==================== 网络配置 ====================
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

extern const char* MQTT_SERVER;
extern const uint16_t MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASSWORD;
extern const char* MQTT_TOPIC_SUB;
extern const char* MQTT_TOPIC_RESET;

#define WEB_SERVER_PORT 80

// ==================== 颜色配置 ====================
// RGB颜色定义 (R, G, B范围: 0-255)
#define COLOR_BREATHE_RED_R 255
#define COLOR_BREATHE_RED_G 0
#define COLOR_BREATHE_RED_B 0

#define COLOR_BREATHE_GREEN_R 0
#define COLOR_BREATHE_GREEN_G 255
#define COLOR_BREATHE_GREEN_B 0

#define COLOR_FLASH_YELLOW_R 255
#define COLOR_FLASH_YELLOW_G 255
#define COLOR_FLASH_YELLOW_B 0

// ==================== 时间配置 ====================
#define DEBOUNCE_DELAY 50
#define WEBSOCKET_UPDATE_INTERVAL 100
#define BLINK_INTERVAL 500
#define BREATHE_INTERVAL 30
#define BREATHE_STEP 5

// ==================== 网络配置 ====================
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

extern const char* MQTT_SERVER;
extern const uint16_t MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASSWORD;
extern const char* MQTT_TOPIC_SUB;
extern const char* MQTT_TOPIC_RESET;

#define WEB_SERVER_PORT 80

// ==================== 枚举定义 ====================
enum LEDMode {
  LED_OFF,
  LED_BREATHE_RED,
  LED_BREATHE_GREEN,
  LED_FLASH_YELLOW
};

enum ButtonIndex {
  BTN_P13 = 0,
  BTN_P12 = 1,
  BTN_P14 = 2,
  BTN_P27 = 3,
  BTN_P26 = 4,
  BTN_P25 = 5,
  BTN_P32 = 6
};

// ==================== 数据结构 ====================
struct ButtonState {
  bool current;
  bool previous;
  unsigned long lastDebounceTime;
  bool stateChanged;
};

// ==================== 数据结构 ====================
struct LEDController {
  LEDMode mode;
  unsigned long lastUpdateTime;
  bool blinkState;
  int breathState;
  int breathDirection;
  int greenBreathBrightness;  // 绿色呼吸的亮度级别 (0-255)
};

struct SystemStatus {
  bool wifiConnected;
  bool mqttConnected;
  bool allPinsTriggered;
  bool previousAllPinsTriggered;
  bool previousP32Triggered;
  bool p32Triggered;  // P32是否被触发过，触发后保持红色呼吸直到系统重置
};

#endif // CONFIG_H