#include <AsyncUDP.h>
#include "common.h"
#include "drv/eeprom.h"
#include "drv/network.h"
#include "esp32cam.h"
#include "wlanMsg.h"
#include <esp_wifi.h>
#include "wifiUser.h"
#include "serialMsg.h"

wlanMsgClass *pwlanMsgObj = NULL;
IPAddress remoteAddr;
bool bHeartbeatTimeout = true;
unsigned long heartBeatTimer = 0;
#define AP_NAME "Cymple_Face"
static void onPacketCallBack(AsyncUDPPacket packet){
    if(!packet.available() || packet.length() < sizeof(TLV_S)){
        return;
    }
    TLV_S *pstMsgHdr = (TLV_S *)packet.data();
    int msgLen = packet.length();
    switch(pstMsgHdr->uiType){
        case MSG_SERVER_HEARTBEAT_E:
        case MSG_SERVER_UNICAST_HEARTBEAT_E:
            if (bHeartbeatTimeout && pwlanMsgObj) {
                pwlanMsgObj->sendVersionFrame(packet.remoteIP(), CYMPLEFACE_SERVER_PORT);
            }
            heartBeatTimer = millis();
            bHeartbeatTimeout = false;
            remoteAddr = packet.remoteIP();
            break;
        default:
            serial_writelog("Invalid msg type%u, msg len:%d\n", pstMsgHdr->uiType, msgLen);
            break;
    }
}

wlanMsgClass::wlanMsgClass(){
    memset(acSSID, 0 , sizeof(acSSID));
    memset(acPassword, 0 , sizeof(acPassword));
    eepromApi::read(acSSID, OFFSET(EEPROM_DATA_S, acSSID), SSID_LENGTH);
    eepromApi::read(acPassword, OFFSET(EEPROM_DATA_S, acPassword), WIFI_PASSWORD_LENGTH);
    acSSID[SSID_LENGTH] = '\0';
    acPassword[WIFI_PASSWORD_LENGTH] = '\0';
    // 过滤未初始化的 EEPROM 区域 (0xFF)
    if((uint8_t)acSSID[0] == 0xFF){
        acSSID[0] = '\0';
    }
    if((uint8_t)acPassword[0] == 0xFF){
        acPassword[0] = '\0';
    }
    remoteAddr = INADDR_NONE;
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);

    int listenRetry = 0;
    while (!udpClient.listen(CYMPLEFACE_CAM_PORT)) //等待udp监听设置成功
    {
        delay(20);
        if(++listenRetry > 50){
            serial_writelog("UDP listen timeout\r\n");
            break;
        }
    }
    udpClient.onPacket(onPacketCallBack);
    if(acSSID[0] != 0){
        connect(acSSID, acPassword);
    }else{
        // 没有保存过凭据时直接进入配网模式，不进行无效的阻塞连接。
        tryConCount = 3;
    }
}

void wlanMsgClass::connect(const char *SSID, const char *password){
    if(NULL == SSID){
        serial_writelog("SSID missing\r\n");
        return;
    }
    if(NULL == password){
        password = "";
    }

    size_t ssidLength = strnlen(SSID, SSID_LENGTH + 1);
    size_t passwordLength = strnlen(password, WIFI_PASSWORD_LENGTH + 1);
    if(0 == ssidLength){
        serial_writelog("SSID missing\r\n");
        return;
    }
    if(ssidLength > SSID_LENGTH || passwordLength > WIFI_PASSWORD_LENGTH){
        serial_writelog("WiFi credentials are too long\r\n");
        return;
    }

    bool credentialsChanged = false;
    if((ssidLength != strlen(acSSID)) || (0 != memcmp(SSID, acSSID, ssidLength))){
        credentialsChanged = true;
        memset(acSSID, 0, sizeof(acSSID));
        memcpy(acSSID, SSID, ssidLength);
        eepromApi::write((void *)acSSID, OFFSET(EEPROM_DATA_S, acSSID), SSID_LENGTH);
    }
    if((passwordLength != strlen(acPassword)) || (0 != memcmp(password, acPassword, passwordLength))){
        credentialsChanged = true;
        memset(acPassword, 0, sizeof(acPassword));
        memcpy(acPassword, password, passwordLength);
        eepromApi::write((void *)acPassword, OFFSET(EEPROM_DATA_S, acPassword), WIFI_PASSWORD_LENGTH);
    }
    if(credentialsChanged || !WiFi.isConnected()){
        serial_writelog("Connecting to %s\n", acSSID);
        if(networkApi::connect(acSSID, acPassword)){
            tryConCount = 0;
        }
    }
}


