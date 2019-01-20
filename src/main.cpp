#include <Arduino.h>
#include <FS.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
//#include <Ticker.h>
#include <ArduinoJson.h>
#include <time.h>

extern "C" {
#include "libb64/cdecode.h"
}

// - To format the SPIFFS on setup.
//#define FORMAT_SPIFFS
// - Set the keepalive at 10min
// - Note: Google IoT core disconnects a device if no message is send during 20min
#define MQTT_KEEPALIVE 600
#include <PubSubClient.h>

#include "ciotc_config.h"
#include "strip_ctrl.h"
#include "device_ctrl.h"

#define DELAY_MS_EVENTS 60000
#define DELAY_MS_MQTT_RETRY 5000

#define PIXEL_PIN     D4
#define TEMP_PWR_PIN  D8
#define TEMP_READ_PIN A0

#define PIXEL_COUNT   3

// resistance at 25 degrees C
#define THERMISTOR_NOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMP_NOMINAL 25
// how many samples to take and average
#define TEMP_NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define TEMP_SERIESRESISTOR 10000

//const Ticker ticker;

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
StripCtrl stripCtrl(PIXEL_COUNT, PIXEL_PIN);
DeviceCtrl deviceCtrl;

WiFiManager wifiManager;

bool g_stateChanged = false;
long g_lastStateMs = 0;
long g_lastEventMs = 0;
long g_bootTime = 0;
uint8_t g_brightness = 100;
uint32_t g_color = Adafruit_NeoPixel::Color(255, 255, 255);
bool g_on = true;
double g_temperature = 0;
unsigned short g_mqttFail = 0;

//////////////////////////////////////////////
int b64decode(String b64Text, uint8_t* output);
void mqttCallback(char *topic, uint8_t *payload, unsigned int length);
bool mqttReconnect();
bool readConfigFile();
bool initialize();
void processJsonMessage(JsonObject& root);
void publishState();
void publishEvent();
void updateTemperature();
//////////////////////////////////////////////

WiFiEventHandler disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
  Serial.println("Station disconnected");
  // TODO
});

WiFiEventHandler gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
  Serial.println("Station got IP");
  if(WiFi.getMode() == WIFI_STA) {
    // TODO
  }
});

/////////////////////////////////////////////

void setup() {
  Serial.begin(115200);

  //Serial.setDebugOutput(true);

  pinMode(TEMP_PWR_PIN, OUTPUT);
  digitalWrite(TEMP_PWR_PIN, LOW);

  // - Strip initialization
  stripCtrl.begin();
  stripCtrl.animate(StripMode::BOOT);

#ifdef FORMAT_SPIFFS
  SPIFFS.format();
#endif

  updateTemperature();

  if(!initialize()) {
    stripCtrl.animate(StripMode::ERROR);
    delay(3000);
    ESP.reset();
    delay(5000);
  }
}

void loop() {
  if (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
    if(!mqttReconnect()) {
      Serial.print("MQTT connection failed, status code =");
      Serial.println(mqttClient.state());
      g_mqttFail++;
      delay(DELAY_MS_MQTT_RETRY); // Wait few seconds before retrying

      // - The connection issue is displayed only after trying 12 times
      if(g_mqttFail > 12) {
        stripCtrl.animate(StripMode::CONNECTION_IN_PROGRESS);
      }
    } else {
      g_mqttFail = 0;
    }
  }

  if (mqttClient.connected() && deviceCtrl.isTokenExpired()) {
    Serial.println("Token is expired but still connected. Force deconnection !");
    mqttClient.disconnect();
  }

  if (mqttClient.connected()) {
    if (millis() - g_lastEventMs > DELAY_MS_EVENTS) {
      g_lastEventMs = millis();

      updateTemperature();

      Serial.println("Publishing event");
      publishEvent();
    }

    if(g_stateChanged) {
      g_stateChanged = false;
      if(g_on) {
        if(g_brightness > 99) {
          stripCtrl.setPixelsColor(g_color);
        } else {
          stripCtrl.setPixelsColor(g_color, g_brightness);
        }
      } else {
        stripCtrl.setPixelsColor(Adafruit_NeoPixel::Color(0, 0, 0));
      }
      stripCtrl.show();

      if(millis() - g_lastStateMs < 5000) {
        // State cannot be published more than once each second
        delay(2000);
      }
      g_lastStateMs = millis();

      Serial.println("Publishing state");
      publishState();
    }
  }

  mqttClient.loop();

  // had some issues on the PubSubClient without some delay
  delay(10);
}

