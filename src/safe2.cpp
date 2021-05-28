#include <Arduino.h>
#include <HardwareSerial.h>
#include "safe2.h"

// number of loops to wait for the end of initialization 
#define TECHNOLOGY_INIT_WAITING  7

#define LEN_MODEM_AT  4
const unsigned char MODEM_AT[LEN_MODEM_AT] = {'a', 't', '\r', '\n'};

#define LEN_MODEM_AT_ECHO_OFF  6
const unsigned char MODEM_AT_ECHO_OFF[LEN_MODEM_AT_ECHO_OFF] = {'a', 't', 'e', '0', '\r', '\n'};


#define LEN_MODEM_AT_CREG  10
const unsigned char MODEM_AT_CREG[LEN_MODEM_AT_CREG] = {'a', 't', '+', 'c', 'r', 'e', 'g', '?', '\r', '\n'};

#define LEN_MODEM_CREG  8
const char MODEM_CREG[LEN_MODEM_CREG] = {'+', 'C', 'R', 'E', 'G', ':', ' ', '\0'};

#define LEN_MODEM_AT_CSIM  8
const unsigned char MODEM_AT_CSIM[LEN_MODEM_AT_CSIM] = {'a', 't', '+', 'c', 's', 'i', 'm', '='};

#define LEN_MODEM_CSIM  8
const char MODEM_CSIM[LEN_MODEM_CSIM] = {'+', 'C', 'S', 'I', 'M', ':', ' ', '\0'};


const char APDU_MANAGE_CHANNEL[LEN_APDU_HEADER] = { 0x00, 0x70, 0x00, 0x00, 0x00 };

const char APDU_SELECT[LEN_APDU_HEADER] = { 0x00, (char)0xA4, 0x04, 0x0C, 0x00 };

#define APDU_ISO_CLA  0x00

#define APDU_PUT_DATA_INS  0xda
#define APDU_PUT_DATA_P1  0x02

#define APDU_GET_DATA_INS  0xca
#define APDU_GET_DATA_MODE_REQUEST  0x00
#define APDU_GET_DATA_MODE_DATA  0x01

#define APDU_GET_STATUS_INS  0xcc
#define APDU_GET_STATUS_MODE_GET  0x02
#define APDU_GET_STATUS_LEN_GET  0x04

#define APDU_GET_RESPONSE_INS  0xc0

#define LEN_AID_SAFE2  12
const char AID_SAFE2[LEN_AID_SAFE2] = {(char)0xf0, 0x70, 0x6F, 0x64, 0x67, 0x73, 0x61, 0x66, 0x65, 0x32, 0x01, 0x01 };

#define LEN_MODEM_OK  4
const char MODEM_OK[LEN_MODEM_OK] = {'O', 'K', '\r', '\0'};

#define LEN_MODEM_ERROR  7
const char MODEM_ERROR[LEN_MODEM_ERROR] = {'E', 'R', 'R', 'O', 'R', '\r', '\0'};

#define LEN_1_CHAR_MAX  10
#define LEN_2_CHARS_MAX  100

#define CR   '\r'
#define EOL  '\n'


Safe2::Safe2(HardwareSerial * modemSerial) {
  _modem = modemSerial;
}

byte Safe2::init(long modemBaudRate) {

  if (modemBaudRate < 1200) {
    return RES_INVALID_PARAMETERS;
  }

  // initialize modem AT interface
  _modem->begin(modemBaudRate);
//  delay(1000);
  while (!(*_modem)) {
    ; // wait for serial port to connect
  }
  return RES_OK;
}


#define TIME_WAIT_FOR_CHECK_AT_RESPONSE_MS  100
#define TIME_WAIT_FOR_NEXT_CHAR_MS  10
#define TIME_WAIT_FOR_SERIAL_AVAILABLE_MS  300

void Safe2::waitForModemStart() {
  char b;
  while (true) {
    // put AT command
    _modem->write(MODEM_AT, LEN_MODEM_AT);
    delay(TIME_WAIT_FOR_CHECK_AT_RESPONSE_MS);
    // modem could not respond during initialization
    if (_modem->available()) {
      while (_modem->available()) {
        b = _modem->read();
        delay(TIME_WAIT_FOR_NEXT_CHAR_MS);
      }
      break;
    }
    delay(TIME_WAIT_FOR_SERIAL_AVAILABLE_MS);
  };
  _modem->write(MODEM_AT_ECHO_OFF, LEN_MODEM_AT_ECHO_OFF);
  delay(TIME_WAIT_FOR_CHECK_AT_RESPONSE_MS);
  while (_modem->available()) {
    b = _modem->read();
    delay(TIME_WAIT_FOR_NEXT_CHAR_MS);
  }
}

