#include <MKRGSM.h>
#include <ArduinoUniqueID.h>

#include "dht.h"
#include "safe2.h"


// AM2302 (DHT22)
#define PIN_DHT  4
#define PIN_LED  7

Safe2 safe2(&SerialGSM, &Serial);

dht sensor;

#define MODEM_BAUD_RATE  9600
#define LOG_BAUD_RATE  115200

#define LEN_DATA  22
#define OFS_DATA_DIGITS  16
const char DATA_ITEM[LEN_DATA] = { '{', '"', 't', 'e', 'm', 'p', 'e', 'r', 'a', 't', 'u', 'r', 'e', '"', ':', ' ', ' ', ' ', '.', ' ', ' ', '}' }; 

unsigned char dataBuf[LEN_DATA];

char digits[4];

#define LEN_DEVICE_ID_MAX  16
byte id[LEN_DEVICE_ID_MAX];

void setup() {
  byte res;
  
  // configure pins
  pinMode(PIN_DHT, INPUT);
  pinMode(PIN_LED, OUTPUT);

  safe2.init(MODEM_BAUD_RATE, LOG_BAUD_RATE);

  safe2.waitForModemStart();
  safe2.waitForNetworkRegistration();
  Serial.println("setup finished");

  
  // put Device ID
  short idLen = UniqueIDsize;
  if (idLen > LEN_DEVICE_ID_MAX) {
    idLen = LEN_DEVICE_ID_MAX;
  }
  memcpy(id, (void *)UniqueID, idLen);
  res = safe2.deviceIdSet(id, idLen);
  if (res == RES_OK) {
    Serial.println("Set Device ID: OK");
  } else {
    Serial.println("Set Device ID: ERROR");
  }
  safe2.prepareForSleep();
}

void loop() {

  memcpy(dataBuf, DATA_ITEM, LEN_DATA);

  int chk = sensor.read22(PIN_DHT);
  switch (chk) {
    case DHTLIB_OK:
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("DHT checksum error");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("DHT time out error");
      break;
    default:
      Serial.println("DHT unknown error");
      break;
  }
  if (chk != DHTLIB_OK) {
    delay(10000);
    return;
  }
  float t = sensor.temperature;
  int it = round(t * 100);
  char b;
  for (char i=0; i<4; i++) {
    b = it % 10;
    digits[3 - i] = (char)(0x30 + b);
    it -= b;
    it /= 10;
  }

  // put Temperature in buffer
  // assume 10 <= temperature < 99
  dataBuf[OFS_DATA_DIGITS] = digits[0];
  dataBuf[OFS_DATA_DIGITS + 1] = digits[1];
  // skip offset 2 for '.'
  dataBuf[OFS_DATA_DIGITS + 3] = digits[2];
  dataBuf[OFS_DATA_DIGITS + 4] = digits[3];

  byte res = safe2.dataSend(dataBuf, LEN_DATA);
  if (res == RES_OK) {
    Serial.println("Put Data: OK");
  } else {
    Serial.print("Put Data: ERROR");
    Serial.println(res);
  }

  Serial.print("waiting");
  byte minutes = 5;
  byte secs;
  while (minutes > 0) {
    for (secs=0; secs < 60; secs++) {
      delay(1000);
    }
    minutes -= 1;
    Serial.print('.');
  }
  Serial.println();
}
