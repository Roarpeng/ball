#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Update.h>
#include "config.h"

// ==================== 全局对象 ====================
Adafruit_NeoPixel strip(NUM_LEDS, PIN_WS2812, NEO_GRB + NEO_KHZ800);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
AsyncWebServer webServer(WEB_SERVER_PORT);
AsyncWebSocket webSocket("/ws");

// 全局状态
ButtonState buttonStates[7];  // 固定7个按钮
LEDController ledController;
SystemStatus systemStatus;

// ==================== 函数声明 ====================
void initializeSystem();
void initializeButtons();
void initializeLED();
void initializeWiFi();
void initializeMQTT();
void initializeWebServer();

void mainLoop();
void updateButtonStates();
void updateLEDController();
void updateMQTTConnection();
void updateWebSocket();

void handleButtonLogic();
void setLEDMode(LEDMode mode);
void processLEDFlash();
void processLEDBreathe();
void turnOffLEDs();

void onMQTTMessage(char* topic, byte* payload, unsigned int length);
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                      AwsEventType type, void *arg, uint8_t *data, size_t len);

bool connectToWiFi();
bool connectToMQTT();
void sendButtonStates();
void sendMQTTMessage(const char* topic, const char* message);

String getHTMLContent();
void handleOTAUpload(AsyncWebServerRequest *request, String filename, 
                     size_t index, uint8_t *data, size_t len, bool final);

// ==================== Arduino 主函数 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Ball 系统启动中...");
  
  initializeSystem();
  Serial.println("系统初始化完成");
}

void loop() {
  mainLoop();
}

// ==================== 系统初始化 ====================
void initializeSystem() {
  initializeButtons();
  initializeLED();
  initializeWiFi();
  initializeMQTT();
  initializeWebServer();
  
  // 初始化系统状态
  systemStatus.wifiConnected = false;
  systemStatus.mqttConnected = false;
  systemStatus.allPinsTriggered = false;
  systemStatus.previousAllPinsTriggered = false;
  systemStatus.previousP32Triggered = false;
  
  // 初始化LED控制器
  ledController.mode = LED_OFF;
  ledController.lastUpdateTime = 0;
  ledController.blinkState = false;
  ledController.breathState = 0;
  ledController.breathDirection = 1;
}

void initializeButtons() {
  for (uint8_t i = 0; i < 7; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttonStates[i].current = HIGH;
    buttonStates[i].previous = HIGH;
    buttonStates[i].lastDebounceTime = 0;
    buttonStates[i].stateChanged = false;
  }
  Serial.println("按钮初始化完成");
}

void initializeLED() {
  strip.begin();
  strip.show();
  strip.setBrightness(LED_BRIGHTNESS);
  Serial.println("LED灯带初始化完成");
}

void initializeWiFi() {
  Serial.print("连接WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    systemStatus.wifiConnected = true;
    Serial.println("\nWiFi连接成功");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
    Serial.print("WebUI地址: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi连接失败");
  }
}

void initializeMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(onMQTTMessage);
  Serial.println("MQTT客户端初始化完成");
}

void initializeWebServer() {
  webSocket.onEvent(onWebSocketEvent);
  webServer.addHandler(&webSocket);
  
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getHTMLContent());
  });
  
  webServer.on("/api/buttons", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    for (uint8_t i = 0; i < 7; i++) {
      json += "\"p" + String(BUTTON_PINS[i]) + "\":" + 
              String(buttonStates[i].current == LOW ? "true" : "false");
      if (i < 6) json += ",";
    }
    json += "}";
    request->send(200, "application/json", json);
  });
  
  webServer.on("/update", HTTP_POST, 
    [](AsyncWebServerRequest *request) {
      if (Update.hasError()) {
        request->send(500, "text/plain", "更新失败");
      } else {
        request->send(200, "text/plain", "更新成功，设备正在重启...");
        delay(1000);
        ESP.restart();
      }
    }, 
    handleOTAUpload
  );
  
  webServer.begin();
  Serial.println("Web服务器启动完成");
}

// ==================== 主循环 ====================
void mainLoop() {
  updateButtonStates();
  updateLEDController();
  updateMQTTConnection();
  updateWebSocket();
  handleButtonLogic();
  
  delay(10); // 小延迟以稳定系统
}

// ==================== 按钮状态更新 ====================
void updateButtonStates() {
  for (uint8_t i = 0; i < 7; i++) {
    bool reading = digitalRead(BUTTON_PINS[i]);
    
    if (reading != buttonStates[i].previous) {
      buttonStates[i].lastDebounceTime = millis();
    }
    
    if ((millis() - buttonStates[i].lastDebounceTime) > DEBOUNCE_DELAY) {
      if (reading != buttonStates[i].current) {
        buttonStates[i].current = reading;
        buttonStates[i].stateChanged = true;
      }
    }
    
    buttonStates[i].previous = reading;
  }
}

