#ifndef SERIALMSG_H
#define SERIALMSG_H
#include "common.h"
#define SERIAL_RX_BUFF_SIZE 1024
// 消息类型枚举
typedef enum {
    SERIAL_MSG_LOG_E = 0,
    SERIAL_MSG_REQ_DEVICEINFO_E = 1,
    SERIAL_MSG_WIFI_CONFIG_E = 3,
    SERIAL_MSG_POSITION_CFG_E = 4,
    SERIAL_MSG_MAX_E
} SERIAL_MSG_TYPE_E;

// WiFi配置消息
typedef struct {
    STREAM_TLV_S tlv;
    char acSSID[SSID_LENGTH];
    char acPassword[WIFI_PASSWORD_LENGTH];
} SERIAL_MSG_WIFICONFIG_S;

typedef struct {
    STREAM_TLV_S tlv;
    uint8_t ucPosition;
    int8_t reserved[3];
}SERIAL_MSG_POSITION_CFG_S;

class serialClass{
public:
    serialClass();
    void runFrame(unsigned long currentT);
private:
    void serialMsgCallback(uint16_t type, uint16_t len);
    bool bRcvSerialHdr = false;
    uint16_t usSerialRxDataLen = 0;
    uint16_t usSerialRxDataOffset = 0;
    char acSerialRxBuffer[SERIAL_RX_BUFF_SIZE];
    unsigned long timer = 0;
    bool getSerialMsgHead();
};

#define SERIAL_MIN_SIZE sizeof(STREAM_TLV_S)
extern void serial_writelog(const char *format, ...);
extern serialClass *pserialObj;
#endif /* SERIALMSG_H */
