#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Update.h>
#include "config.h"

// ==================== 全局对象 ====================
CRGB leds[NUM_LEDS];
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
void processLEDBreatheRed();
void processLEDBreatheGreen();
void processLEDFlashYellow();
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
  systemStatus.p32Triggered = false;
  systemStatus.firstTriggeredSent = false;  // 初始化首次触发标志为false
  
  // 初始化LED控制器
  ledController.mode = LED_BREATHE_RED;  // 默认红色呼吸
  ledController.lastUpdateTime = 0;
  ledController.blinkState = false;
  ledController.breathState = 0;
  ledController.breathDirection = 1;
  ledController.greenBreathBrightness = 0;  // 初始亮度为0
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
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
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
    case LED_BREATHE_RED:
      processLEDBreatheRed();
      break;
    case LED_BREATHE_GREEN:
      processLEDBreatheGreen();
      break;
    case LED_FLASH_YELLOW:
      processLEDFlashYellow();
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
    
    CRGB color = ledController.blinkState ? 
                 CRGB::Red :    // 红色
                 CRGB(255, 165, 0);   // 黄色
    
    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();
    
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
    
    fill_solid(leds, NUM_LEDS, CRGB(0, ledController.breathState, 0));
    FastLED.show();
  }
}

void turnOffLEDs() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

// ==================== LED灯效函数 ====================
void processLEDBreatheRed() {
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

    // 使用配置的RGB颜色，根据呼吸状态调整亮度
    uint8_t brightness = ledController.breathState;
    uint8_t r = (COLOR_BREATHE_RED_R * brightness) / 255;
    uint8_t g = (COLOR_BREATHE_RED_G * brightness) / 255;
    uint8_t b = (COLOR_BREATHE_RED_B * brightness) / 255;

    fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
    FastLED.show();
  }
}

void processLEDBreatheGreen() {
  unsigned long currentTime = millis();
  if (currentTime - ledController.lastUpdateTime >= BREATHE_INTERVAL) {
    ledController.lastUpdateTime = currentTime;

    ledController.breathState += ledController.breathDirection * BREATHE_STEP;

    if (ledController.breathState >= ledController.greenBreathBrightness) {
      ledController.breathState = ledController.greenBreathBrightness;
      ledController.breathDirection = -1;
    } else if (ledController.breathState <= 0) {
      ledController.breathState = 0;
      ledController.breathDirection = 1;
    }

    // 使用配置的RGB颜色，根据呼吸状态调整亮度
    uint8_t brightness = ledController.breathState;
    uint8_t r = (COLOR_BREATHE_GREEN_R * brightness) / 255;
    uint8_t g = (COLOR_BREATHE_GREEN_G * brightness) / 255;
    uint8_t b = (COLOR_BREATHE_GREEN_B * brightness) / 255;

    fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
    FastLED.show();
  }
}

void processLEDFlashYellow() {
  unsigned long currentTime = millis();
  if (currentTime - ledController.lastUpdateTime >= BLINK_INTERVAL) {
    ledController.lastUpdateTime = currentTime;

    // 使用配置的RGB颜色
    CRGB color = ledController.blinkState ?
                 CRGB(COLOR_FLASH_YELLOW_R, COLOR_FLASH_YELLOW_G, COLOR_FLASH_YELLOW_B) :
                 CRGB::Black;

    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();

    ledController.blinkState = !ledController.blinkState;
  }
}

void setLEDMode(LEDMode mode) {
  if (ledController.mode != mode) {
    ledController.mode = mode;
    ledController.lastUpdateTime = millis();
    ledController.breathState = 0;  // 重置呼吸状态
    ledController.breathDirection = 1;
    
    if (mode == LED_OFF) {
      turnOffLEDs();
    }
  }
}

