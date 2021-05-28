/*
  Safe2.h - Library to access SIM based SAFE2 services.
  Created by Kostiantyn Chertov, PodGroup, May 14, 2021.
  Released into the public domain.
*/
#ifndef SAFE2_H
#define SAFE2_H

#include "Arduino.h"
#include "HardwareSerial.h"

#define LEN_APDU_HEADER  5

// IO buffer used for AT commands proceeding, size = header + 256 * 2
#define LEN_IO_BUFFER  600

#define LEN_DEVICE_ID_MAX  16

#define STATUS_RECEIVE_DONE  0x0003
#define STATUS_RECEIVE_DATA  0x0007
#define STATUS_RECEIVE_ERROR  0x000f


#define RES_OK  0x00
#define RES_INVALID_PARAMETERS  0x01
#define RES_INVALID_CSIM_RESPONSE  0x02
#define RES_INVALID_OPEN_CHANNEL_RESPONSE  0x03
#define RES_INVALID_CHANNEL_ID  0x04
#define RES_INVALID_SELECT_RESPONSE  0x05
#define RES_UNEXPECTED_GET_STATUS_RESPONSE  0x06
#define RES_INVALID_GET_STATUS_RESPONSE  0x07
#define RES_UNEXPECTED_GET_DATA_RESPONSE  0x08
#define RES_INVALID_GET_DATA_RESPONSE  0x09


class Safe2 {
  public:
    Safe2(HardwareSerial * modemSerial);
    
    // standard API
    byte init(long modemBaudRate);
    
    void waitForModemStart();
    void waitForNetworkRegistration();

    byte state(short &receivingState, short &receivingResult);
    
    byte deviceIdSet(byte * deviceIdBuffer, short deviceIdLength);
    byte dataSend(byte * dataBuffer, short dataLength);
    
    byte configRequest();
    byte configGet(byte * configBuffer, short &configLength);

    // extended API
    short atCsimBuild(byte * apduBuf, byte * dataBuf, short dataLen);
    void atCommandSend(short len);
    short atCommandResponse();
    
    void prepareForSleep();
    
  private:
    HardwareSerial * _modem;

    byte _bufferAT[LEN_IO_BUFFER];
    byte _apduHeader[LEN_APDU_HEADER];

    byte _channelNo;

    bool registered(char * resp);
    short decimalPut(int v, short offset);
    short bytePutHex(byte b, short offset);
    char hex2val(char h);
    short chars2short(char * buf);
    
    byte channelOpen();
    byte channelClose();
    byte dataGet(byte mode, short &dataLen);
    byte dataPut(byte tag, byte * dataBuffer, short dataLen);
};

#endif
