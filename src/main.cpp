#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Update.h>

#define PIN_WS2812 23      // 假设WS2812灯带连接到GPIO23，请根据实际连接调整
#define NUM_LEDS 144       // 灯带上的LED数量
#define P13_PIN 13         // P13引脚
#define P12_PIN 12         // P12引脚
#define P14_PIN 14         // P14引脚
#define P27_PIN 27         // P27引脚
#define P26_PIN 26         // P26引脚
#define P25_PIN 25         // P25引脚
#define P32_PIN 32         // P32引脚

// WiFi 配置 - 请根据您的网络环境修改
const char* ssid = "LC_01";
const char* password = "12345678";

// MQTT 配置
#define MQTT_SERVER "192.168.10.80"
#define MQTT_PORT 1883
#define MQTT_USER "ball"
#define MQTT_PASSWORD ""
#define MQTT_TOPIC_SUB "ball/triggered"

// Web服务器配置
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// 按钮状态结构体
struct ButtonState {
  bool p13;
  bool p12;
  bool p14;
  bool p27;
  bool p26;
  bool p25;
  bool p32;
};

ButtonState currentButtonState = {false, false, false, false, false, false, false};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN_WS2812, NEO_GRB + NEO_KHZ800);

WiFiClient espClient;
PubSubClient client(espClient);

// 函数声明
void flashRedYellow();
void breatheGreen();
void turnOffAll();
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_web_server();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void sendButtonStates();
String getHTML();
void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

// 全局变量
unsigned long previousMillis = 0;
bool blinkState = false;    // 用于频闪效果
int breathState = 0;        // 用于呼吸效果
int breathDirection = 1;    // 呼吸方向：1为增加，-1为减少
bool allPinsTriggered = false;  // 标记所有指定引脚是否都已触发
bool previousAllPinsTriggered = false;  // 上次循环时的状态，用于检测状态变化

void setup() {
  // 初始化引脚为输入模式，启用内部上拉电阻
  pinMode(P13_PIN, INPUT_PULLUP);
  pinMode(P12_PIN, INPUT_PULLUP);
  pinMode(P14_PIN, INPUT_PULLUP);
  pinMode(P27_PIN, INPUT_PULLUP);
  pinMode(P26_PIN, INPUT_PULLUP);
  pinMode(P25_PIN, INPUT_PULLUP);
  pinMode(P32_PIN, INPUT_PULLUP);
  
  // 初始化串口以便调试
  Serial.begin(115200);
  
  // 初始化WS2812灯带
  strip.begin();
  strip.show(); // 初始化所有LED为关闭状态
  strip.setBrightness(50); // 设置亮度（0-255）
  
  // 设置WiFi和MQTT
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  
  // 设置Web服务器
  setup_web_server();
}

