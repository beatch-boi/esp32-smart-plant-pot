#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <LittleFS.h>
#include <TimeLib.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>
#include <map>
#include <functional>
#include <array>

// Log error macro
#define LOGE(ERR_MSG, ...) \
    (void)Serial.printf("E: " ERR_MSG "\n", ##__VA_ARGS__);

// Log info macro
#define LOGI(INFO_MSG, ...) \
    (void)Serial.printf("I: " INFO_MSG "\n", ##__VA_ARGS__);

// Log debug macro
#define LOGD(DEBUG_MSG, ...) \
    (void)Serial.printf("D: " DEBUG_MSG "\n", ##__VA_ARGS__);

// Translate hex color to RGB format
#define COLOR(HEX) \
    HEX >> 16 & 0xFF, HEX >> 8 & 0xFF, HEX & 0xFF

// RGB LED pins
#define R_LED_PIN 23
#define G_LED_PIN 19
#define B_LED_PIN 18

// I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

// Buzzer pin
#define BUZZER_PIN 13

// Hall sensor pin
#define HALL_SENSOR_PIN 14

// UV LEDs pins
#define UV1_PIN 26
#define UV2_PIN 27
#define UV3_PIN 4

const u8_t NUM_SECTIONS = 3;
const u8_t uvSections[NUM_SECTIONS] = {UV1_PIN, UV2_PIN, UV3_PIN};

// Wi-Fi credentials
const char ssid[] = "Infinity";
const char password[] = "Voltiqshop2020";

// AsyncWebServer on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Global constants
const u8_t maxTemperature = 40;
const u8_t minTemperature = 20;

const u8_t maxHumidity = 70;
const u8_t minHumidity = 20;

time_t startTime, startTimeOffset = 0;

bool timeSynchronized = false;

bool is_water_level_timer_active = false;
u32_t water_level_timer_start_time = 0;

JsonDocument Json;

enum class SectionStatus { Off, On };

struct Section {
    int brightness;
    SectionStatus status;
    String timer;
    String timeOn;
    String timeOff;
};

std::array<Section, NUM_SECTIONS> sections;

void setRGB(const u8_t r, const u8_t g, const u8_t b)
{
    analogWrite(R_LED_PIN, r);
    analogWrite(G_LED_PIN, g);
    analogWrite(B_LED_PIN, b);
}

float requestTemperature()
{
    u8_t CMD[3] = {0xAC, 0x33, 0x00};

    Wire.beginTransmission(0x38);
    Wire.write(CMD, 3);
    Wire.endTransmission();

    Wire.requestFrom(0x38, 6);

    u8_t data[6];

    for (u8_t i = 0; Wire.available() > 0; i++)
        data[i] = Wire.read();

    // u8_t i = 0;
    // while (Wire.available() > 0)
    // {
    //     data[i] = Wire.read();
    //     i++;
    // }

    // Formula to retrive temperature from the datasheet
    return (((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5]) * 0.000191 - 50;
}

float requestHumidity()
{
    u8_t CMD[3] = {0xAC, 0x33, 0x00};

    Wire.beginTransmission(0x38);
    Wire.write(CMD, 3);
    Wire.endTransmission();

    Wire.requestFrom(0x38, 6);

    u8_t data[6];

    for (u8_t i = 0; Wire.available() > 0; i++)
        data[i] = Wire.read();

    // u8_t i = 0;
    // while (Wire.available() > 0)
    // {
    //     data[i] = Wire.read();
    //     i++;
    // }

    // Formula to retrive humidity from the datasheet
    return (((data[1] << 16) | (data[2] << 8) | data[3]) >> 4) * 0.000095;
}

int checkWaterLevel()
{
    return !digitalRead(HALL_SENSOR_PIN);
}

u64_t millis64()
{
    static u32_t low32, high32;
    u32_t new_low32 = millis();

    if (new_low32 < low32)
        high32++;
    low32 = new_low32;

    return (u64_t)high32 << 32 | low32;
}

void handleWebSocketMessage(void *arg, u8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        data[len] = 0;
        String message = (char *)data;

        std::map<String, std::function<void()>> wsHandlers = {
            
            {"get-temperature", []() { ws.textAll("temperature:" + String(requestTemperature())); }},
            {"get-humidity", []() { ws.textAll("humidity:" + String(requestHumidity())); }},
            {"get-water-level", []() { ws.textAll("water-level:" + String(checkWaterLevel())); }},

            {"get-uv1-value", []() { ws.textAll("uv1-value:" + String(sections[0].brightness)); }},
            {"get-uv2-value", []() { ws.textAll("uv2-value:" + String(sections[1].brightness)); }},
            {"get-uv3-value", []() { ws.textAll("uv3-value:" + String(sections[2].brightness)); }},

            {"get-uv1-status", []() { ws.textAll("uv1-status:" + String(sections[0].status == SectionStatus::On ? "on" : "off")); }},
            {"get-uv2-status", []() { ws.textAll("uv2-status:" + String(sections[1].status == SectionStatus::On ? "on" : "off")); }},
            {"get-uv3-status", []() { ws.textAll("uv3-status:" + String(sections[2].status == SectionStatus::On ? "on" : "off")); }},

            {"get-time-on-1", []() { ws.textAll("time-on-1:" + sections[0].timeOn); }},
            {"get-time-on-2", []() { ws.textAll("time-on-2:" + sections[1].timeOn); }},
            {"get-time-on-3", []() { ws.textAll("time-on-3:" + sections[2].timeOn); }},

            {"get-time-off-1", []() { ws.textAll("time-off-1:" + sections[0].timeOff); }},
            {"get-time-off-2", []() { ws.textAll("time-off-2:" + sections[1].timeOff); }},
            {"get-time-off-3", []() { ws.textAll("time-off-3:" + sections[2].timeOff); }},

            {"get-name-1", []() { ws.textAll("name-1:" + Json["sections"][0]["name"].as<String>()); }},
            {"get-name-2", []() { ws.textAll("name-2:" + Json["sections"][1]["name"].as<String>()); }},
            {"get-name-3", []() { ws.textAll("name-3:" + Json["sections"][2]["name"].as<String>()); }},
        };

        if (wsHandlers.count(message)) {
            wsHandlers[message]();
        } else if (message.startsWith("uv1:")) {
            String value = message.substring(4);
            u8_t brightness = map(value.toInt(), 0, 100, 0, 255);

            sections[0].brightness = brightness;
            sections[0].status = brightness == 0 ? SectionStatus::Off : SectionStatus::On;
            analogWrite(uvSections[0], brightness);
            ws.textAll("uv1-status:" + String(sections[0].status == SectionStatus::On ? "on" : "off"));
        } else if (message.startsWith("uv2:")) {
            String value = message.substring(4);
            u8_t brightness = map(value.toInt(), 0, 100, 0, 255);

            sections[1].brightness = brightness;
            sections[1].status = brightness == 0 ? SectionStatus::Off : SectionStatus::On;
            analogWrite(uvSections[1], brightness);
            ws.textAll("uv2-status:" + String(sections[1].status == SectionStatus::On ? "on" : "off"));
        } else if (message.startsWith("uv3:")) {
            String value = message.substring(4);
            u8_t brightness = map(value.toInt(), 0, 100, 0, 255);

            sections[2].brightness = brightness;
            sections[2].status = brightness == 0 ? SectionStatus::Off : SectionStatus::On;
            analogWrite(uvSections[2], brightness);
            ws.textAll("uv3-status:" + String(sections[2].status == SectionStatus::On ? "on" : "off"));
        }
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, u8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        LOGI("WebSocket client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());
        break;

    case WS_EVT_DISCONNECT:
        LOGI("WebSocket client #%u disconnected", client->id());
        break;

    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void JsonSave()
{
    File f = LittleFS.open("/sections.json", FILE_WRITE);
    size_t bytesWritten = serializeJson(Json, f);
    LOGI("Saved %zu bytes", bytesWritten);
    f.close();
}

void JsonLoad()
{
    File f = LittleFS.open("/sections.json", FILE_READ);
    DeserializationError result = deserializeJson(Json, f);
    if (result != DeserializationError::Ok)
    {
        LOGE("DeserializationError %d", result);
    }
    else
    {
        LOGI("Loaded successfully!");
    }
    f.close();

    for (u8_t i = 0; i < NUM_SECTIONS; i++)
        if (Json["sections"][i]["status"].as<String>() == "on")
            analogWrite(uvSections[i], Json["sections"][i]["brightness"].as<int>());
}

void uvSectionApplySettings(AsyncWebServerRequest *request, JsonVariant &json, u8_t id)
{
    JsonDocument jsonDoc;

    if (json.is<JsonArray>())
        jsonDoc = json.as<JsonArray>();
    else if (json.is<JsonObject>())
        jsonDoc = json.as<JsonObject>();
    else
        return;

    struct tm tm_on;
    const char *switch_on_time = jsonDoc["time-on"];

    struct tm tm_off;
    const char *switch_off_time = jsonDoc["time-off"];

    String name = jsonDoc["name"].as<String>();
    int brightness = jsonDoc["brightness"].as<int>();

    if (strptime(switch_on_time, "%Y-%m-%d %H:%M:%S", &tm_on) == NULL)
        return request->send(500, "application/json", "{\"message\":\"Wrong time format\"}");

    if (strptime(switch_off_time, "%Y-%m-%d %H:%M:%S", &tm_off) == NULL)
        return request->send(500, "application/json", "{\"message\":\"Wrong time format\"}");
        send_P(200, "application/json", "{\"message\":\"Wrong time format\"}");
    char buf[6];

    time_t on_time = mktime(&tm_on);
    sprintf(buf, "%02u:%02u", hour(on_time), minute(on_time));
    Json["sections"][id]["timeOn"] = buf;

    time_t off_time = mktime(&tm_off);
    sprintf(buf, "%02u:%02u", hour(off_time), minute(off_time));
    Json["sections"][id]["timeOff"] = buf;

    Json["sections"][id]["name"] = name;
    Json["sections"][id]["brightness"] = map(brightness, 0, 100, 0, 255);

    if (on_time == off_time)
        Json["sections"][id]["timer"] = "notSet";
    else
        Json["sections"][id]["timer"] = "set";

    JsonSave();

    request->send(200, "application/json", "{\"message\":\"Ok\"}");
}

void setup()
{
    pinMode(R_LED_PIN, OUTPUT);
    pinMode(G_LED_PIN, OUTPUT);
    pinMode(B_LED_PIN, OUTPUT);

    for (u8_t i = 0; i < NUM_SECTIONS; i++)
        pinMode(uvSections[i], OUTPUT);

    pinMode(BUZZER_PIN, OUTPUT);

    Wire.begin(SDA_PIN, SCL_PIN);

    Serial.begin(115200);

    LittleFS.begin(true);

    JsonLoad();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.hostname("SmartPlantPot");

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        LOGE("Could not connect to WiFi, ssid=%s, password=%s. Going to AP mode.", ssid, password);
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(IPAddress(192, 168, 4, 4), IPAddress(192, 168, 4, 4), IPAddress(255, 255, 255, 0));
        WiFi.softAP("SmartPot", "VoltiqSmart");
        LOGI("Wifi AP started, ssid=SmartPot, password=VoltiqSmart, ip=%s", WiFi.softAPIP().toString().c_str());
    }
    else
    {
        LOGI("Connected to Wifi, ssid=%s, ip=%s", ssid, WiFi.localIP().toString().c_str());
    }

    server.serveStatic("/favicon.ico", LittleFS, "/img/favicon.ico");

    server.serveStatic("/img/heart.svg", LittleFS, "/img/heart.svg");
    server.serveStatic("/img/trash.svg", LittleFS, "/img/trash.svg");
    server.serveStatic("/img/wishlist-heart.svg", LittleFS, "/img/wishlist-heart.svg");

    server.serveStatic("/css/base-min.css", LittleFS, "/css/base-min.css");
    server.serveStatic("/css/grids-min.css", LittleFS, "/css/grids-min.css");
    server.serveStatic("/css/buttons-min.css", LittleFS, "/css/buttons-min.css");
    server.serveStatic("/css/grids-responsive-min.css", LittleFS, "/css/grids-responsive-min.css");
    
    server.serveStatic("/scripts/main.js", LittleFS, "/scripts/main.js");
    server.serveStatic("/scripts/sockets.js", LittleFS, "/scripts/sockets.js");
    server.serveStatic("/scripts/utils.js", LittleFS, "/scripts/utils.js");

    AsyncCallbackJsonWebHandler *timesSynchronizeHandler = new AsyncCallbackJsonWebHandler("/time/synchronize", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                           {
        JsonDocument jsonDoc;

        if (json.is<JsonArray>())
            jsonDoc = json.as<JsonArray>();
        else if (json.is<JsonObject>())
            jsonDoc = json.as<JsonObject>();
        else 
            return;

        struct tm tm;
        const char *dateTimeStr = jsonDoc["datetime"];

        if (strptime(dateTimeStr, "%Y-%m-%d %H:%M:%S", &tm) != NULL) 
        {
            startTime = mktime(&tm);
            startTimeOffset = millis64() / 1000;

            LOGI("Time synchronized successfully");

            timeSynchronized = true;
            request->send(200, "application/json", "{\"message\":\"Ok\"}");
        }
        else 
        {
            LOGI("Time synchronize failed");

            timeSynchronized = false;
            request->send(500, "application/json", "{\"message\":\"Wrong time format\"}");
        } });

    AsyncCallbackJsonWebHandler *uv1_timer_set_handler = new AsyncCallbackJsonWebHandler("/group-1/apply", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                         { uvSectionApplySettings(request, json, 0); });

    AsyncCallbackJsonWebHandler *uv2_timer_set_handler = new AsyncCallbackJsonWebHandler("/group-2/apply", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                         { uvSectionApplySettings(request, json, 1); });

    AsyncCallbackJsonWebHandler *uv3_timer_set_handler = new AsyncCallbackJsonWebHandler("/group-3/apply", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                         { uvSectionApplySettings(request, json, 2); });

    server.on("/presets/get", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/presets.json", "application/json");
        request->send(response); });

    AsyncCallbackJsonWebHandler *savePresetHandler = new AsyncCallbackJsonWebHandler("/preset/save", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                     {
        JsonDocument jsonDoc;

        if (json.is<JsonArray>())
            jsonDoc = json.as<JsonArray>();
        else if (json.is<JsonObject>())
            jsonDoc = json.as<JsonObject>();
        else 
            return;

        File presetsFile= LittleFS.open("/presets.json", FILE_READ);
        if (!presetsFile)
            return request->send(500, "application/json", "{\"message\":\"File \"presets.json\" does not exist\"}");

        JsonDocument presetsJson;
        deserializeJson(presetsJson, presetsFile);

        presetsFile.close();

        JsonArray presets = presetsJson["presets"];
        
        JsonObject newPreset = presets.add<JsonObject>();
        newPreset["name"] = jsonDoc["name"];
        newPreset["time-on"] = jsonDoc["time-on"];
        newPreset["time-off"] = jsonDoc["time-off"];
        newPreset["brightness"] = jsonDoc["brightness"];

        presetsFile = LittleFS.open("/presets.json", FILE_WRITE);
        serializeJson(presetsJson, presetsFile);
        presetsFile.close();

        request->send(200, "application/json", "{\"message\":\"Ok\"}"); });

    AsyncCallbackJsonWebHandler *deletePresetHandler = new AsyncCallbackJsonWebHandler("/preset/delete", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                       {
        JsonDocument jsonDoc;

        if (json.is<JsonArray>())
            jsonDoc = json.as<JsonArray>();
        else if (json.is<JsonObject>())
            jsonDoc = json.as<JsonObject>();
        else
            return;

        JsonDocument presetsJson;
        File f = LittleFS.open("/presets.json", FILE_READ);
        if (!f) 
            return request->send(500, "application/json", "{\"message\":\"File \"presets.json\" does not exist\"}");
        
        deserializeJson(presetsJson, f);
        f.close();

        int id = jsonDoc["id"];

        JsonArray presets = presetsJson["presets"];
        size_t index = 0;
        for (JsonArray::iterator it = presets.begin(); it != presets.end(); ++it)
        {
            if (index == id)
            {
                presets.remove(it);
                break;
            }
            index++;
        }

        f = LittleFS.open("/presets.json", FILE_WRITE);
        serializeJson(presetsJson, f);
        f.close();

        request->send(200, "application/json", "{\"message\":\"Ok\"}"); });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html", "text/html");
        request->send(response); });

    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.addHandler(timesSynchronizeHandler);
    server.addHandler(savePresetHandler);
    server.addHandler(deletePresetHandler);

    server.addHandler(uv1_timer_set_handler);
    server.addHandler(uv2_timer_set_handler);
    server.addHandler(uv3_timer_set_handler);

    AsyncElegantOTA.begin(&server);

    server.begin();

    LOGI("Boot completed successfully");
}

bool isConditionOk(int value, int min, int max) {
    return value >= min && value <= max;
}

void updateSection(u8_t i) {
    const int sectionBrightness = sections[i].brightness;
    const SectionStatus sectionStatus = sections[i].status;
    const String sectionTimer = sections[i].timer;

    if (sectionTimer == "set" && sectionBrightness > 0) {
        u8_t startHour, startMinute;
        sscanf(sections[i].timeOn.c_str(), "%hhd:%hhd", &startHour, &startMinute);

        u8_t endHour, endMinute;
        sscanf(sections[i].timeOff.c_str(), "%hhd:%hhd", &endHour, &endMinute);

        time_t currentTime = startTime + ((millis64() / 1000) - startTimeOffset);
        u8_t currentMinute = minute(currentTime);
        u8_t currentHour = hour(currentTime);

        bool mustBeActive = (currentHour > startHour || (currentHour == startHour && currentMinute >= startMinute)) &&
                            (currentHour < endHour || (currentHour == endHour && currentMinute < endMinute));

        // If UV must be ON but it is OFF, switch it ON
        if (mustBeActive && sectionStatus == SectionStatus::Off) {
            sections[i].status = SectionStatus::On;
            analogWrite(uvSections[i], sectionBrightness);
            JsonSave();
            ws.textAll("uv" + String(i + 1) + "-status:" + (sections[i].status == SectionStatus::On ? "on" : "off"));
        }
        // If UV must be OFF but it is ON, switch it OFF
        else if (!mustBeActive && sectionStatus == SectionStatus::On) {
            sections[i].status = SectionStatus::Off;
            analogWrite(uvSections[i], 0);
            JsonSave();
            ws.textAll("uv" + String(i + 1) + "-status:" + (sections[i].status == SectionStatus::On ? "on" : "off"));
        }
    } else if (sectionBrightness <= 0 && sectionStatus == SectionStatus::On) {
        sections[i].status = SectionStatus::Off;
    }
}

void loop()
{
    ws.cleanupClients();

    // if (Serial.available())
    // {
    //     String cmd = Serial.readStringUntil('\n');
    //     if (cmd == "print")
    //     {
    //         LOGD("is_water_level_timer_active=%s", is_water_level_timer_active ? "true" : "false");
    //         LOGD("water_level_timer_start_time=%ld", water_level_timer_start_time);

    //         serializeJsonPretty(Json, Serial);
    //     }
    //     else if (cmd = "presets")
    //     {
    //         File f = LittleFS.open("/presets.json", FILE_READ);
    //         JsonDocument jj;
    //         deserializeJson(jj, f);
    //         f.close();
    //         serializeJsonPretty(jj, Serial);
    //     }
    // }

    if (!timeSynchronized)
        return setRGB(COLOR(0xF032E6));

    // Static variables are automaticly initialized to zero
    static int previousTemperature;
    static int previousHumidity;
    static int previousWaterLevel;

    int temperature = requestTemperature();
    int humidity = requestHumidity();
    int waterLevel = checkWaterLevel();

    if (temperature != previousTemperature)
    {
        previousTemperature = temperature;
        if (ws.availableForWriteAll())
            ws.textAll("temperature:" + String(temperature));
    }

    if (humidity != previousHumidity)
    {
        previousHumidity = humidity;
        if (ws.availableForWriteAll())
            ws.textAll("humidity:" + String(humidity));
    }

    if (waterLevel != previousWaterLevel)
    {
        previousWaterLevel = waterLevel;
        if (ws.availableForWriteAll())
            ws.textAll("water-level:" + String(waterLevel));
    }

    bool isWaterLevelOk = (waterLevel == 0);
    bool isTemperatureOk = isConditionOk(temperature, minTemperature, maxTemperature);
    bool isHumidityOk = isConditionOk(humidity, minHumidity, maxHumidity);

    if (!isWaterLevelOk)
    {
        setRGB(COLOR(0xFF5733));

        if (!is_water_level_timer_active)
        {
            // Start water level timer
            is_water_level_timer_active = true;
            water_level_timer_start_time = startTime + ((millis64() / 1000) - startTimeOffset);
        }

        time_t currentTime = startTime + ((millis64() / 1000) - startTimeOffset);
        time_t elapsed_time_since_water_level_timer_started = currentTime - water_level_timer_start_time;

        // If water has not been filled in 24 hours
        if (elapsed_time_since_water_level_timer_started > 24L * 3600L)
        {
            // Set buzzer alarm to beep every minute
            int beepInterval = 1000 * 60;                 // 1 minute
            int threshold = 2;                            // 2 seconds
            int untillNextBeep = millis() % beepInterval; // in seconds

            if (untillNextBeep >= 0 && untillNextBeep <= threshold)
            {
                tone(BUZZER_PIN, 660, 100);
                delayMicroseconds(200000);
                tone(BUZZER_PIN, 660, 100);
                delayMicroseconds(500000);
                tone(BUZZER_PIN, 660, 100);
                delayMicroseconds(200000);
                tone(BUZZER_PIN, 660, 100);
            }
        }
    }
    else
    {
        if (isTemperatureOk && isHumidityOk)
            setRGB(COLOR(0x3CB44B));
        else
            setRGB(COLOR(0x72C2FF));

        if (is_water_level_timer_active)
        {
            is_water_level_timer_active = false;
            water_level_timer_start_time = 0;
        }
    }

    for (u8_t i = 0; i < NUM_SECTIONS; i++) {
        updateSection(i);
    }
}