// ==================== 按钮逻辑处理 ====================
void handleButtonLogic() {
  // 检查是否有非P32的按钮被按下
  bool nonP32Pressed = false;
  for (int i = 0; i < 6; i++) { // 检查前6个按钮（非P32）
    if (buttonStates[i].current == LOW) {
      nonP32Pressed = true;
      break;
    }
  }
  
  // 如果尚未发送首次触发消息，且有非P32按钮被按下，则发送首次触发消息
  if (!systemStatus.firstTriggeredSent && nonP32Pressed) {
    sendMQTTMessage(MQTT_TOPIC_FIRST_TRIGGERED, "");
    Serial.println("首次触发：发送 ball/firstTriggered 消息");
    systemStatus.firstTriggeredSent = true;
  }

  // 打印所有引脚状态信息
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastPrintTime >= 1000) { // 每秒打印一次
    lastPrintTime = currentTime;
    Serial.print("引脚状态: P13=");
    Serial.print(buttonStates[BTN_P13].current == LOW ? "LOW" : "HIGH");
    Serial.print(", P32=");
    Serial.print(buttonStates[BTN_P32].current == LOW ? "LOW" : "HIGH");
    Serial.print(", P12=");
    Serial.print(buttonStates[BTN_P12].current == LOW ? "LOW" : "HIGH");
    Serial.print(", P14=");
    Serial.print(buttonStates[BTN_P14].current == LOW ? "LOW" : "HIGH");
    Serial.print(", P25=");
    Serial.print(buttonStates[BTN_P25].current == LOW ? "LOW" : "HIGH");
    Serial.print(", P26=");
    Serial.print(buttonStates[BTN_P26].current == LOW ? "LOW" : "HIGH");
    Serial.print(", P27=");
    Serial.println(buttonStates[BTN_P27].current == LOW ? "LOW" : "HIGH");
  }
  
  // 1. 统计当前绿色灯效组按下的按键数量
  int greenPressedCount = 0;
  if (buttonStates[BTN_P12].current == LOW) greenPressedCount++;
  if (buttonStates[BTN_P14].current == LOW) greenPressedCount++;
  if (buttonStates[BTN_P25].current == LOW) greenPressedCount++;
  if (buttonStates[BTN_P26].current == LOW) greenPressedCount++;
  if (buttonStates[BTN_P27].current == LOW) greenPressedCount++;

  // 2. 核心逻辑判断（严格执行优先级）

  // --- 优先级 1: P13 黄色频闪 (最高优先级，错误/警告) ---
  if (buttonStates[BTN_P13].current == LOW) {
    setLEDMode(LED_FLASH_YELLOW);
    systemStatus.previousAllPinsTriggered = false;
    systemStatus.previousP32Triggered = false;
  }
  
  // --- 优先级 2: P32 红色呼吸 (它必须排在绿色之前) ---
  // 逻辑：如果 P32 被按下，直接强制进入红色模式，忽略任何绿色按钮的状态
  else if (buttonStates[BTN_P32].current == LOW) {
    setLEDMode(LED_BREATHE_RED);
    
    // 发送 MQTT 重置消息（仅在按下瞬间发送一次）
    if (!systemStatus.previousP32Triggered) {
      sendMQTTMessage(MQTT_TOPIC_RESET, "");
      Serial.println("P32 触发：强制红色呼吸并发送 RESET");
      systemStatus.previousP32Triggered = true;
    }
    systemStatus.previousAllPinsTriggered = false;
  }
  
  // --- 优先级 3: 绿色组触发 (只有在 P13 和 P32 都没按时才生效) ---
  else if (greenPressedCount > 0) {
    // 设置亮度：按下的个数 * 51
    ledController.greenBreathBrightness = greenPressedCount * 51;
    setLEDMode(LED_BREATHE_GREEN);
    
    // 检查是否全亮
    bool allGreen = (greenPressedCount == 5);
    if (allGreen && !systemStatus.previousAllPinsTriggered) {
      sendMQTTMessage(MQTT_TOPIC_SUB, ""); 
      Serial.println("全部绿色引脚触发：发送 TRIGGERED");
    }
    systemStatus.previousAllPinsTriggered = allGreen;
    systemStatus.previousP32Triggered = false; 
  }
  
  // --- 优先级 4: 默认状态（无任何引脚触发）- 红色呼吸 ---
  else {
    setLEDMode(LED_BREATHE_RED);
    ledController.greenBreathBrightness = 0;
    systemStatus.previousAllPinsTriggered = false;
    systemStatus.previousP32Triggered = false;
  }
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
    mqttClient.subscribe(MQTT_TOPIC_FIRST_TRIGGERED);  // 添加对ball/firstTriggered的订阅
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