void loop() {
  // 检测P13状态 (使用消抖处理)
  static unsigned long lastDebounceTimeP13 = 0;
  static bool lastP13State = HIGH;  // 默认为HIGH，因为使用了内部上拉
  static bool currentP13State = HIGH;
  bool readingP13 = digitalRead(P13_PIN);
  
  if (readingP13 != lastP13State) {
    lastDebounceTimeP13 = millis();
  }
  
  if ((millis() - lastDebounceTimeP13) > 50) {  // 50ms消抖延迟
    if (readingP13 != currentP13State) {
      currentP13State = readingP13;
    }
  }
  lastP13State = readingP13;
  
  // 检测P12, P14, P27, P26, P25状态 (使用消抖处理)
  static unsigned long lastDebounceTimeP12 = 0;
  static unsigned long lastDebounceTimeP14 = 0;
  static unsigned long lastDebounceTimeP27 = 0;
  static unsigned long lastDebounceTimeP26 = 0;
  static unsigned long lastDebounceTimeP25 = 0;
  static bool lastP12State = HIGH, lastP14State = HIGH, lastP27State = HIGH, lastP26State = HIGH, lastP25State = HIGH;
  static bool currentP12State = HIGH, currentP14State = HIGH, currentP27State = HIGH, currentP26State = HIGH, currentP25State = HIGH;
  
  bool readingP12 = digitalRead(P12_PIN);
  bool readingP14 = digitalRead(P14_PIN);
  bool readingP27 = digitalRead(P27_PIN);
  bool readingP26 = digitalRead(P26_PIN);
  bool readingP25 = digitalRead(P25_PIN);
  
  // P12 消抖处理
  if (readingP12 != lastP12State) {
    lastDebounceTimeP12 = millis();
  }
  if ((millis() - lastDebounceTimeP12) > 50) {
    if (readingP12 != currentP12State) {
      currentP12State = readingP12;
    }
  }
  lastP12State = readingP12;
  
  // P14 消抖处理
  if (readingP14 != lastP14State) {
    lastDebounceTimeP14 = millis();
  }
  if ((millis() - lastDebounceTimeP14) > 50) {
    if (readingP14 != currentP14State) {
      currentP14State = readingP14;
    }
  }
  lastP14State = readingP14;
  
  // P27 消抖处理
  if (readingP27 != lastP27State) {
    lastDebounceTimeP27 = millis();
  }
  if ((millis() - lastDebounceTimeP27) > 50) {
    if (readingP27 != currentP27State) {
      currentP27State = readingP27;
    }
  }
  lastP27State = readingP27;
  
  // P26 消抖处理
  if (readingP26 != lastP26State) {
    lastDebounceTimeP26 = millis();
  }
  if ((millis() - lastDebounceTimeP26) > 50) {
    if (readingP26 != currentP26State) {
      currentP26State = readingP26;
    }
  }
  lastP26State = readingP26;
  
  // P25 消抖处理
  if (readingP25 != lastP25State) {
    lastDebounceTimeP25 = millis();
  }
  if ((millis() - lastDebounceTimeP25) > 50) {
    if (readingP25 != currentP25State) {
      currentP25State = readingP25;
    }
  }
  lastP25State = readingP25;
  
  // 检测P32状态 (使用消抖处理)
  static unsigned long lastDebounceTimeP32 = 0;
  static bool lastP32State = HIGH;
  static bool currentP32State = HIGH;
  bool readingP32 = digitalRead(P32_PIN);
  
  if (readingP32 != lastP32State) {
    lastDebounceTimeP32 = millis();
  }
  
  if ((millis() - lastDebounceTimeP32) > 50) {  // 50ms消抖延迟
    if (readingP32 != currentP32State) {
      currentP32State = readingP32;
    }
  }
  lastP32State = readingP32;
  
  // 检查所有指定引脚是否都已触发 (低电平触发，因为使用了内部上拉)
  allPinsTriggered = (currentP12State == LOW && currentP14State == LOW && 
                      currentP25State == LOW && currentP26State == LOW && currentP27State == LOW);
  
  // 如果P13有信号（低电平），执行红色黄色频闪
  if (currentP13State == LOW) {
    flashRedYellow();
  }
  // 如果P12, P14, P27, P26, P25都有信号（低电平），执行绿色呼吸
  else if (allPinsTriggered) {
    breatheGreen();
  }
  // 否则关闭所有LED
  else {
    turnOffAll();
  }
  
  // MQTT 连接管理
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // WebSocket清理
  ws.cleanupClients();
  
  // 更新按钮状态
  currentButtonState.p13 = (currentP13State == LOW);
  currentButtonState.p12 = (currentP12State == LOW);
  currentButtonState.p14 = (currentP14State == LOW);
  currentButtonState.p27 = (currentP27State == LOW);
  currentButtonState.p26 = (currentP26State == LOW);
  currentButtonState.p25 = (currentP25State == LOW);
  currentButtonState.p32 = (currentP32State == LOW);
  
  // 定期发送按钮状态到WebSocket客户端
  static unsigned long lastWebSocketUpdate = 0;
  if (millis() - lastWebSocketUpdate > 100) { // 每100ms更新一次
    sendButtonStates();
    lastWebSocketUpdate = millis();
  }
  
  // 如果所有指定引脚都已触发，且之前未触发，则发送空报文
  if (allPinsTriggered && !previousAllPinsTriggered) {
    // 发送空报文到 MQTT 主题
    client.publish("ball/triggered", ""); 
    Serial.println("已发送空报文到 ball/triggered");
  }

  // 如果P32被触发（低电平），则发送空报文到MQTT主题#/reset
  static bool previousP32Triggered = false;
  bool currentP32Triggered = (currentP32State == LOW);
  if (currentP32Triggered && !previousP32Triggered) {
    // 发送空报文到 MQTT 主题 #/reset
    client.publish("#/reset", "");
    Serial.println("已发送空报文到 #/reset");
  }
  previousP32Triggered = currentP32Triggered;

  // 更新上一次的状态
  previousAllPinsTriggered = allPinsTriggered;
}

// 红色黄色频闪效果
void flashRedYellow() {
  unsigned long currentMillis = millis();
  
  // 每500ms切换一次状态
  if (currentMillis - previousMillis >= 500) {
    previousMillis = currentMillis;
    
    if (blinkState == false) {
      // 设置红色
      for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(255, 0, 0)); // 红色
      }
      blinkState = true;
    } else {
      // 设置黄色
      for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(255, 165, 0)); // 黄色
      }
      blinkState = false;
    }
    strip.show();
  }
}

