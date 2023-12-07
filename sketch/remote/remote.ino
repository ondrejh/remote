/*
  Web interface remote controller.

  The program is based on Arduino AdvancedWebServer example.
  I uses Arduinojson library (by Benoit Blanchon) to prepare json data and to parse
  json POST input data.

  Author: Ondrej Hejda
  Date: 12.2023

  Other sources used:

  AP mode:
    https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/

  PWM:
    https://lastminuteengineers.com/esp32-pwm-tutorial/

  ESP dev board pinout and pin restrictions:
    https://lastminuteengineers.com/esp32-pinout-reference/

  ADC:
    https://lastminuteengineers.com/esp32-basics-adc/
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.
*/

#ifdef ESP32
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
#else
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
#endif

#include <ArduinoJson.h>

#define AP_MODE

#ifdef AP_MODE
/* Put your SSID & Password */
char ssid[16] = "rem";
const char* password = "remote123";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192,168,48,1);
IPAddress gateway(192,168,48,1);
IPAddress subnet(255,255,255,0);
#else
#include "secrets.h"
#endif

#include "webi.h"

#ifdef ESP32
  WebServer server(80);
#else
  ESP8266WebServer server(80);
#endif

const int PWM_CHANNEL_1 = 0;    // ESP32 has 16 channels which can generate 16 independent waveforms
const int PWM_CHANNEL_2 = 1;
const int PWM_CHANNEL_3 = 2;
const int PWM_CHANNEL_4 = 3;
const int PWM_FREQ = 500;     // Recall that Arduino Uno is ~490 Hz. Official ESP32 example uses 5,000Hz
const int PWM_RESOLUTION = 8;
const int PWM_OUT1 = 19;
const int PWM_OUT2 = 18;
const int PWM_OUT3 = 17;
const int PWM_OUT4 = 16;

const int SERVO_FREQ = 50;
const int SERVO_RES = 12;
const int SERVO_CH1 = 4;
const int SERVO_CH2 = 5;
const int SERVO_OUT1 = 13;
const int SERVO_OUT2 = 14;

const int OC_CH1 = 26;
const int OC_CH2 = 27;

const int BEEP = 32;

const int PWM_LIMIT = 50; // %
const bool SERVO_SWAP = true;
const int SERVO_LIMIT = 75; // %

StaticJsonDocument<512> doc;

const int led = 2;
const int bat = 36;

