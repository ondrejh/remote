/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include "secrets.h"
#include "webi.h"

#ifdef ESP32
  WebServer server(80);
#else
  ESP8266WebServer server(80);
#endif

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
    }
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
  digitalWrite(led, 0);
}

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(bat, INPUT); // battery analog input
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  JsonObject jsroot = doc.to<JsonObject>();
  jsroot["cnt"] = 0.0;
  JsonObject jscontrol = jsroot.createNestedObject("control");
  jscontrol["right"] = false;
  jscontrol["left"] = false;
  jscontrol["up"] = false;
  jscontrol["down"] = false;
  jscontrol["light"] = false;
  JsonObject jsactual = jsroot.createNestedObject("actual");
  jsactual["acc"] = 0.0;
  jsactual["dir"] = 0.0;
  jsactual["bat"] = 0.0;

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
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

void loop(void) {
  server.handleClient();
  
  uint32_t now = millis();
  static uint32_t tcnt = 0;
  if (pollBatteryVoltage(now)) {}
  else if ((now - tcnt) >= 1000) {
    uint32_t cnt = doc["cnt"];
    cnt ++;
    doc["cnt"] = cnt;
    tcnt = now; //+= 1000;
  }
  else delay(1);//allow the cpu to switch to other tasks
}