// 绿色呼吸效果
void breatheGreen() {
  unsigned long currentMillis = millis();
  
  // 每30ms更新一次亮度
  if (currentMillis - previousMillis >= 30) {
    previousMillis = currentMillis;
    
    // 更新呼吸状态
    breathState += breathDirection * 5;
    
    // 控制亮度范围在0-255之间
    if (breathState >= 255) {
      breathState = 255;
      breathDirection = -1; // 开始变暗
    } else if (breathState <= 0) {
      breathState = 0;
      breathDirection = 1;  // 开始变亮
    }
    
    // 设置所有LED为绿色，使用变化的亮度
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(0, breathState, 0)); // 绿色，亮度变化
    }
    strip.show();
  }
}

// 关闭所有LED
void turnOffAll() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0, 0, 0); // 关闭LED
  }
  strip.show();
}

void setup_wifi() {
  delay(10);
  // 连接到WiFi网络
  Serial.println();
  Serial.print("正在连接到 WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi 连接成功");
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());
  Serial.print("访问WebUI: http://");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("收到消息 [");
  Serial.print(topic);
  Serial.print("] ");
  
  // 可以在这里处理收到的消息
  // 目前只打印到串口
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // 循环直到重新连接
  while (!client.connected()) {
    Serial.print("尝试连接到 MQTT 服务器...");
    // 尝试连接，使用 MQTT_USER 作为客户端ID
    if (client.connect(MQTT_USER)) {
      Serial.println(" 连接成功");
      // 订阅主题
      client.subscribe(MQTT_TOPIC_SUB);
      Serial.print("已订阅主题: ");
      Serial.println(MQTT_TOPIC_SUB);
    } else {
      Serial.print(" 连接失败，失败代码: ");
      Serial.print(client.state());
      Serial.println(" 2秒后重试");
      delay(2000);
    }
  }
}

// Web服务器设置
void setup_web_server() {
  // WebSocket事件处理
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  
  // 主页路由
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", getHTML());
  });
  
  // OTA升级路由
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    if (Update.hasError()) {
      request->send(500, "text/plain", "更新失败");
    } else {
      request->send(200, "text/plain", "更新成功，设备正在重启...");
      delay(1000);
      ESP.restart();
    }
  }, handleOTAUpload);
  
  // 获取按钮状态API
  server.on("/api/buttons", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"p13\":" + String(currentButtonState.p13 ? "true" : "false") + ",";
    json += "\"p12\":" + String(currentButtonState.p12 ? "true" : "false") + ",";
    json += "\"p14\":" + String(currentButtonState.p14 ? "true" : "false") + ",";
    json += "\"p27\":" + String(currentButtonState.p27 ? "true" : "false") + ",";
    json += "\"p26\":" + String(currentButtonState.p26 ? "true" : "false") + ",";
    json += "\"p25\":" + String(currentButtonState.p25 ? "true" : "false") + ",";
    json += "\"p32\":" + String(currentButtonState.p32 ? "true" : "false");
    json += "}";
    request->send(200, "application/json", json);
  });
  
  // 启动服务器
  server.begin();
  Serial.println("Web服务器已启动");
}

// WebSocket事件处理
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket客户端 #%u 连接\n", client->id());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket客户端 #%u 断开连接\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// 处理WebSocket消息
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  // 这里可以处理来自客户端的消息
}

// 发送按钮状态到所有WebSocket客户端
void sendButtonStates() {
  String json = "{";
  json += "\"p13\":" + String(currentButtonState.p13 ? "true" : "false") + ",";
  json += "\"p12\":" + String(currentButtonState.p12 ? "true" : "false") + ",";
  json += "\"p14\":" + String(currentButtonState.p14 ? "true" : "false") + ",";
  json += "\"p27\":" + String(currentButtonState.p27 ? "true" : "false") + ",";
  json += "\"p26\":" + String(currentButtonState.p26 ? "true" : "false") + ",";
  json += "\"p25\":" + String(currentButtonState.p25 ? "true" : "false") + ",";
  json += "\"p32\":" + String(currentButtonState.p32 ? "true" : "false");
  json += "}";
  ws.textAll(json);
}

