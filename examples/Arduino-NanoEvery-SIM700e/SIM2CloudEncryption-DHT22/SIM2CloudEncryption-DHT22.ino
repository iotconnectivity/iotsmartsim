#include "dht.h"
#include "podenosim.h"

// Waveshare Sim7000E NB-IoT HAT
#define PIN_DTR  4

// AM2302 (DHT22)
#define PIN_DHT  5

PodEnoSim enosim(&Serial1);

dht sensor;

#define MODEM_BAUD_RATE  57600
#define LOG_BAUD_RATE  57600

#define LEN_DATA  22
#define OFS_DATA_DIGITS  16
const char DATA_ITEM[LEN_DATA] = { '{', '"', 't', 'e', 'm', 'p', 'e', 'r', 'a', 't', 'u', 'r', 'e', '"', ':', ' ', ' ', ' ', '.', ' ', ' ', '}' }; 

byte dataBuf[LEN_DATA];

char digits[4];

#define LEN_DEVICE_ID  10
byte id[LEN_DEVICE_ID];

void getTemperatureDigits(float t, char * digitBuffer) {
  int it = round(t * 100);
  char b;
  for (char i=0; i<4; i++) {
    b = it % 10;
    digitBuffer[3 - i] = (char)(0x30 + b);
    it -= b;
    it /= 10;
  }
}

void logHex(byte * buf, short len) {
  byte b;
  for (short i=0; i<len; i++) {
    b = buf[i];
    if (b < 0x10) {
      Serial.print("0");
    }
    Serial.print(b, HEX);
  }
  Serial.println();
}

void setup() {
  byte res;
  
  Serial.begin(LOG_BAUD_RATE); 
  while (!Serial) {
    ; // wait for serial port to connect
  }

  // Get Device ID
  memcpy(id, (void *)&SIGROW.SERNUM0, LEN_DEVICE_ID);
  Serial.print("Device ID: ");
  logHex(id, LEN_DEVICE_ID);

  // configure pins
  pinMode(PIN_DTR, OUTPUT);
  pinMode(PIN_DHT, INPUT);
  
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
}

void loop() {

  memcpy(dataBuf, DATA_ITEM, LEN_DATA);

  // read DHT sensor
  int chk = sensor.read22(PIN_DHT);
  if (chk != DHTLIB_OK) {
    Serial.println("DHT checksum error");
    delay(10000);
    return;
  }
  // convert temperature value into digits
  getTemperatureDigits(sensor.temperature, digits);

  // put temperature digits in message buffer
  // assume 10 <= temperature < 99
  dataBuf[OFS_DATA_DIGITS] = digits[0];
  dataBuf[OFS_DATA_DIGITS + 1] = digits[1];
  dataBuf[OFS_DATA_DIGITS + 3] = digits[2];
  dataBuf[OFS_DATA_DIGITS + 4] = digits[3];

  Serial.print("Put Data: ");
  byte res = enosim.dataSend(dataBuf, LEN_DATA);
  if (res == RES_OK) {
    Serial.println("OK");
  } else {
    Serial.print("ERROR ");
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
