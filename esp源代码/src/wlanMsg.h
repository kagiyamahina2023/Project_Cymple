#ifndef WLANMSG_H
#define WLANMSG_H
#include <AsyncUDP.h>
#include "common.h"
enum{
    MSG_SERVER_HEARTBEAT_E = 0,
    MSG_SERVER_UNICAST_HEARTBEAT_E = 4,
    MSG_IMAGE_V2_E = 5,
    MSG_VERSION_E = 6,
    MSG_MAX_E = 9,
};

#define VERSION_MARKER_START 0xAA55AA55
#define VERSION_MARKER_END   0x55AA55AA
#define DEFAULT_HARDWARE_VER 1
#define DEFAULT_FIRMWARE_VER 906

typedef struct {
    // 总长度固定 28 字节 (payload 24 字节)
    TLV_S tlv;               // uiType = MSG_VERSION_E (6), uiLength = 24
    uint32_t uiMarkerStart;  // 0xAA55AA55
    uint32_t uiHardwareVer;  // hardware version (e.g. 1)
    uint32_t uiFirmwareVer;  // firmware version (e.g. 906)
    uint8_t aucReserved[8];  // 8 bytes reserved (0)
    uint32_t uiMarkerEnd;    // 0x55AA55AA
} MSG_WLAN_VERSION_S;

typedef struct {
    // tlv头部，uiType为MSG_IMAGE_V2_E，uiLength总长度(包含该头部)
    TLV_S tlv;
    // 偏移
    uint16_t uiOffset;
    // jpg图片总长度
    uint16_t uiTotalLen;
    // 当前包总长度（包含该头部）
    uint16_t uiDataLen;
    // 0 为左眼，1为右眼，2为单目
    uint8_t ucDeviceFlag;
    // 帧自增索引，每发送一帧图像加1
    uint8_t ucFrameIndex;
    // JPG图像数据
    uint8_t aucData[0];
}MSG_WLAN_IMAGE_V2_S;

// 如果是在电脑端编程，可以将图像resize到合适的大小或者压缩到整个包小于一个udp报文大小（65535），
// 无需手动分片（uiOffset == 0, uiTotalLen == uiDataLen）就可以直接发送
// ESP由于强制使用TCP MTU为1460，所以需要手动分片

class wlanMsgClass{
private:
    char acSSID[SSID_LENGTH + 1];
    char acPassword[WIFI_PASSWORD_LENGTH + 1];
    AsyncUDP udpClient;
    
public:
    uint8_t tryConCount = 0;
    wlanMsgClass();
    void connect(const char *SSID, const char *password);
    void connect();
    void send(uint8_t *data, size_t len, IPAddress ip, uint16_t port);
    void send(uint8_t *data, size_t len);
    void sendVersionFrame(IPAddress ip, uint16_t port);
    void sendVersionFrame();
    int runFrame(unsigned long currentT);
    void APMode();
};
static IPAddress apIP(192, 168, 4, 1);            //设置AP的IP地址
extern wlanMsgClass *pwlanMsgObj;
#endif