void Safe2::waitForNetworkRegistration() {
  byte cntr = 0;
  bool registeredFlag = false;
  while (cntr < TECHNOLOGY_INIT_WAITING) {
    // wait for a half of a second  
    delay(500);                       
    // put AT command
    memcpy(_bufferAT, MODEM_AT_CREG, LEN_MODEM_AT_CREG);
    atCommandSend(LEN_MODEM_AT_CREG);
    // read AT response
    short respLen = atCommandResponse();
    
    // analyze response
    char * ptr;
    ptr = strstr((char *)_bufferAT, MODEM_CREG);
    if (ptr != NULL) {
      registeredFlag = registered(&ptr[LEN_MODEM_CREG + 1]);
    } else {
    }
    if (registeredFlag) {
      cntr++;
    }
    // wait 1 sec before repeat
    delay(1000);
  }
}

bool Safe2::registered(char * resp) {
  char b = *resp;
  b &= 0x0f;
  resp++;
  if (*resp != CR) {
    // 2-chars <stat> value
    b *= 10;
    b += *resp - 0x30;
  }
  switch (b) {
    case 1: // 1: registered, home network
    case 5: // 5: registered, roaming
      return true;
    
    default:
      return false;
  }
}

void Safe2::atCommandSend(short len) {
  short cmdLen = len;
  short blockLen;
  short ofs = 0;
  while (cmdLen > 0) {
    blockLen = _modem->availableForWrite();
    if (blockLen < 1) {
      delay(1);
      continue;
    }
    if (blockLen > cmdLen) {
      blockLen = cmdLen;
    }
    _modem->write(&_bufferAT[ofs], blockLen);
    ofs += blockLen;
    cmdLen -= blockLen;
  }
  _modem->flush();
}

#define CNTR_MAX  10000

short Safe2::atCommandResponse() {
  // read AT response
  short cntr = 0;
  char * ptr;
  short len = 0;
  short blockLen;

  // initialize for timeout
  _bufferAT[0] = 0;
  do {
    blockLen = _modem->readBytesUntil(EOL, &_bufferAT[len], (int)(LEN_IO_BUFFER - len));
    if (blockLen > 0) {
      len += blockLen;
      _bufferAT[len++] = EOL;
      _bufferAT[len] = 0;
    }

    ptr = strstr((char *)_bufferAT, MODEM_OK);
    if (ptr == NULL) {
      ptr = strstr((char *)_bufferAT, MODEM_ERROR);
    }
    
  } while (ptr == NULL);
  return len;
}

short Safe2::decimalPut(int v, short offset) {
  short ofs = offset;
  // number of chars populated
  char cnt = 1;
  char b;
  // safety check - shall be redesigned for negative numbers
  if (v < 0) {
    v = 0;
  }
  if (v >= LEN_1_CHAR_MAX) {
    ofs++;
    cnt++;
  }
  if (v >= LEN_2_CHARS_MAX) {
    ofs++;
    cnt++;
  }
  // at least 1 digit shall be populated
  do {
    b = (char)(v % 10);
    _bufferAT[ofs--] = '0' + b;
    v -= b;
    v /= 10;
  } while (v > 0);
  return offset + cnt;
}

#define LEN_HEX  16
const char HEX_CONVERSION[LEN_HEX] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

short Safe2::bytePutHex(byte b, short offset) {
  _bufferAT[offset++] = HEX_CONVERSION[(b >> 4) & 0x0f];
  _bufferAT[offset++] = HEX_CONVERSION[b & 0x0f];
  return offset;
}

#define LEN_BYTE_HEX  2