// HTML页面
String getHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 Ball 控制面板</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "h1 { text-align: center; color: #333; }";
  html += ".section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }";
  html += ".button-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 10px; }";
  html += ".button-status { padding: 10px; text-align: center; border-radius: 5px; font-weight: bold; }";
  html += ".button-on { background-color: #4CAF50; color: white; }";
  html += ".button-off { background-color: #f44336; color: white; }";
  html += ".ota-section { text-align: center; }";
  html += ".file-input { margin: 10px 0; }";
  html += ".upload-btn { background-color: #2196F3; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }";
  html += ".upload-btn:hover { background-color: #1976D2; }";
  html += ".status { margin: 10px 0; padding: 10px; border-radius: 5px; }";
  html += ".success { background-color: #dff0d8; color: #3c763d; }";
  html += ".error { background-color: #f2dede; color: #a94442; }";
  html += "</style></head><body>";
  html += "<div class=\"container\">";
  html += "<h1>ESP32 Ball 控制面板</h1>";
  html += "<div class=\"section\">";
  html += "<h2>按钮状态监控</h2>";
  html += "<div class=\"button-grid\">";
  html += "<div id=\"p13\" class=\"button-status button-off\">P13: 关闭</div>";
  html += "<div id=\"p12\" class=\"button-status button-off\">P12: 关闭</div>";
  html += "<div id=\"p14\" class=\"button-status button-off\">P14: 关闭</div>";
  html += "<div id=\"p27\" class=\"button-status button-off\">P27: 关闭</div>";
  html += "<div id=\"p26\" class=\"button-status button-off\">P26: 关闭</div>";
  html += "<div id=\"p25\" class=\"button-status button-off\">P25: 关闭</div>";
  html += "<div id=\"p32\" class=\"button-status button-off\">P32: 关闭</div>";
  html += "</div></div>";
  html += "<div class=\"section ota-section\">";
  html += "<h2>OTA 固件升级</h2>";
  html += "<div class=\"file-input\">";
  html += "<input type=\"file\" id=\"firmware\" accept=\".bin\">";
  html += "</div>";
  html += "<button class=\"upload-btn\" onclick=\"uploadFirmware()\">上传固件</button>";
  html += "<div id=\"status\"></div>";
  html += "</div></div>";
  html += "<script>";
  html += "const ws = new WebSocket('ws://' + window.location.hostname + '/ws');";
  html += "ws.onmessage = function(event) {";
  html += "const data = JSON.parse(event.data);";
  html += "updateButtonStatus('p13', data.p13);";
  html += "updateButtonStatus('p12', data.p12);";
  html += "updateButtonStatus('p14', data.p14);";
  html += "updateButtonStatus('p27', data.p27);";
  html += "updateButtonStatus('p26', data.p26);";
  html += "updateButtonStatus('p25', data.p25);";
  html += "updateButtonStatus('p32', data.p32);";
  html += "};";
  html += "function updateButtonStatus(buttonId, isPressed) {";
  html += "const element = document.getElementById(buttonId);";
  html += "if (isPressed) {";
  html += "element.className = 'button-status button-on';";
  html += "element.textContent = buttonId.toUpperCase() + ': 开启';";
  html += "} else {";
  html += "element.className = 'button-status button-off';";
  html += "element.textContent = buttonId.toUpperCase() + ': 关闭';";
  html += "}";
  html += "}";
  html += "function uploadFirmware() {";
  html += "const file = document.getElementById('firmware').files[0];";
  html += "if (!file) {";
  html += "showStatus('请选择固件文件', 'error');";
  html += "return;";
  html += "}";
  html += "const formData = new FormData();";
  html += "formData.append('firmware', file);";
  html += "showStatus('正在上传固件...', 'success');";
  html += "fetch('/update', {";
  html += "method: 'POST',";
  html += "body: formData";
  html += "})";
  html += ".then(response => response.text())";
  html += ".then(data => {";
  html += "showStatus(data, 'success');";
  html += "if (data.includes('成功')) {";
  html += "setTimeout(() => {";
  html += "window.location.reload();";
  html += "}, 3000);";
  html += "}";
  html += "})";
  html += ".catch(error => {";
  html += "showStatus('上传失败: ' + error, 'error');";
  html += "});";
  html += "}";
  html += "function showStatus(message, type) {";
  html += "const statusElement = document.getElementById('status');";
  html += "statusElement.textContent = message;";
  html += "statusElement.className = 'status ' + type;";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  return html;
}

// OTA上传处理
void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    Serial.printf("开始OTA更新: %s\n", filename.c_str());
    // 开始OTA更新
    size_t contentLength = request->contentLength();
    if (!Update.begin(contentLength)) {
      Update.printError(Serial);
    }
  }
  
  // 写入OTA数据
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }
  
  if (final) {
    if (Update.end(true)) {
      Serial.printf("OTA更新成功: %u bytes\n", index + len);
    } else {
      Update.printError(Serial);
    }
  }
}