void wlanMsgClass::connect(){
    if(0 == acSSID[0]){
        serial_writelog("SSID missing\r\n");
        tryConCount = 3;
        return;
    }
    serial_writelog("Connecting to %s\n", acSSID);
    if(networkApi::connect(acSSID, acPassword)){
        tryConCount = 0;
    }
}

void wlanMsgClass::APMode(){
    WiFi.mode(WIFI_AP);
    WiFi.persistent(false);
    WiFi.softAP(AP_NAME);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));   //设置AP热点IP和子网掩码
    wifiConfig();
}

void wlanMsgClass::send(uint8_t *data, size_t len, IPAddress ip, uint16_t port){
    if(len > CONFIG_TCP_MSS){
        serial_writelog("wlanMsgClass::send: data too large:%u\n", len);
        return;
    }
    udpClient.writeTo(data, len, ip, port);
}

void wlanMsgClass::send(uint8_t *data, size_t len){
    if(INADDR_NONE !=remoteAddr){
        send(data, len, remoteAddr, CYMPLEFACE_SERVER_PORT);
    }
    
}

void wlanMsgClass::sendVersionFrame(IPAddress ip, uint16_t port){
    MSG_WLAN_VERSION_S versionMsg;
    versionMsg.tlv.uiType = MSG_VERSION_E;
    versionMsg.tlv.uiLength = sizeof(MSG_WLAN_VERSION_S) - sizeof(TLV_S);
    versionMsg.uiMarkerStart = VERSION_MARKER_START;
    versionMsg.uiHardwareVer = DEFAULT_HARDWARE_VER;
    versionMsg.uiFirmwareVer = DEFAULT_FIRMWARE_VER;
    memset(versionMsg.aucReserved, 0, sizeof(versionMsg.aucReserved));
    versionMsg.uiMarkerEnd = VERSION_MARKER_END;
    send((uint8_t *)&versionMsg, sizeof(MSG_WLAN_VERSION_S), ip, port);
}

void wlanMsgClass::sendVersionFrame(){
    if(INADDR_NONE != remoteAddr){
        sendVersionFrame(remoteAddr, CYMPLEFACE_SERVER_PORT);
    }
}

int wlanMsgClass::runFrame(unsigned long currentT){
    if(tryConCount > 2){
        if(3 == tryConCount){
            serial_writelog("Wating to config WIFI\r\n");
            APMode();
            tryConCount++;
        }
        checkDNS_HTTP();                  //检测客户端DNS&HTTP请求，也就是检查配网页面那部分
        delay(30);
        return 1;
    }
    if(WiFi.isConnected()){
        tryConCount = 0;
    }
    if((!bHeartbeatTimeout) && ((currentT - heartBeatTimer) > WLAN_HEARTBEAT_TIMEOUT)){
        bHeartbeatTimeout = true;
    }
    if(bHeartbeatTimeout){
        if(!WiFi.isConnected()){
            serial_writelog("Wlan disconnected\r\n");
            tryConCount++;
            connect();
            delay(3000);
            return 1;
        }else{
            serial_writelog("Heartbeat timeout\r\n");
            delay(1000);
            return 1;
        }
    }
    return 0;
}