short Safe2::atCsimBuild(byte * apduBuf, byte * dataBuf, short dataLen) {
  short len = LEN_APDU_HEADER; 
  short ofs = 0;
  if (dataBuf != NULL) {
    len += dataLen;
  }
  memcpy(_bufferAT, MODEM_AT_CSIM, LEN_MODEM_AT_CSIM);
  ofs = decimalPut(len << 1, LEN_MODEM_AT_CSIM);
  _bufferAT[ofs++] = ',';
  _bufferAT[ofs++] = '"';
  
  // put CLA
  ofs = bytePutHex(apduBuf[0] | _channelNo, ofs);
  // put INS, P1, P2
  for (char i=1; i<4; i++) {
    ofs = bytePutHex(apduBuf[i], ofs);
  }
  // put LEN
  ofs = bytePutHex(dataLen, ofs);
  // put Data if any
  if ((dataLen > 0) && (dataBuf != NULL)) {
    for (short i=0; i<dataLen; i++) {
      ofs = bytePutHex(dataBuf[i], ofs);
    }
  }
  
  _bufferAT[ofs++] = '"';
  _bufferAT[ofs++] = '\r';
  _bufferAT[ofs++] = '\n';
  _bufferAT[ofs] = 0;
  return ofs;
}


#define OFS_APDU_CLA  0x00
#define OFS_APDU_INS  0x01
#define OFS_APDU_P1  0x02
#define OFS_APDU_P2  0x03
#define OFS_APDU_LEN  0x04

#define MODE_CHANNEL_CLOSE  0x80

#define TAG_DEVICE_ID  0xc0
#define TAG_DEVICE_DATA 0xc1

byte Safe2::channelOpen() {
  if (_channelNo != 0) {
    return RES_OK;
  }
  // put AT SIM command: Open Supplementary Logical Channel
  short cmdLen = atCsimBuild((byte *)APDU_MANAGE_CHANNEL, NULL, 1);
  atCommandSend(cmdLen);
  short respLen = atCommandResponse();
  // analyze response
  char * ptr;
  ptr = strstr((char *)_bufferAT, MODEM_CSIM);
  if (ptr == NULL) {
    return RES_INVALID_CSIM_RESPONSE;
  }

  if ((ptr[LEN_MODEM_CSIM - 1] != '6') ||
      (ptr[LEN_MODEM_CSIM + 0] != ',') || 
      (ptr[LEN_MODEM_CSIM + 1] != '"') ||
      (ptr[LEN_MODEM_CSIM + 2] != '0')) {

    return RES_INVALID_OPEN_CHANNEL_RESPONSE;
  }

  // extract channel ID
  char ch = ptr[LEN_MODEM_CSIM + 3] - '0';
  if ((ch <= 0) || (ch >= 4)) {
    return RES_INVALID_CHANNEL_ID;
  }
  _channelNo = ch;

  // SELECT (by AID) SAFE2
  cmdLen = atCsimBuild((byte *)APDU_SELECT, (byte *)AID_SAFE2, LEN_AID_SAFE2);
  atCommandSend(cmdLen);
  respLen = atCommandResponse();

  ptr = strstr((char *)_bufferAT, MODEM_CSIM);
  if (ptr == NULL) {
    return RES_INVALID_CSIM_RESPONSE;
  }

  if ((ptr[LEN_MODEM_CSIM - 1] != '4') ||
      (ptr[LEN_MODEM_CSIM + 0] != ',') || 
      (ptr[LEN_MODEM_CSIM + 1] != '"') ||
      (ptr[LEN_MODEM_CSIM + 2] != '9') ||
      (ptr[LEN_MODEM_CSIM + 3] != '0') ||
      (ptr[LEN_MODEM_CSIM + 4] != '0') ||
      (ptr[LEN_MODEM_CSIM + 5] != '0')) {

    return RES_INVALID_SELECT_RESPONSE;
  }
  return RES_OK;
}

byte Safe2::channelClose() {
  if (_channelNo == 0) {
    return RES_OK;
  }
  // close supplementary logical channel
  memcpy(_apduHeader, APDU_MANAGE_CHANNEL, LEN_APDU_HEADER);
  _apduHeader[OFS_APDU_P1] = MODE_CHANNEL_CLOSE;
  _apduHeader[OFS_APDU_P2] = _channelNo;
  short cmdLen = atCsimBuild(_apduHeader, NULL, 0);
  atCommandSend(cmdLen);
  short respLen = atCommandResponse();
  _channelNo = 0;
  return RES_OK;
}

void Safe2::prepareForSleep() {
  // Deselect
  channelClose();
}