// ==================== LED控制器 ====================
void updateLEDController() {
  switch (ledController.mode) {
    case LED_FLASH_RED_YELLOW:
      processLEDFlash();
      break;
    case LED_BREATHE_GREEN:
      processLEDBreathe();
      break;
    case LED_OFF:
    default:
      turnOffLEDs();
      break;
  }
}

void processLEDFlash() {
  unsigned long currentTime = millis();
  if (currentTime - ledController.lastUpdateTime >= BLINK_INTERVAL) {
    ledController.lastUpdateTime = currentTime;
    
    uint32_t color = ledController.blinkState ? 
                     strip.Color(255, 0, 0) :    // 红色
                     strip.Color(255, 165, 0);   // 黄色
    
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, color);
    }
    strip.show();
    
    ledController.blinkState = !ledController.blinkState;
  }
}

void processLEDBreathe() {
  unsigned long currentTime = millis();
  if (currentTime - ledController.lastUpdateTime >= BREATHE_INTERVAL) {
    ledController.lastUpdateTime = currentTime;
    
    ledController.breathState += ledController.breathDirection * BREATHE_STEP;
    
    if (ledController.breathState >= 255) {
      ledController.breathState = 255;
      ledController.breathDirection = -1;
    } else if (ledController.breathState <= 0) {
      ledController.breathState = 0;
      ledController.breathDirection = 1;
    }
    
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(0, ledController.breathState, 0));
    }
    strip.show();
  }
}

void turnOffLEDs() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void setLEDMode(LEDMode mode) {
  if (ledController.mode != mode) {
    ledController.mode = mode;
    ledController.lastUpdateTime = millis();
    
    if (mode == LED_OFF) {
      turnOffLEDs();
    }
  }
}

// ==================== 按钮逻辑处理 ====================
void handleButtonLogic() {
  // 检查所有指定引脚是否都已触发 (P12, P14, P25, P26, P27)
  systemStatus.allPinsTriggered = 
    (buttonStates[BTN_P12].current == LOW) &&
    (buttonStates[BTN_P14].current == LOW) &&
    (buttonStates[BTN_P25].current == LOW) &&
    (buttonStates[BTN_P26].current == LOW) &&
    (buttonStates[BTN_P27].current == LOW);
  
  // P13单独按下 - 红黄频闪
  if (buttonStates[BTN_P13].current == LOW) {
    setLEDMode(LED_FLASH_RED_YELLOW);
  }
  // 所有指定引脚按下 - 绿色呼吸
  else if (systemStatus.allPinsTriggered) {
    setLEDMode(LED_BREATHE_GREEN);
    
    // 发送MQTT消息（状态变化时）
    if (systemStatus.allPinsTriggered != systemStatus.previousAllPinsTriggered) {
      sendMQTTMessage(MQTT_TOPIC_SUB, "");
      Serial.println("发送触发消息到MQTT");
    }
  }
  // 默认关闭
  else {
    setLEDMode(LED_OFF);
  }
  
  // P32按下 - 发送重置信号
  bool currentP32Triggered = (buttonStates[BTN_P32].current == LOW);
  if (currentP32Triggered && !systemStatus.previousP32Triggered) {
    sendMQTTMessage(MQTT_TOPIC_RESET, "");
    Serial.println("发送重置信号到MQTT");
  }
  
  // 更新状态记录
  systemStatus.previousAllPinsTriggered = systemStatus.allPinsTriggered;
  systemStatus.previousP32Triggered = currentP32Triggered;
}

// ==================== MQTT连接管理 ====================
void updateMQTTConnection() {
  if (!mqttClient.connected()) {
    systemStatus.mqttConnected = false;
    connectToMQTT();
  } else {
    systemStatus.mqttConnected = true;
  }
  mqttClient.loop();
}

bool connectToMQTT() {
  static unsigned long lastAttempt = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastAttempt < 2000) {
    return false; // 避免频繁重连
  }
  
  lastAttempt = currentTime;
  Serial.print("尝试连接MQTT服务器...");
  
  if (mqttClient.connect(MQTT_USER)) {
    Serial.println("连接成功");
    mqttClient.subscribe(MQTT_TOPIC_SUB);
    systemStatus.mqttConnected = true;
    return true;
  } else {
    Serial.print("连接失败，状态码: ");
    Serial.println(mqttClient.state());
    systemStatus.mqttConnected = false;
    return false;
  }
}

void sendMQTTMessage(const char* topic, const char* message) {
  if (systemStatus.mqttConnected) {
    mqttClient.publish(topic, message);
  }
}

void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("收到MQTT消息 [");
  Serial.print(topic);
  Serial.print("]: ");
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// ==================== WebSocket管理 ====================
void updateWebSocket() {
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastUpdate >= WEBSOCKET_UPDATE_INTERVAL) {
    lastUpdate = currentTime;
    sendButtonStates();
  }
  
  webSocket.cleanupClients();
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket客户端 #%u 连接\n", client->id());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket客户端 #%u 断开连接\n", client->id());
      break;
    case WS_EVT_DATA:
      // 处理接收到的WebSocket消息
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void sendButtonStates() {
  String json = "{";
  for (uint8_t i = 0; i < 7; i++) {
    json += "\"p" + String(BUTTON_PINS[i]) + "\":" + 
            String(buttonStates[i].current == LOW ? "true" : "false");
    if (i < 6) json += ",";
  }
  json += "}";
  webSocket.textAll(json);
}

