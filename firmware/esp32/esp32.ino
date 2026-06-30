// **************** 接入AI ******************
// ESP32-S3 智慧大棚环境监测系统
// 功能：获取天气 + 接收STM32传感器数据 + AI分析建议 + Web展示
// 使用前请在下方填写您自己的 WiFi、API Key 等配置信息

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <WebServer.h>

// ========== OLED 配置 ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCL_PIN 5
#define SDA_PIN 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ========== Web服务器配置 ==========
WebServer server(80);

// ========== WiFi 配置 ==========
const char* ssid = "YOUR_WIFI_SSID";           // 请填写您的WiFi名称
const char* password = "YOUR_WIFI_PASSWORD"; // 请填写您的WiFi密码

// ========== 心知天气配置 ==========
const char* apiKey = "YOUR_SENIVERSE_API_KEY";  // 请填写您的心知天气 API Key (https://www.seniverse.com)
String location = "beijing";                      // 城市名

// ========== AI配置 ==========
const char* aiApiKey = "YOUR_SILICONFLOW_API_KEY"; // 请填写您的 SiliconFlow API Key (https://siliconflow.cn)
String g_ai_advice = "正在分析...";
unsigned long lastAiFetch = 0;

// ========== 全局变量 ==========
String g_weather = "---";
String g_temp = "--";
float g_temperature_float = 0;
String g_last_weather = "";
String g_last_temp = "";

// 传感器数据（从STM32接收）
float indoor_temp = 0;
float indoor_humidity = 0;
int light_intensity = 0;
int soil_fertility = 0;

bool sensor_data_updated = false;

// ========== 函数声明 ==========
String extractValue(String json, String keyStart, String keyEnd);
void sendToSTM32(String data);
void updateOLED();
void initOLED();
void connectWiFi();
void fetchWeather();
void fetchAIAdvice();
String getLocalAdvice();
void handleRoot();
void handleData();
void handleAdvice();
void parseSTM32Data(String data);

// ========== 发送到STM32 ==========
void sendToSTM32(String data) {
    Serial1.print(data);
    Serial1.print("\r\n");
    Serial.print("Sent to STM32: ");
    Serial.println(data);
}

// ========== 解析STM32传感器数据 ==========
void parseSTM32Data(String data) {
    data.trim();

    int tempIdx = data.indexOf("TEMP,");
    int humiIdx = data.indexOf("HUMI,");
    int lightIdx = data.indexOf("LIGHT,");
    int soilIdx = data.indexOf("SOIL,");

    if (tempIdx != -1) {
        int start = tempIdx + 5;
        int end = data.indexOf(",", start);
        if (end == -1) end = data.length();
        indoor_temp = data.substring(start, end).toFloat();
    }
    if (humiIdx != -1) {
        int start = humiIdx + 5;
        int end = data.indexOf(",", start);
        if (end == -1) end = data.length();
        indoor_humidity = data.substring(start, end).toFloat();
    }
    if (lightIdx != -1) {
        int start = lightIdx + 6;
        int end = data.indexOf(",", start);
        if (end == -1) end = data.length();
        light_intensity = data.substring(start, end).toInt();
    }
    if (soilIdx != -1) {
        int start = soilIdx + 5;
        int end = data.indexOf(",", start);
        if (end == -1) end = data.length();
        soil_fertility = data.substring(start, end).toInt();
    }

    sensor_data_updated = true;
    Serial.printf("STM32: T=%.1f, H=%.1f, L=%d, S=%d\n",
                  indoor_temp, indoor_humidity, light_intensity, soil_fertility);
}

// ========== 提取JSON值 ==========
String extractValue(String json, String keyStart, String keyEnd) {
    int startIdx = json.indexOf(keyStart);
    if (startIdx == -1) return "Unknown";
    startIdx += keyStart.length();
    int endIdx = json.indexOf(keyEnd, startIdx);
    if (endIdx == -1) return "Unknown";
    return json.substring(startIdx, endIdx);
}