byte Safe2::dataGet(byte mode, short &dataLength) {
  char bh, bl;
  short cmdLen;
  short respLen;
  byte res;
  if (_channelNo == 0) {
    res = channelOpen();
    if (res != RES_OK) {
      return res;
    }
  }
  
  _apduHeader[OFS_APDU_CLA] = APDU_ISO_CLA;
  _apduHeader[OFS_APDU_INS] = APDU_GET_DATA_INS;
  _apduHeader[OFS_APDU_P1] = mode;
  _apduHeader[OFS_APDU_P2] = 0x00;
  _apduHeader[OFS_APDU_LEN] = 0x00;
  cmdLen = atCsimBuild(_apduHeader, NULL, 0);
  atCommandSend(cmdLen);
  respLen = atCommandResponse();

  if (mode == APDU_GET_DATA_MODE_DATA) {
    // analyze response
    char * ptr;
    ptr = strstr((char *)_bufferAT, MODEM_CSIM);
    if (ptr == NULL) {
      return RES_INVALID_CSIM_RESPONSE;
    }
  
    // expected response: 4,"61xx"
    if ((ptr[LEN_MODEM_CSIM - 1] == '4') &&
        (ptr[LEN_MODEM_CSIM + 0] == ',') && 
        (ptr[LEN_MODEM_CSIM + 1] == '"') &&
        (ptr[LEN_MODEM_CSIM + 2] == '6') && 
        (ptr[LEN_MODEM_CSIM + 3] == '1')) {
  
      bh = hex2val(ptr[LEN_MODEM_CSIM + 4]);
      bl = hex2val(ptr[LEN_MODEM_CSIM + 5]);
      if ((bh == -1) || (bl == -1)) {
        return RES_INVALID_GET_DATA_RESPONSE;
      }
      byte len = (unsigned char)((bh << 4) | bl);
    
      _apduHeader[OFS_APDU_CLA] = APDU_ISO_CLA;
      _apduHeader[OFS_APDU_INS] = APDU_GET_RESPONSE_INS;
      _apduHeader[OFS_APDU_P1] = 0x00;
      _apduHeader[OFS_APDU_P2] = 0x00;
      _apduHeader[OFS_APDU_LEN] = len;
      cmdLen = atCsimBuild(_apduHeader, NULL, len);
      atCommandSend(cmdLen);
      respLen = atCommandResponse();
    
      ptr = strstr((char *)_bufferAT, MODEM_CSIM);
      if (ptr == NULL) {
        return RES_INVALID_CSIM_RESPONSE;
      }
    }    
    short hlen = 0;
    short ofs = LEN_MODEM_CSIM - 1;
    char b;
    do {
      b = ptr[ofs++] - 0x30;
      if ((b < 0) || (b > 9)) {
        return RES_INVALID_CSIM_RESPONSE;
      }
      hlen *= 10; // shift left for 1 tetrade
      hlen += b;
    } while (ptr[ofs] != ',');
    // skip ',' and '"'
    ofs += 2;
    hlen >>= 1; // number of bytes
    if (hlen <= 0) {
      return RES_INVALID_GET_DATA_RESPONSE;
    }
    hlen -= 2; // exclude SW
    for (short ii=0; ii<hlen; ii++) {
      bh = hex2val(ptr[ofs]);
      bl = hex2val(ptr[ofs + 1]);
      if ((bh == -1) || (bl == -1)) {
        return RES_INVALID_CSIM_RESPONSE;
      }
      ofs += 2;
      _bufferAT[ii] = (byte)((bh << 4) | bl);
    }
    _bufferAT[hlen] = 0;
    // re-assign response length
    respLen = hlen;
  }
  dataLength = respLen;
  return RES_OK;
}

byte Safe2::configRequest() {

  short len = 0;
  return dataGet(APDU_GET_DATA_MODE_REQUEST, len);
}

byte Safe2::configGet(byte * configBuffer, short &configLength) {
  short len = 0;
  byte res = dataGet(APDU_GET_DATA_MODE_DATA, len);
  if (res != RES_OK) {
    return res;
  }
  if (len <= 0) {
    return RES_INVALID_GET_DATA_RESPONSE;
  }
  memcpy(configBuffer, _bufferAT, len);
  return RES_OK;
}

char Safe2::hex2val(char h) {
  for (char i=0; i<LEN_HEX; i++) {
    if (HEX_CONVERSION[i] == h) {
      return i;
    }
  }
  return -1;
}

short Safe2::chars2short(char * buf) {
  char b; 
  short w = 0;
  for (char i=0; i<4; i++) {
    b = hex2val(buf[i]);
    if (b < 0) {
      return -1;
    }
    w <<= 4;
    w += b;
  }
  return w;
}