bool initialize() {
    // - Read config file
    if(!readConfigFile()) {
      Serial.println("Failed to read config file");
      return false;
    }

    //reset settings - for testing
    //wifiManager.resetSettings();

    //set minimu quality of signal so it ignores AP's under that quality
    //defaults to 8%
    //wifiManager.setMinimumSignalQuality();

    stripCtrl.animate(StripMode::ACCESS_POINT);

    String apName = String("NightLight-") + ESP.getChipId();
    if (!wifiManager.autoConnect(apName.c_str())) {
      Serial.println("Failed to connect to WiFi and hit timeout");
      return false;
    }

    Serial.println("Connected to WiFi");
    Serial.print("Local ip: ");
    Serial.println(WiFi.localIP());

    stripCtrl.animate(StripMode::CONNECTION_IN_PROGRESS);

    // - Update time
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting on time sync...");
    while (time(nullptr) < 1510644967) {
      delay(10);
    }
    g_bootTime = time(nullptr);

    // - MQTT Configuration
    mqttClient.setServer(deviceCtrl.getMqttHost(), deviceCtrl.getMqttPort());
    mqttClient.setCallback(mqttCallback);

/*
    //uint8_t binaryCert[root_cert.length()]; // with String
    uint8_t binaryCert[strlen(root_cert)];
    int len = b64decode(root_cert, binaryCert); // to test by comment out the BEGIN and END lines
    bool res = wifiClient.setCACert(binaryCert, len);
    //bool res = client.setCACert_P(root_cert, strlen(root_cert));
    if (!res) {
      Serial.println("Failed to load root CA certificate!");
      return false;
    }
*/
    return true;
}

bool readConfigFile() {
  Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nJson parsed");
          deviceCtrl.setMqttHost(json["mqttHost"]);
          deviceCtrl.setMqttPort(json["mqttPort"]);
          deviceCtrl.setProjectId(json["projectId"]);
          deviceCtrl.setLocation(json["location"]);
          deviceCtrl.setRegistryId(json["registryId"]);
          deviceCtrl.setDeviceId(json["deviceId"]);
          deviceCtrl.setPrivateKey(json["privateKey"]);
          return true;
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  return false;
}

int b64decode(String b64Text, uint8_t* output) {
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block(b64Text.c_str(), b64Text.length(), (char*)output, &s);
  return cnt;
}

void mqttCallback(char *topic, uint8_t *payload, unsigned int length) {
  Serial.print("New config received");

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject((char*)payload);
  if(root.success()) {
    root.printTo(Serial);
    Serial.println();

    processJsonMessage(root);
    g_stateChanged = true;
  }
}

void processJsonMessage(JsonObject& root) {
  if(root.containsKey("color")) {
    g_color = root["color"].as<uint32_t>();
  }
  if(root.containsKey("brightness")) {
    g_brightness = root["brightness"].as<uint8_t>();
  }
  if(root.containsKey("on")) {
      g_on = root["on"].as<bool>();
  }
}

void publishState() {
  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["time"] = time(nullptr);
  root["color"] = g_color;
  root["on"] = g_on;
  root["brightness"] = g_brightness;

  String message;
  root.printTo(Serial);
  Serial.println();
  root.printTo(message);

  mqttClient.publish(deviceCtrl.getStateTopic().c_str(), message.c_str());
}

void publishEvent() {
  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["time"] = time(nullptr);
  root["chipid"] = ESP.getChipId();
  root["boottime"] = g_bootTime;
  root["temperature"] = g_temperature;
  root["freeheap"] = ESP.getFreeHeap();

  String message;
  root.printTo(Serial);
  Serial.println();
  root.printTo(message);

  mqttClient.publish(deviceCtrl.getEventsTopic().c_str(), message.c_str());
}

bool mqttReconnect() {
  if (!mqttClient.connected()) {
    Serial.println("MQTT connecting ...");
    ESP.wdtDisable();
    String jwt = deviceCtrl.getJwt();
    ESP.wdtEnable(0);
    Serial.print("JWT: ");
    Serial.println(jwt.c_str());
    const char *user = "unused";
    String clientId = deviceCtrl.getClientId();
    Serial.print("Client id: ");
    Serial.println(clientId.c_str());

    if (mqttClient.connect(clientId.c_str(), user, jwt.c_str())) {
      Serial.println("Connected to MQTT server");
      String topic = deviceCtrl.getConfigTopic();
      Serial.print("Subscribe to ");
      Serial.println(topic.c_str());
      mqttClient.subscribe(topic.c_str(), 1);
      Serial.print("Subscribe to ");
      topic = deviceCtrl.getCommandsTopic();
      Serial.println(topic.c_str());
      mqttClient.subscribe(topic.c_str(), 1);
      return true;
    }
    return false;
  }
  return true;
}

void updateTemperature() {
  float average = 0;

  digitalWrite(TEMP_PWR_PIN, HIGH);
  for (uint8_t i = 0; i < TEMP_NUMSAMPLES; i++) {
   average += analogRead(TEMP_READ_PIN);
   delay(10);
  }
  average /= TEMP_NUMSAMPLES;
  digitalWrite(TEMP_PWR_PIN, LOW);

  // convert the value to resistance
  average = 1023 / average - 1;
  average = TEMP_SERIESRESISTOR / average;

  double steinhart;
  steinhart = average / THERMISTOR_NOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMP_NOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  g_temperature = steinhart;

  Serial.print("Temperature updated to ");
  Serial.print(g_temperature);
  Serial.println(" *C");
}
