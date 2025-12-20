#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <stdint.h>

// ==================== 硬件配置 ====================
#define PIN_WS2812 23
#define NUM_LEDS 144
#define LED_BRIGHTNESS 50

// 按钮引脚定义
extern const uint8_t BUTTON_PINS[];
extern const uint8_t NUM_BUTTONS;

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
  LED_FLASH_RED_YELLOW,
  LED_BREATHE_GREEN
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

struct LEDController {
  LEDMode mode;
  unsigned long lastUpdateTime;
  bool blinkState;
  int breathState;
  int breathDirection;
};

struct SystemStatus {
  bool wifiConnected;
  bool mqttConnected;
  bool allPinsTriggered;
  bool previousAllPinsTriggered;
  bool previousP32Triggered;
};

#endif // CONFIG_H