byte Safe2::state(short &receivingState, short &receivingResult) {
  _apduHeader[OFS_APDU_CLA] = APDU_ISO_CLA;
  _apduHeader[OFS_APDU_INS] = APDU_GET_STATUS_INS;
  _apduHeader[OFS_APDU_P1] = APDU_GET_STATUS_MODE_GET;
  _apduHeader[OFS_APDU_P2] = 0x00;
  _apduHeader[OFS_APDU_LEN] = 0x00;
  short cmdLen = atCsimBuild(_apduHeader, NULL, 0);
  atCommandSend(cmdLen);
  short respLen = atCommandResponse();

  // analyze response
  char * ptr;
  ptr = strstr((char *)_bufferAT, MODEM_CSIM);
  if (ptr == NULL) {
    return RES_INVALID_CSIM_RESPONSE;
  }

  // expected response: 4,"6104"
  if ((ptr[LEN_MODEM_CSIM - 1] == '4') &&
      (ptr[LEN_MODEM_CSIM + 0] == ',') && 
      (ptr[LEN_MODEM_CSIM + 1] == '"') &&
      (ptr[LEN_MODEM_CSIM + 2] == '6') && 
      (ptr[LEN_MODEM_CSIM + 3] == '1') &&
      (ptr[LEN_MODEM_CSIM + 4] == '0') && 
      (ptr[LEN_MODEM_CSIM + 5] == '4')) {

    _apduHeader[OFS_APDU_CLA] = APDU_ISO_CLA;
    _apduHeader[OFS_APDU_INS] = APDU_GET_RESPONSE_INS;
    _apduHeader[OFS_APDU_P1] = 0x00;
    _apduHeader[OFS_APDU_P2] = 0x00;
    _apduHeader[OFS_APDU_LEN] = APDU_GET_STATUS_LEN_GET;
    cmdLen = atCsimBuild(_apduHeader, NULL, APDU_GET_STATUS_LEN_GET);
    atCommandSend(cmdLen);
    respLen = atCommandResponse();
  
    ptr = strstr((char *)_bufferAT, MODEM_CSIM);
    if (ptr == NULL) {
      return RES_INVALID_CSIM_RESPONSE;
    }
  }
  
  // expected response: [receivingState:2][resultCode:2][SW:2] => 12 chars
  if ((ptr[LEN_MODEM_CSIM - 1] != '1') ||
      (ptr[LEN_MODEM_CSIM + 0] != '2') || 
      (ptr[LEN_MODEM_CSIM + 1] != ',') ||
      (ptr[LEN_MODEM_CSIM + 2] != '"')) {
    return RES_UNEXPECTED_GET_STATUS_RESPONSE;
  }

  // extract state
  short wState = chars2short(&ptr[LEN_MODEM_CSIM + 3]);
  if (wState < 0) {
    return RES_INVALID_GET_STATUS_RESPONSE;
  }
  short wResult = chars2short(&ptr[LEN_MODEM_CSIM + 3 + 4]);
  if (wResult < 0) {
    return RES_INVALID_GET_STATUS_RESPONSE;
  }
  receivingState = wState;
  receivingResult = wResult;
  return RES_OK;
}


byte Safe2::dataPut(byte tag, byte * dataPtr, short dataLen) {
  short cmdLen;
  short respLen;
  byte res;
  if (_channelNo == 0) {
    res = channelOpen();
    if (res != RES_OK) {
      return res;
    }
  }
  
  // PUT DATA
  _apduHeader[OFS_APDU_CLA] = APDU_ISO_CLA;
  _apduHeader[OFS_APDU_INS] = APDU_PUT_DATA_INS;
  _apduHeader[OFS_APDU_P1] = APDU_PUT_DATA_P1;
  _apduHeader[OFS_APDU_P2] = tag;
  _apduHeader[OFS_APDU_LEN] = 0x00;
  cmdLen = atCsimBuild(_apduHeader, dataPtr, dataLen);
  atCommandSend(cmdLen);
  respLen = atCommandResponse();
  return RES_OK;
}

byte Safe2::dataSend(byte * dataBuffer, short dataLength) {
  return dataPut(TAG_DEVICE_DATA, dataBuffer, dataLength);
}

byte Safe2::deviceIdSet(byte * deviceIdBuffer, short deviceIdLength) {
  return dataPut(TAG_DEVICE_ID, deviceIdBuffer, deviceIdLength);
}