// ========== 本地智能建议 ==========
String getLocalAdvice() {
    String advice = "";

    // 温度
    if (indoor_temp > 35) advice += "🔴 温度过高！开启通风或遮阳。";
    else if (indoor_temp > 30) advice += "🟡 温度偏高，注意通风。";
    else if (indoor_temp < 10) advice += "🔴 温度过低！开启加温设备。";
    else if (indoor_temp < 15) advice += "🟡 温度偏低，注意保温。";
    else advice += "🟢 温度适宜。";

    // 湿度
    if (indoor_humidity > 90) advice += " 湿度太高，通风除湿。";
    else if (indoor_humidity > 80) advice += " 湿度偏高。";
    else if (indoor_humidity < 30) advice += " 湿度太低，喷雾加湿。";
    else if (indoor_humidity < 40) advice += " 湿度偏低。";
    else advice += " 湿度正常。";

    // 光照
    if (light_intensity > 50000) advice += " 光照过强，建议遮阳。";
    else if (light_intensity > 0 && light_intensity < 3000) advice += " 光照不足，考虑补光。";

    // 土壤
    if (soil_fertility > 1000) advice += " 肥力过高，暂停施肥。";
    else if (soil_fertility > 0 && soil_fertility < 80) advice += " 肥力偏低，适当施肥。";

    // 内外对比
    if (g_temperature_float > indoor_temp + 5) advice += " 室外更暖，可开窗。";
    else if (g_temperature_float < indoor_temp - 5) advice += " 室外更冷，注意保温。";

    if (advice == "") advice = "🟢 一切正常，无需操作。";
    return advice;
}

// ========== 调用AI分析 ==========
void fetchAIAdvice() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (g_weather == "---") return;

    HTTPClient http;
    http.setTimeout(15000);

    String prompt = "你是农业专家。当前温室：室外" + g_temp + "℃ " + g_weather +
                    "，室内" + String(indoor_temp,1) + "℃ 湿度" + String(indoor_humidity,0) +
                    "% 光照" + String(light_intensity) + "Lux 土壤肥力" + String(soil_fertility) +
                    "ppm。给一句种植建议（30字内）。";

    http.begin("https://api.siliconflow.cn/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(aiApiKey));

    String body = "{\"model\":\"Qwen/Qwen2.5-7B-Instruct\",\"messages\":[{\"role\":\"user\",\"content\":\"" + prompt + "\"}],\"max_tokens\":80}";

    int httpCode = http.POST(body);

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        String advice = extractValue(payload, "\"content\":\"", "\"");
        advice.replace("\\n", " ");
        if (advice != "Unknown" && advice.length() > 0) {
            g_ai_advice = advice;
        }
        Serial.println("AI: " + g_ai_advice);
    } else {
        Serial.printf("AI failed: %d\n", httpCode);
        g_ai_advice = getLocalAdvice();  // 失败用本地建议
    }
    http.end();
}

// ========== 更新OLED显示 ==========
void updateOLED() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("ESP32 Gateway");
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    display.setCursor(0, 14);
    display.print("Out: ");
    display.print(g_weather);
    display.print(" ");
    display.print(g_temp);
    display.println("C");

    display.setCursor(0, 24);
    display.print("In: ");
    if (indoor_temp > -100) {
        display.print(indoor_temp, 1);
    } else {
        display.print("--");
    }
    display.print("C ");
    if (indoor_humidity > 0) {
        display.print(indoor_humidity, 0);
    } else {
        display.print("--");
    }
    display.println("%");

    display.setCursor(0, 34);
    display.print("Light: ");
    if (light_intensity > 0) {
        display.print(light_intensity);
    } else {
        display.print("--");
    }
    display.println(" Lux");

    display.setCursor(0, 44);
    display.print("Soil: ");
    if (soil_fertility > 0) {
        display.print(soil_fertility);
    } else {
        display.print("--");
    }
    display.println(" ppm");

    display.setCursor(0, 54);
    display.print("WiFi:OK");

    display.display();
}

// ========== 初始化OLED ==========
void initOLED() {
    Wire.begin(SDA_PIN, SCL_PIN);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED init failed!");
        return;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ESP32-S3");
    display.println("Weather Station");
    display.println("Starting...");
    display.display();
    delay(2000);
    Serial.println("OLED init OK");
}

// ========== 连接WiFi ==========
void connectWiFi() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connecting to");
    display.println(ssid);
    display.display();

    WiFi.begin(ssid, password);
    Serial.print("Connecting WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi OK!");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
    delay(2000);
}

