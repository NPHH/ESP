#include "WOL.h"


/******************************************************************************************************************************

******************************************************************************************************************************/
const uint8_t NAS_MAC[6] =
{
    MAC_BYTE_0, MAC_BYTE_1, MAC_BYTE_2, MAC_BYTE_3, MAC_BYTE_4, MAC_BYTE_5,
};

/******************************************************************************************************************************

******************************************************************************************************************************/
const IPAddress NAS_IP(IP_ADDRESS_0, IP_ADDRESS_1, IP_ADDRESS_2, IP_ADDRESS_3);  

/******************************************************************************************************************************

******************************************************************************************************************************/
const IPAddress BROAD_CAST_IP(BROAD_CAST_IP_ADDRESS_0, BROAD_CAST_IP_ADDRESS_1, BROAD_CAST_IP_ADDRESS_2, BROAD_CAST_IP_ADDRESS_3);











/******************************************************************************************************************************

******************************************************************************************************************************/
int                                                                                                 LEDState = LOW;

uint32_t                                                                                            PreviousMillis = 0;

const uint32_t                                                                                      Interval = 300;

uint32_t                                                                                            LastPoll = 0;

/******************************************************************************************************************************

******************************************************************************************************************************/
WOL_STATE_t                                                                                         WOLState = WOL_STATE_IDLE;

/******************************************************************************************************************************

******************************************************************************************************************************/
WiFiUDP                                                                                             _UDP;

WiFiClientSecure                                                                                    _TLSClient;















/******************************************************************************************************************************

******************************************************************************************************************************/
void WIFICheckConnect(void)
{
    if(WiFi.status() != WL_CONNECTED)
    {
        WiFi.reconnect();

        delay(1000);

        return;
    }
}

/******************************************************************************************************************************

******************************************************************************************************************************/
void WIFIConnect(void)
{
    WiFi.mode(WIFI_STA);

    WiFi.begin(SSID, PASS);

    Serial.print("Connect To WiFi ");

    Serial.print(SSID);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);

        Serial.print(".");
    }

    Serial.println();

    Serial.print("ESP8266 IP Address: ");

    Serial.println(WiFi.localIP());
}



/******************************************************************************************************************************

******************************************************************************************************************************/
bool WIFIWebResponse(const String& Action, String& Body)
{
    String URL = String(WEB_ADDRESS_URL) + "?ACTION=" + Action + "&KEY=" + DEVICE_KEY;

    //Serial.println(URL);

    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.setTimeout(12000);

    if(!http.begin(_TLSClient, URL))
    {
        Serial.println("[POLL] Loi: khong khoi tao duoc ket noi HTTP");

        return false;
    }

    int Code = http.GET();

    if(Code == HTTP_CODE_OK)
    {
        Body = http.getString();

        http.end();

        return true;
    }

    Serial.printf("[POLL] Loi HTTP: %d (%s)\n", Code, HTTPClient::errorToString(Code).c_str());

    http.end();

    return false;
}



/******************************************************************************************************************************

******************************************************************************************************************************/
void POLLCommand(void)
{
    if((WOLState == WOL_STATE_IDLE) && (millis() - LastPoll >= POLL_INTERVAL))
    {
        LastPoll = millis();

        static const char* const STATE_NAME[] = { "IDLE", "WAIT_BOOT", "PINGING" };

        if(WOLState != WOL_STATE_IDLE) return;

        String BODY;

        if(!WIFIWebResponse("POLL", BODY))
        {
            Serial.println("[POLL] That Bai - Khong Nhan Duoc Phan Hoi Tu Web");

            return;
        }

        JsonDocument DOC;

        DeserializationError Error = deserializeJson(DOC, BODY);

        #if 0

        Serial.print("[POLL] DOC: ");

        serializeJson(DOC, Serial);

        Serial.println();

        #endif

        if(Error)
        {
            Serial.printf("[POLL] Loi Phan Tich JSON: %s | Du lieu: %s\n", Error.c_str(), BODY.c_str());

            return;
        }

        if(!DOC["STATUS"].as<bool>())
        {
            const char* error = DOC["ERROR"] | "Unkown";

            Serial.printf("[POLL] Web Tu Choi: %s\n", error);

            return;
        }

        const char* Command = DOC["CMD"] | "NONE";

        Serial.printf("[POLL] ESP: %s | Lenh Tu Web: %s\n", STATE_NAME[WOLState], Command);

        if(strcmp(Command, "WOL") == 0)
        {
            Serial.println("\n[LENH] Nhan Duoc Lenh WOL Tu Web!");

            // if(sendWOL())
            // {
            //     WOLState = WOL_STATE_WAIT_BOOT;

            //     // Gui WaitTime (giay) len web de web dem nguoc
            //     sendAckWithRetry("BOOTING", WOL_WAIT_TIME_TO_PING / 1000);

            //     // Bat dau dem WaitTime SAU KHI ack xong de khop voi dong ho tren web
            //     wolStateTimer = millis();

            //     Serial.printf("[BOOT] Bat dau dem WaitTime: %u giay\n", (unsigned int)(WOL_WAIT_TIME_TO_PING / 1000));
            // }
        }
    }
}



