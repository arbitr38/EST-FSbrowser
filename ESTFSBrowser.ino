/*
  FSWebServer - Example WebServer with FS backend for esp8266/esp32
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  upload the contents of the data folder with MkSPIFFS Tool ("ESP32 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp32fs.local/edit; done

  access the sample web page at http://esp32fs.local
  edit the page by going to http://esp32fs.local/edit
*/
#include <Arduino.h>
#include "esp_adc_cal.h"
#include <Button2.h>

#define ENABLE_SSD1306
#define ENABLE_BUTTON

#ifdef ENABLE_BUTTON
Button2 *pBtn = nullptr;
#endif

#define ADC_PINS 35

int loop_mode = 0;

enum {
    WIFI_SCAN = 32,
    VOLTAG_DETECT = 33,
    DEEPSLEEP = 34,
    KEY1 = 36,
    KEY2 = 39,
};


struct {
    uint8_t pin;
    const char *name;
} btns[] = {
    {32, "up"},
    {33, "down"},
    {34, "center"},
    {36, "left"},
    {39, "right"},
};

#define BTNSIZES    sizeof(btns)/sizeof(btns[0])

#ifdef ENABLE_SSD1306
#include "SSD1306.h"
#define SSD1306_ADDRESS 0x3c
#define I2C_SDA 21
#define I2C_SCL 22
SSD1306 oled(SSD1306_ADDRESS, I2C_SDA, I2C_SCL);
#endif

int vref = 1100;

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define FILESYSTEM SPIFFS
// You only need to format the filesystem once
#define FORMAT_FILESYSTEM false
#define DBG_OUTPUT_PORT Serial

#if FILESYSTEM == FFat
#include <FFat.h>
#endif
#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif

const char* ssid = "cisco.irk.ru";
const char* password = "Ve!c0dinC1n@";
const char* host = "esp32fs";
WebServer server(80);
//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool exists(String path){
  bool yes = false;
  File file = FILESYSTEM.open(path, "r");
  if(!file.isDirectory()){
    yes = true;
  }
  file.close();
  return yes;
}

bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(pathWithGz) || exists(path)) {
    if (exists(pathWithGz)) {
      path += ".gz";
    }
    File file = FILESYSTEM.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if (server.uri() != "/edit") {
    return;
  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = FILESYSTEM.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (!exists(path)) {
    return server.send(404, "text/plain", "FileNotFound");
  }
  FILESYSTEM.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (exists(path)) {
    return server.send(500, "text/plain", "FILE EXISTS");
  }
  File file = FILESYSTEM.open(path, "w");
  if (file) {
    file.close();
  } else {
    return server.send(500, "text/plain", "CREATE FAILED");
  }
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);


  File root = FILESYSTEM.open(path);
  path = String();

  String output = "[";
  if(root.isDirectory()){
      File file = root.openNextFile();
      while(file){
          if (output != "[") {
            output += ',';
          }
          output += "{\"type\":\"";
          output += (file.isDirectory()) ? "dir" : "file";
          output += "\",\"name\":\"";
          output += String(file.name()).substring(1);
          output += "\"}";
          file = root.openNextFile();
      }
  }
  output += "]";
  server.send(200, "text/json", output);
}

void battery_test()
{
    oled.clear();
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "battery test");
    oled.display();
    delay(1000);
}

void wifi_test()
{
    oled.clear();
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "wifi scan");
    oled.display();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int n = WiFi.scanNetworks();
    oled.clear();
    if (n == 0) {
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "no networks found");
        oled.display();
        return;
    }
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "Found" + String(n) + "net");
    oled.display();
}

void enter_sleep()
{
    oled.clear();
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "enter sleep 10 sec");
    oled.display();
    delay(1000);
    oled.clear();
    oled.display();
    system_deep_sleep(10 * 1000 * 1000);
}


void showVoltage()
{
    static uint64_t timeStamp = 0;
    if (millis() - timeStamp > 1000) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PINS);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
#ifdef ENABLE_SSD1306
        String voltage = "Voltage :" + String(battery_voltage) + "V";
        oled.clear();
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, voltage);
        oled.display();
#else
        Serial.println("DEBUG: Value: " + String(v) + " ,Battery Voltage is" + String(battery_voltage) + "V");
#endif
    }
}

#ifdef ENABLE_BUTTON
void btn_loop()
{
    for (int i = 0; i < BTNSIZES; i++) {
        pBtn[i].loop();
    }
}

void btn_callback(Button2 &b)
{
    for (int i = 0; i < BTNSIZES; i++) {
        if (pBtn[i] == b) {
            loop_mode = pBtn[i].getAttachPin();
            Serial.println(pBtn[i].getAttachPin());
            return;
        }
    }
}
#endif


void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  if (FORMAT_FILESYSTEM) FILESYSTEM.format();
  FILESYSTEM.begin();
  {
      File root = FILESYSTEM.open("/");
      File file = root.openNextFile();
      while(file){
          String fileName = file.name();
          size_t fileSize = file.size();
          DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
          file = root.openNextFile();
      }
      DBG_OUTPUT_PORT.printf("\n");
  }


  //WIFI INIT
  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
  }
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  MDNS.begin(host);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.println(".local/edit to see the file browser");


  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from FILESYSTEM
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(0));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

#ifdef ENABLE_BUTTON
    pBtn = new Button2 [BTNSIZES]();
    if (pBtn != nullptr) {
        for (int i = 0; i < BTNSIZES; i++) {
            pBtn[i] = Button2(btns[i].pin);
            pBtn[i].setPressedHandler(btn_callback);
        }
    }
#endif


#ifdef ENABLE_SSD1306
    oled.init();
    oled.flipScreenVertically();
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    delay(50);
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, WiFi.localIP().toString() + "/edit");
    oled.display();
#endif
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_7, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
    //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    } else {
        Serial.println("Default Vref: 1100mV");
    }
  

}


void loop(void) {
  server.handleClient();
   btn_loop();
    switch (loop_mode) {
    case WIFI_SCAN:
        wifi_test();
        loop_mode = -1;
        break;
    case VOLTAG_DETECT:
        showVoltage();
        break;
    case DEEPSLEEP:
        enter_sleep();
        break;
    case KEY1:
        oled.clear();
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "DOWN");
        oled.display();
        loop_mode = -1;
        break;
    case KEY2:
        oled.clear();
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "OK");
        oled.display();
        loop_mode = -1;
        break;
    default:
        break;
    }
}