// ========== 获取天气数据 ==========
void fetchWeather() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.setTimeout(5000);

    String url = "https://api.seniverse.com/v3/weather/now.json?key=" + String(apiKey) +
                 "&location=" + location + "&language=en&unit=c";

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        String weather = extractValue(payload, "\"text\":\"", "\"");
        String temp = extractValue(payload, "\"temperature\":\"", "\"");

        if (weather != "Unknown" && temp != "Unknown") {
            g_weather = weather;
            g_temp = temp;
            g_temperature_float = temp.toFloat();

            String dataToSend = weather + "," + temp;

            Serial.print("Weather: ");
            Serial.println(dataToSend);

            if (weather != g_last_weather || temp != g_last_temp) {
                sendToSTM32(dataToSend);
                g_last_weather = weather;
                g_last_temp = temp;
            }

            updateOLED();
        }
    } else {
        Serial.print("HTTP failed: ");
        Serial.println(httpCode);
    }
    http.end();
}

// ========== Web服务器 - 首页 ==========
void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=yes">
    <title>智慧大棚环境监测系统</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', 'PingFang SC', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1a5f7a 0%, #2d8a4e 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container { max-width: 600px; margin: 0 auto; }
        .header { text-align: center; margin-bottom: 25px; }
        .header h1 { font-size: 1.8em; color: white; text-shadow: 2px 2px 4px rgba(0,0,0,0.2); letter-spacing: 2px; }
        .header p { color: rgba(255,255,255,0.8); font-size: 0.8em; margin-top: 5px; }
        .weather-card {
            background: linear-gradient(135deg, #ffffff 0%, #e8f4f8 100%);
            border-radius: 30px;
            padding: 25px 20px;
            margin-bottom: 20px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.15);
            text-align: center;
        }
        .weather-icon { font-size: 64px; margin-bottom: 5px; }
        .temp-large { font-size: 52px; font-weight: bold; color: #e74c3c; line-height: 1; }
        .weather-desc { font-size: 18px; color: #555; margin-top: 8px; }
        .card { background: white; border-radius: 20px; padding: 20px; margin-bottom: 15px; box-shadow: 0 5px 15px rgba(0,0,0,0.1); }
        .card-title { font-size: 16px; font-weight: bold; color: #2c5f2d; padding-bottom: 10px; border-bottom: 2px solid #e0e0e0; margin-bottom: 15px; display: flex; align-items: center; gap: 8px; }
        .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        .data-item { text-align: center; padding: 10px; background: #f8f9fa; border-radius: 15px; }
        .data-label { font-size: 12px; color: #888; margin-bottom: 5px; }
        .data-value { font-size: 22px; font-weight: bold; color: #333; }
        .data-unit { font-size: 12px; color: #999; }
        .advice-box {
            background: linear-gradient(135deg, #fff3cd, #ffeaa7);
            border-radius: 15px;
            padding: 15px;
            margin-bottom: 15px;
            font-size: 14px;
            color: #333;
            line-height: 1.6;
            border-left: 4px solid #f39c12;
        }
        .advice-box .ai-label {
            font-size: 11px;
            color: #999;
            margin-bottom: 5px;
        }
        .status-bar {
            background: rgba(255,255,255,0.9);
            border-radius: 15px;
            padding: 12px 15px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 12px;
            color: #666;
        }
        .online-dot {
            display: inline-block;
            width: 8px;
            height: 8px;
            background-color: #2ecc71;
            border-radius: 50%;
            margin-right: 5px;
            animation: pulse 1.5s infinite;
        }
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
        .footer { text-align: center; color: rgba(255,255,255,0.7); font-size: 11px; margin-top: 20px; }
        @media (max-width: 480px) {
            .temp-large { font-size: 38px; }
            .weather-icon { font-size: 40px; }
            .data-value { font-size: 16px; }
        }
    </style>
    <script>
        function fetchData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('weather').innerHTML = data.weather;
                    document.getElementById('temp').innerHTML = data.temp;
                    document.getElementById('indoor_temp').innerHTML = data.indoor_temp;
                    document.getElementById('indoor_humi').innerHTML = data.indoor_humi;
                    document.getElementById('light').innerHTML = data.light;
                    document.getElementById('soil').innerHTML = data.soil;
                    document.getElementById('update_time').innerHTML = new Date().toLocaleString();
                })
                .catch(err => console.log(err));

            fetch('/advice')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('advice_text').innerHTML = data.advice;
                    document.getElementById('advice_source').innerHTML = data.source;
                })
                .catch(err => console.log(err));
        }
        setInterval(fetchData, 3000);
        window.onload = fetchData;
    </script>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌾 智慧大棚监控系统</h1>
            <p>Smart Greenhouse Monitoring System</p>
        </div>

        <div class="advice-box">
            <div class="ai-label">🤖 AI 智能建议</div>
            <div id="advice_text">分析中...</div>
            <div style="font-size:10px;color:#999;margin-top:5px;">来源: <span id="advice_source">--</span></div>
        </div>

        <div class="weather-card">
            <div class="weather-icon">🌤️</div>
            <div class="temp-large"><span id="temp">--</span><span class="unit">°C</span></div>
            <div class="weather-desc"><span id="weather">--</span></div>
            <div style="font-size: 12px; color: #999; margin-top: 10px;">室外天气</div>
        </div>

        <div class="card">
            <div class="card-title">🌡️ 室内环境监测</div>
            <div class="grid-2">
                <div class="data-item">
                    <div class="data-label">🌡️ 温度</div>
                    <div class="data-value"><span id="indoor_temp">--</span><span class="data-unit">°C</span></div>
                </div>
                <div class="data-item">
                    <div class="data-label">💧 湿度</div>
                    <div class="data-value"><span id="indoor_humi">--</span><span class="data-unit">%</span></div>
                </div>
            </div>
        </div>

        <div class="card">
            <div class="card-title">🌿 土壤与光照</div>
            <div class="grid-2">
                <div class="data-item">
                    <div class="data-label">☀️ 光照强度</div>
                    <div class="data-value"><span id="light">--</span><span class="data-unit">Lux</span></div>
                </div>
                <div class="data-item">
                    <div class="data-label">🌱 土壤肥力</div>
                    <div class="data-value"><span id="soil">--</span><span class="data-unit">ppm</span></div>
                </div>
            </div>
        </div>

        <div class="status-bar">
            <span><span class="online-dot"></span>系统在线</span>
            <span>📡 WiFi已连接</span>
            <span>🔄 数据实时更新</span>
        </div>

        <div class="footer">
            最后更新: <span id="update_time">--:--:--</span><br>
            ESP32-S3 智慧农业监测系统 | AI智能分析
        </div>
    </div>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
}

// ========== Web服务器 - 获取JSON数据 ==========
void handleData() {
    String json = "{"
        "\"weather\":\"" + g_weather + "\","
        "\"temp\":" + String(g_temperature_float, 1) + ","
        "\"indoor_temp\":" + String(indoor_temp, 1) + ","
        "\"indoor_humi\":" + String(indoor_humidity, 0) + ","
        "\"light\":" + String(light_intensity) + ","
        "\"soil\":" + String(soil_fertility) +
    "}";
    server.send(200, "application/json", json);
}

// ========== Web服务器 - AI建议 ==========
void handleAdvice() {
    String source;
    String advice;

    if (g_ai_advice != "正在分析..." && g_ai_advice.length() > 0) {
        advice = g_ai_advice;
        source = "云端AI";
    } else {
        advice = getLocalAdvice();
        source = "本地分析";
    }

    String json = "{"
        "\"advice\":\"" + advice + "\","
        "\"source\":\"" + source + "\""
    "}";
    server.send(200, "application/json", json);
}

// ========== Setup ==========
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, 44, 43);

    initOLED();
    connectWiFi();
    fetchWeather();

    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.on("/advice", handleAdvice);
    server.begin();

    Serial.println("\n==========================================");
    Serial.println("ESP32 Ready!");
    Serial.print("Web: http://");
    Serial.println(WiFi.localIP());
    Serial.println("Serial1: RX=GPIO44, TX=GPIO43");
    Serial.println("==========================================");

    updateOLED();
}

// ========== Loop ==========
void loop() {
    static unsigned long lastFetch = 0;
    unsigned long now = millis();

    server.handleClient();

    // 接收STM32串口数据
    if (Serial1.available()) {
        String received = Serial1.readStringUntil('\n');
        if (received.length() > 0) {
            received.trim();
            Serial.print("STM32 -> ");
            Serial.println(received);

            if (received.startsWith("DATA")) {
                parseSTM32Data(received);
                updateOLED();
            }
        }
    }

    // 每30秒获取一次天气
    if (now - lastFetch > 30000) {
        lastFetch = now;
        fetchWeather();
        updateOLED();
    }

    // 每5分钟调用AI
    if (now - lastAiFetch > 300000) {
        lastAiFetch = now;
        fetchAIAdvice();
    }

    delay(10);
}
