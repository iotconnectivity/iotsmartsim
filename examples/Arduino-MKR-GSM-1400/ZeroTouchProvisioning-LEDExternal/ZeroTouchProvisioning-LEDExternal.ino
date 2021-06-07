/*
  Arduino MKR GSM 1400 + External LED. Zero Touch Provisioning Demo.
  Copyright (c) 2021 Pod Group Ltd. http://podgroup.com
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the MIT License.
  
  You should have received a copy of the MIT License
  along with this program.  If not, see <https://opensource.org/licenses/MIT>.

  Description:
    - Powers ON/OFF The LED_BUILTIN for Arduino MKR 1400 GSM board.
    - Uses Pod ENO SIM to retrieve LED configuration from Pod IoT Platform.

  More information:
    - Pod IoT Platform: https://iotsim.podgroup.com/v1/docs/#/
    - Arduino Project Hub Article: https://create.arduino.cc/projecthub/kostiantynchertov/zero-touch-provisioning-based-on-tls-1-3-a07359

  Usage:
    - Open Board Manager and install "Arduino SAMD Boards (32-bits ARM Cortex-M0+)".
    - Open Library Manager and install: "MKRGSM", "ArduinoUniqueID", "ArduinoLowPower", "ArduinoJson"
    - Select Board "Arduino MKR 1400 GSM".
    - Optionally configure seconds between retries.

  Requirements:
    - Arduino MKR 1400 GSM board.
    - Led 10mm + Resistor 220 Ohm.
    - Pod ENO SIM Card (ask for yours, emails below).
    - Configuration for the SIM Card with the following format (at Pod IoT Platform):
      {
        "configuration": {
        "version": "2021-05-31",
        "config": [{
          "action": "iot:Alarm",
          "effect": "On"
        ]}
      }    

  Authors:
   - Kostiantyn Chertov <kostiantyn.chertov@podgroup.com>
   - J. Félix Ontañón <felix.ontanon@podgroup.com>
*/

// --------- BEGINING OF CONFIGURABLE FIRMWARE PARAMETERS SECTION ---------

// PIN the LED is wired to
#define PIN_LED 7

// Seconds to sleep between each run. You can configure this.
// const int SleepSecs = 5 * 60;
const int SleepSecs = 1 * 60;

// --------- END OF CONFIGURABLE FIRMWARE PARAMETERS SECTION ---------

#include <MKRGSM.h>
#include <ArduinoUniqueID.h>
#include <ArduinoJson.h>
#include <ArduinoLowPower.h>
#include "PodEnoSim.h"

PodEnoSim enosim(&SerialGSM);

#define STATE_READY  0x00
#define STATE_REQUEST_SENT  0x01
#define STATE_REQUEST_DATA  0x02
#define STATE_DONE  0x03

#define RES_JSON_DESERIALIZATION_FAILED  0x81

#define MODEM_BAUD_RATE  9600
#define LOG_BAUD_RATE  115200

#define LEN_DEVICE_ID_MAX  16
byte id[LEN_DEVICE_ID_MAX];

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
  short idLen = UniqueIDsize;
  if (idLen > LEN_DEVICE_ID_MAX) {
    idLen = LEN_DEVICE_ID_MAX;
  }
  memcpy(id, (void *)UniqueID, idLen);

  // configure pins (DTR - mandatory)
  pinMode(GSM_RESETN, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  
  Serial.print("Modem initialization...");
  res = enosim.init(MODEM_BAUD_RATE);
  if (res == RES_OK) {
    Serial.println("OK");
  } else {
    Serial.print("ERROR: ");
    Serial.println(res);
  }

  // reset the ublox module
  digitalWrite(GSM_RESETN, HIGH);
  delay(100);
  digitalWrite(GSM_RESETN, LOW);
  delay(300);

  Serial.print("waiting for modem start...");
  enosim.waitForModemStart();
  Serial.println("OK");

  Serial.print("waiting for network registration...");
  enosim.waitForNetworkRegistration();
  Serial.println("OK");
  
  // put Device ID
  res = enosim.deviceIdSet(id, idLen);
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
    Serial.println();
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
    LowPower.deepSleep(SleepSecs * 1000);
  }
}