// ==================== OTA升级处理 ====================
void handleOTAUpload(AsyncWebServerRequest *request, String filename, 
                     size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    Serial.printf("开始OTA更新: %s\n", filename.c_str());
    if (!Update.begin(request->contentLength())) {
      Update.printError(Serial);
    }
  }
  
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

// ==================== HTML内容生成 ====================
String getHTMLContent() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 Ball 控制面板</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
  html += "h1{text-align:center;color:#333}";
  html += ".section{margin:20px 0;padding:15px;border:1px solid #ddd;border-radius:5px}";
  html += ".button-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px}";
  html += ".button-status{padding:10px;text-align:center;border-radius:5px;font-weight:bold;transition:all 0.3s}";
  html += ".button-on{background:#4CAF50;color:white;transform:scale(1.05)}";
  html += ".button-off{background:#f44336;color:white}";
  html += ".ota-section{text-align:center}";
  html += ".upload-btn{background:#2196F3;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:10px 0;transition:background 0.3s}";
  html += ".upload-btn:hover{background:#1976D2}";
  html += ".status{margin:10px 0;padding:10px;border-radius:5px}";
  html += ".success{background:#dff0d8;color:#3c763d}";
  html += ".error{background:#f2dede;color:#a94442}";
  html += ".info{background:#d9edf7;color:#31708f}";
  html += "</style></head><body>";
  html += "<div class=\"container\">";
  html += "<h1>🎮 ESP32 Ball 控制面板</h1>";
  
  // 系统状态
  html += "<div class=\"section\">";
  html += "<h2>📊 系统状态</h2>";
  html += "<div id=\"system-status\" class=\"status info\">正在加载...</div>";
  html += "</div>";
  
  // 按钮状态
  html += "<div class=\"section\">";
  html += "<h2>🔘 按钮状态监控</h2>";
  html += "<div class=\"button-grid\">";
  for (uint8_t i = 0; i < 7; i++) {
    html += "<div id=\"p" + String(BUTTON_PINS[i]) + "\" class=\"button-status button-off\">";
    html += "P" + String(BUTTON_PINS[i]) + ": 关闭</div>";
  }
  html += "</div></div>";
  
  // OTA升级
  html += "<div class=\"section ota-section\">";
  html += "<h2>🔄 OTA 固件升级</h2>";
  html += "<input type=\"file\" id=\"firmware\" accept=\".bin\" style=\"margin:10px 0\">";
  html += "<br><button class=\"upload-btn\" onclick=\"uploadFirmware()\">📤 上传固件</button>";
  html += "<div id=\"status\"></div>";
  html += "</div></div>";
  
  // JavaScript
  html += "<script>";
  html += "const ws=new WebSocket('ws://'+window.location.hostname+'/ws');";
  html += "ws.onmessage=function(e){";
  html += "const data=JSON.parse(e.data);";
  for (uint8_t i = 0; i < 7; i++) {
    html += "updateButton('p" + String(BUTTON_PINS[i]) + "',data.p" + String(BUTTON_PINS[i]) + ");";
  }
  html += "};";
  html += "function updateButton(id,state){";
  html += "const el=document.getElementById(id);";
  html += "if(state){el.className='button-status button-on';el.textContent=id.toUpperCase()+': 开启';}";
  html += "else{el.className='button-status button-off';el.textContent=id.toUpperCase()+': 关闭';}";
  html += "}";
  html += "function uploadFirmware(){";
  html += "const file=document.getElementById('firmware').files[0];";
  html += "if(!file){showStatus('请选择固件文件','error');return;}";
  html += "const fd=new FormData();fd.append('firmware',file);";
  html += "showStatus('正在上传固件...','info');";
  html += "fetch('/update',{method:'POST',body:fd})";
  html += ".then(r=>r.text()).then(d=>{showStatus(d,'success');if(d.includes('成功'))setTimeout(()=>location.reload(),3000);})";
  html += ".catch(e=>showStatus('上传失败: '+e,'error'));";
  html += "}";
  html += "function showStatus(msg,type){";
  html += "const el=document.getElementById('status');el.textContent=msg;el.className='status '+type;";
  html += "}";
  html += "function updateSystemStatus(){";
  html += "fetch('/api/buttons').then(r=>r.json()).then(d=>{";
  html += "const online=Object.values(d).some(v=>v===true||v===false);";
  html += "const status=document.getElementById('system-status');";
  html += "status.textContent=online?'系统运行正常':'连接异常';";
  html += "status.className='status '+(online?'success':'error');";
  html += "}).catch(()=>{document.getElementById('system-status').textContent='连接失败';document.getElementById('system-status').className='status error';});";
  html += "}";
  html += "setInterval(updateSystemStatus,5000);updateSystemStatus();";
  html += "</script></body></html>";
  
  return html;
}