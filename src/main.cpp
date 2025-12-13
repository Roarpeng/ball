#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define PIN_WS2812 23      // 假设WS2812灯带连接到GPIO23，请根据实际连接调整
#define NUM_LEDS 144       // 灯带上的LED数量
#define P13_PIN 13         // P13引脚
#define P12_PIN 12         // P12引脚
#define P14_PIN 14         // P14引脚
#define P27_PIN 27         // P27引脚
#define P26_PIN 26         // P26引脚
#define P25_PIN 25         // P25引脚

// MQTT 配置
#define MQTT_SERVER "192.168.10.80"
#define MQTT_PORT 1883
#define MQTT_USER "ball"
#define MQTT_PASSWORD ""
#define MQTT_TOPIC_SUB "ball/triggered"

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

// 全局变量
unsigned long previousMillis = 0;
bool blinkState = false;    // 用于频闪效果
int breathState = 0;        // 用于呼吸效果
int breathDirection = 1;    // 呼吸方向：1为增加，-1为减少
bool allPinsTriggered = false;  // 标记所有指定引脚是否都已触发
bool previousAllPinsTriggered = false;  // 上次循环时的状态，用于检测状态变化

void setup() {
  // 初始化引脚为输入模式
  pinMode(P13_PIN, INPUT);
  pinMode(P12_PIN, INPUT);
  pinMode(P14_PIN, INPUT);
  pinMode(P27_PIN, INPUT);
  pinMode(P26_PIN, INPUT);
  pinMode(P25_PIN, INPUT);
  
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
}

void loop() {
  // 检测P13状态
  bool p13State = digitalRead(P13_PIN);
  
  // 检测P12, P14, P27, P26, P25状态
  bool p12State = digitalRead(P12_PIN);
  bool p14State = digitalRead(P14_PIN);
  bool p27State = digitalRead(P27_PIN);
  bool p26State = digitalRead(P26_PIN);
  bool p25State = digitalRead(P25_PIN);
  
  // 检查所有指定引脚是否都已触发
  allPinsTriggered = (p12State == HIGH && p14State == HIGH && 
                      p25State == HIGH && p26State == HIGH && p27State == HIGH);
  
  // 如果P13有信号，执行红色黄色频闪
  if (p13State == HIGH) {
    flashRedYellow();
  }
  // 如果P12, P14, P27, P26, P25都有信号，执行绿色呼吸
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
  
  // 如果所有指定引脚都已触发，且之前未触发，则发送空报文
  if (allPinsTriggered && !previousAllPinsTriggered) {
    // 发送空报文到 MQTT 主题
    client.publish("ball/triggered", ""); 
    Serial.println("已发送空报文到 ball/triggered");
  }
  
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
  Serial.print("正在连接到 WiFi...");
  
  // 这里假设连接到现有的 WiFi 网络
  // 如果需要指定网络名称和密码，请修改以下代码
  // WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi 连接成功");
  Serial.print("IP 地址: ");
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