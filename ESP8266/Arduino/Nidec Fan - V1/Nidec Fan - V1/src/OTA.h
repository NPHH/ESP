#ifndef _OTA_H_
#define _OTA_H_

#include <stdio.h>

#include <ESP8266WiFi.h>

#include <ESP8266mDNS.h>

#include <WiFiUdp.h>

#include <ArduinoOTA.h>

/******************************************************************************************************************************
						
******************************************************************************************************************************/
#define OTA_TIME_WAIT                                   5000   

/******************************************************************************************************************************
						
******************************************************************************************************************************/
typedef void(*OTACallback)(void);

/******************************************************************************************************************************
						
******************************************************************************************************************************/
void OTA(const char* ssid, const char* pass);

/******************************************************************************************************************************
						
******************************************************************************************************************************/
uint8_t OTAHandle(OTACallback CallBack = NULL);

/******************************************************************************************************************************
						
******************************************************************************************************************************/








#endif