void handleRoot() {
    digitalWrite(led, 1);
    server.setContentLength(INDEX_HTML_LEN);
    server.send(200, "text/html", "");
    server.sendContent_P(index_html_bin, INDEX_HTML_LEN);
    digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void handleStatic(int i) {
  if (i < NFILES) {
    digitalWrite(led, 1);
    Serial.println(fnames[i]);
    server.setContentLength(flengths[i]);
    server.send(200, contents[contIds[i]], "");
    server.sendContent_P(fdata[i], flengths[i]);
    digitalWrite(led, 0);
  }
  else {
    handleNotFound();
  }
}

void handleData() {
  digitalWrite(led, 1);
  if (server.method() == HTTP_POST) {
    StaticJsonDocument<256> rec;
    DeserializationError error = deserializeJson(rec, server.arg("plain"));
    if (error) {
      Serial.println("Deserialization error.");
    }
    else {
      doc["control"]["up"] = rec["btnUp"];
      doc["control"]["down"] = rec["btnDown"];
      doc["control"]["left"] = rec["btnLeft"];
      doc["control"]["right"] = rec["btnRight"];
      doc["control"]["light"] = rec["btnLight"];
      doc["control"]["beep"] = rec["btnBeep"];
    }
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
  digitalWrite(led, 0);
}


bool pollBatteryVoltage(uint32_t now) {
  static uint32_t tbat = 0;
  static int buff[16];
  static int p = 0;
  if ((now - tbat) >= 100) {
    buff[p++] = analogRead(bat);
    p &= 0x0F;
    int adc = 0;
    for (int i=0; i<16; i++)
      adc += buff[i];
    doc["actual"]["bat"] = (float)adc * (6.94 / (1380 * 16));
    tbat = now; //+= 100;
    return true;
  }
  return false;
}

float addVal(bool add, bool sub, float val, float dt, float speed, float decay) {
  float v = val;
  if (add) {
    v += dt * speed;
    if (v > 1.0)
      v = 1.0;
  }
  else if (sub) {
    v -= dt * speed;
    if (v < -1.0)
      v = -1.0;
  }
  else if (v > 0.0) {
    v -= dt * decay;
    if (v < 0.0) {
      v = 0.0;
    }
  }
  else if (v < 0.0) {
    v += dt * decay;
    if (v > 0.0) {
      v = 0.0;
    }
  }
  return v;
}

void setPWM(float val, const int pwmP, const int pwmN) {
  int dc = 255.0 * abs(val) * ((float)PWM_LIMIT / 100.0);
  if (dc > 255)
    dc = 255;
  if (val > 0) {
    ledcWrite(pwmN, 0);
    ledcWrite(pwmP, dc);
  }
  else {
    ledcWrite(pwmP, 0);
    ledcWrite(pwmN, dc);
  }
}

void setServo(float val, const int pwm) {
  int dc = 204.8 * (1.5 + val / 2.0 * (SERVO_SWAP ? -1.0 : 1.0) * ((float)SERVO_LIMIT / 100.0));
  if (dc > 410) dc = 410;
  if (dc < 205) dc = 205;
  ledcWrite(pwm, dc);
}

bool pollControl(uint32_t now) {
  static uint32_t tspd = 0;
  if ((now - tspd) >= 5) {
    float acc = addVal(doc["control"]["up"], doc["control"]["down"], doc["actual"]["acc"], 0.005, 4.0, 4.0);
    float dir = addVal(doc["control"]["right"], doc["control"]["left"], doc["actual"]["dir"], 0.005, 4.0, 1.0);
    setPWM(acc, PWM_CHANNEL_1, PWM_CHANNEL_2);
    setPWM(acc, PWM_CHANNEL_3, PWM_CHANNEL_4);
    setServo(dir, SERVO_CH1);
    setServo(dir, SERVO_CH2);
    doc["actual"]["dir"] = dir;
    doc["actual"]["acc"] = acc;
    bool light = doc["control"]["light"];
    digitalWrite(OC_CH1, light);
    digitalWrite(OC_CH2, light);
    digitalWrite(BEEP, doc["control"]["beep"]);
    tspd = now;
    return true;
  }
  return false;
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  Serial.println("");

#ifdef AP_MODE
  // Create SSID out of MAC
  uint8_t mac[6];
  WiFi.macAddress(mac);
  sprintf(ssid, "%s%02X%02X%02X", ssid, mac[3], mac[4], mac[5]);
  // Create AP
  Serial.print("Create AP: ");
  Serial.println(ssid);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
#else
  // Connect to Wi-Fi network with SSID and password
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
#endif

  // battery voltage adc input
  pinMode(bat, INPUT); // battery analog input

  // open ocllector outputs
  pinMode(OC_CH1, OUTPUT);
  pinMode(OC_CH2, OUTPUT);
  digitalWrite(OC_CH1, 0);
  digitalWrite(OC_CH2, 0);

  // beeper output
  pinMode(BEEP, OUTPUT);
  digitalWrite(BEEP, 0);

  // motor pwm
  ledcSetup(PWM_CHANNEL_1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_2, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_3, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_4, PWM_FREQ, PWM_RESOLUTION);

  ledcAttachPin(PWM_OUT1, PWM_CHANNEL_1);
  ledcAttachPin(PWM_OUT2, PWM_CHANNEL_2);
  ledcAttachPin(PWM_OUT3, PWM_CHANNEL_3);
  ledcAttachPin(PWM_OUT4, PWM_CHANNEL_4);

  ledcWrite(PWM_CHANNEL_1, 0);
  ledcWrite(PWM_CHANNEL_2, 0);
  ledcWrite(PWM_CHANNEL_3, 0);
  ledcWrite(PWM_CHANNEL_4, 0);

  // servo pwm
  ledcSetup(SERVO_CH1, SERVO_FREQ, SERVO_RES);
  ledcSetup(SERVO_CH2, SERVO_FREQ, SERVO_RES);
  
  ledcAttachPin(SERVO_OUT1, SERVO_CH1);
  ledcAttachPin(SERVO_OUT2, SERVO_CH2);

  setServo(0.0, SERVO_CH1); //ledcWrite(SERVO_CH1, 614);
  setServo(0.0, SERVO_CH2); //ledcWrite(SERVO_CH1, 614);

  JsonObject jsroot = doc.to<JsonObject>();
  jsroot["cnt"] = 0.0;
  JsonObject jscontrol = jsroot.createNestedObject("control");
  jscontrol["right"] = false;
  jscontrol["left"] = false;
  jscontrol["up"] = false;
  jscontrol["down"] = false;
  jscontrol["light"] = false;
  jscontrol["beep"] = false;
  JsonObject jsactual = jsroot.createNestedObject("actual");
  jsactual["acc"] = 0.0;
  jsactual["dir"] = 0.0;
  jsactual["bat"] = 0.0;

#ifdef AP_MODE
  Serial.print("IP address: ");
  Serial.println(local_ip);
#else
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);*/
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  if (MDNS.begin(ssid)) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/data.json", handleData);
  server.on("/data.json", HTTP_POST, handleData);

  server.on(fnames[0], [](){handleStatic(0);});
  server.on(fnames[1], [](){handleStatic(1);});
  server.on(fnames[2], [](){handleStatic(2);});
  server.on(fnames[3], [](){handleStatic(3);});

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  
  uint32_t now = millis();
  static uint32_t tcnt = 0;
  if (pollControl(now)) {}
  else if (pollBatteryVoltage(now)) {}
  else if ((now - tcnt) >= 1000) {
    uint32_t cnt = doc["cnt"];
    cnt ++;
    doc["cnt"] = cnt;
    tcnt = now; //+= 1000;
  }
  else delay(1);//allow the cpu to switch to other tasks (which? duno!)
}