/*
  Arduino Nano Every + Waveshare Sim7000E NB-IoT Hat + DHT22 SIM2Cloud-Encryption Demo.
  Copyright (c) 2021 Pod Group Ltd. http://podgroup.com
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the MIT License.
  
  You should have received a copy of the MIT License
  along with this program.  If not, see <https://opensource.org/licenses/MIT>.

  Description:
    - Post DHT22 Temperature to Pod IoT Platform API (See below).
    - Uses Pod ENO SIM built-in TLS1.3-PSK to request HTTPS POST /v1/data/{DEVICEID}

  More information:
    - Pod IoT Platform: https://iotsim.podgroup.com/v1/docs/#/
    - Arduino Project Hub Article: https://create.arduino.cc/projecthub/kostiantynchertov/zero-touch-provisioning-based-on-tls-1-3-a07359

  Usage:
    - Open Board Manager and install "Arduino megaAVR Boards".
    - Open Library Manager and install: "DHT Sensor Library".
    - Select Board "Arduino Nano Every".

  Requirements:
    - Arduino Nano Every.
    - Waveshare Sim7000E NB-IoT Hat
    - AM2302 DHT22 Temperature Sensor.
    - Pod ENO SIM Card (ask for yours, emails below).

  Authors:
   - Kostiantyn Chertov <kostiantyn.chertov@podgroup.com>
   - J. Félix Ontañón <felix.ontanon@podgroup.com>
*/

// --------- BEGINING OF CONFIGURABLE FIRMWARE PARAMETERS SECTION ---------

// Waveshare Sim7000E NB-IoT HAT
#define PIN_DTR  4

// LED (Config)
#define PIN_LED  6

// --------- END OF CONFIGURABLE FIRMWARE PARAMETERS SECTION ---------

#include <ArduinoJson.h>
#include "podenosim.h"

PodEnoSim enosim(&Serial1);

#define STATE_READY  0x00
#define STATE_REQUEST_SENT  0x01
#define STATE_REQUEST_DATA  0x02
#define STATE_DONE  0x03

#define RES_JSON_DESERIALIZATION_FAILED  0x81

#define MODEM_BAUD_RATE  57600
#define LOG_BAUD_RATE  57600

#define LEN_DEVICE_ID  10
byte id[LEN_DEVICE_ID];

char gState;

#define JSON_CONFIG_CAPACITY  192
StaticJsonDocument<JSON_CONFIG_CAPACITY> cfg;

#define LEN_CONFIG  255
byte gConfig[LEN_CONFIG];

#define LEN_NAME_CONTROL_LED  9
const char NAME_CONTROL_LED[LEN_NAME_CONTROL_LED] = { 'i', 'o', 't', ':', 'A', 'l', 'a', 'r', 'm' };

#define LEN_NAME_EFFECT_ON  2
const char NAME_EFFECT_ON[LEN_NAME_EFFECT_ON] = { 'O', 'n' };

char configParseApply(const char * buf, short dataLen) {
  
  DeserializationError error = deserializeJson(cfg, buf, dataLen);
  
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return RES_JSON_DESERIALIZATION_FAILED;
  }
  
  const char* cfg_version = cfg["configuration"]["version"]; // "2020-10-13"
  Serial.print("configuration version: ");
  Serial.println(cfg_version);
  
  const char* cfg_config_0_action = cfg["configuration"]["config"][0]["action"]; // "iot:Alarm"
  const char* cfg_config_0_effect = cfg["configuration"]["config"][0]["effect"]; // "On"

  if (memcmp(cfg_config_0_action, NAME_CONTROL_LED, LEN_NAME_CONTROL_LED) == 0) {
    if (memcmp(cfg_config_0_effect, NAME_EFFECT_ON, LEN_NAME_EFFECT_ON) == 0) {
      Serial.println("iot:Alarm configured as ON");
      digitalWrite(PIN_LED, HIGH);
    } else {
      Serial.println("iot:Alarm configured as OFF");
      digitalWrite(PIN_LED, LOW);
    }
  }
  
}

void setup() {
  byte res;
  
  Serial.begin(LOG_BAUD_RATE); 
  while (!Serial) {
    ; // wait for serial port to connect
  }

  // get Device ID
  memcpy(id, (void *)&SIGROW.SERNUM0, LEN_DEVICE_ID);

  // configure pins (DTR - mandatory)
  pinMode(PIN_DTR, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  
  Serial.print("Modem initialization...");
  res = enosim.init(MODEM_BAUD_RATE);
  if (res == RES_OK) {
    Serial.println("OK");
  } else {
    Serial.print("ERROR: ");
    Serial.println(res);
  }

  // wake up Sim7000
  digitalWrite(PIN_DTR, LOW);   
  delay(1000);

  Serial.print("waiting for modem start...");
  enosim.waitForModemStart();
  Serial.println("OK");

  Serial.print("waiting for network registration...");
  enosim.waitForNetworkRegistration();
  Serial.println("OK");
  
  // put Device ID
  res = enosim.deviceIdSet(id, LEN_DEVICE_ID);
  if (res == RES_OK) {
    Serial.println("Set Device ID: OK");
  } else {
    Serial.println("Set Device ID: ERROR");
  }
  enosim.prepareForSleep();
  
  gState = STATE_READY;
}

void loop() {

  byte secs;
  short dataLen = 0;
  short receiveState, receiveRes;
  char res;


  if (gState == STATE_READY) {
    Serial.println("config request");
    res = enosim.configRequest();
    if (res == RES_OK) {
      Serial.println("request - OK");
      gState = STATE_REQUEST_SENT;
    } else {
      gState = STATE_DONE;
    }
  }
  if (gState == STATE_REQUEST_SENT) {
    
    for (secs=0; secs < 10; secs++) {
      delay(1000);
    }
    res = enosim.state(receiveState, receiveRes);
    Serial.print("receiving state: ");
    Serial.println(receiveState, HEX);
    if (receiveState == STATUS_RECEIVE_DATA) {
      gState = STATE_REQUEST_DATA;
    }
    if (receiveState == STATUS_RECEIVE_DONE) {
      if (receiveRes != 0) {
        Serial.print("receiving error: ");
        Serial.println(receiveRes, HEX);
        gState = STATE_DONE;  
        return;
      }
    }
    if (res != RES_OK) {
      gState = STATE_DONE;
      return;
    }
  }
  if (gState == STATE_REQUEST_DATA) {
    dataLen = LEN_CONFIG;
    res = enosim.configGet(gConfig, dataLen);
    if (res == RES_OK) {
      res = configParseApply((char *)gConfig, dataLen);
    }
    gState = STATE_DONE;
  }
  if (gState == STATE_DONE) {
    enosim.prepareForSleep();
    gState = STATE_READY;
  
    Serial.print("waiting");
    byte minutes = 5;
    
    while (minutes > 0) {
      for (secs=0; secs < 60; secs++) {
        delay(1000);
      }
      minutes -= 1;
      Serial.print('.');
    }
  }
  Serial.println();
}
