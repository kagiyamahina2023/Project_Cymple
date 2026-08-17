#ifndef _NETWORK_HPP
#define _NETWORK_HPP
#include <Arduino.h>
#include <WiFi.h>
#include "../serialMsg.h"
namespace networkApi
{
    static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000;

    static bool connect(const char *pSSID, const char *pPassword)
    {
        if((NULL == pSSID) || ('\0' == pSSID[0])){
            serial_writelog("SSID missing\r\n");
            return false;
        }
        WiFi.mode(WIFI_STA);
        if(WL_CONNECT_FAILED == WiFi.begin(pSSID, pPassword)){
            serial_writelog("Fail to start WiFi connection to %s\n", pSSID);
            return false;
        }
        if(WL_CONNECTED != WiFi.waitForConnectResult(WIFI_CONNECT_TIMEOUT_MS))
        {
            serial_writelog("Fail to connect to %s\n", pSSID);
        }
        else{
            serial_writelog("Success to connect to WIFI\r\n");
        }
        return WiFi.isConnected();
    }
};


#endif
