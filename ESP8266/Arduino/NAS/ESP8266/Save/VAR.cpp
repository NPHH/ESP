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

uint32_t                                                                                            WOLStateTimer = 0;

uint8_t                                                                                             PingRetryCount = 0;

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
    //  Ghép chuỗi tạo thành URL hoàn chỉnh kèm các tham số Query Parameter:
    //  ACTION: Hành động cần thực hiện (POLL hoặc ACK).
    //  KEY: Mã khoá bảo mật của thiết bị (DEVICE_KEY) để Server kiểm tra quyền truy cập.

    String URL = String(WEB_ADDRESS_URL) + "?ACTION=" + Action + "&KEY=" + DEVICE_KEY;

    #if 0

    Serial.println(URL);

    #endif
    
    //  Rất quan trọng đối với Google Apps Script: Server của Google luôn tự động chuyển hướng (Redirect 302/307) từ URL gốc sang URL thực thi tạm thời. 

    //  Dòng này giúp thư viện HTTPClient tự động đi theo liên kết chuyển hướng để lấy đúng dữ liệu.
    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.setTimeout(12000);

    //  Khởi tạo kết nối HTTPS an toàn (sử dụng Client TLS/SSL _TLSClient). Nếu URL sai định dạng hoặc không đủ bộ nhớ RAM, hàm báo lỗi kết nối và trả về false.
    if(!http.begin(_TLSClient, URL))
    {
        Serial.println("[POLL] Loi: khong khoi tao duoc ket noi HTTP");

        return false;
    }

    //  Thực hiện gửi yêu cầu GET lên Google Apps Script.
    int Code = http.GET();

    //  Nếu mã HTTP phản hồi là 200 (OK):
    
    if(Code == HTTP_CODE_OK)
    {
        //  Body = http.getString();: Lấy toàn bộ chuỗi JSON do Server trả về (ví dụ: {"STATUS":true,"CMD":"WOL"}) lưu vào biến tham chiếu Body.

        Body = http.getString();

        #if 0

        Serial.println(Body);

        #endif

        //  http.end();: Ngắt kết nối để giải phóng tài nguyên RAM.
        http.end();

        return true;
    }

    //  Nếu mã lỗi (ví dụ -1 lỗi WiFi, 404, 500...):
    //  In mã lỗi kèm mô tả ra Serial.printf.

    Serial.printf("[POLL] Loi HTTP: %d (%s)\n", Code, HTTPClient::errorToString(Code).c_str());

    //  Giải phóng kết nối http.end(); và trả về false.

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

        if(WOLState != WOL_STATE_IDLE) return;

        String BODY;

        if(!WIFIWebResponse("POLL", BODY))
        {
            Serial.println("[POLL] That Bai - Khong Nhan Duoc Phan Hoi Tu Web");

            return;
        }

        JsonDocument DOC;

        DeserializationError Error = deserializeJson(DOC, BODY);

        #if 1

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

        static const char* const STATE_NAME[] = { "IDLE", "WAIT_BOOT", "PINGING" };

        Serial.printf("[POLL] ESP: %s | Lenh Tu Web: %s\n", STATE_NAME[WOLState], Command);

        if(strcmp(Command, "WOL") == 0)
        {
            Serial.println("\n[LENH] Nhan Duoc Lenh WOL Tu Web!");

            if(SendWOL())
            {
                WOLState = WOL_STATE_WAIT_BOOT;

                SendACKWithRetry("BOOTING", WOL_WAIT_TIME_TO_PING / 1000);

                WOLStateTimer = millis();

                Serial.printf("[BOOT] Bat dau dem WaitTime: %u giay\n", (unsigned int)(WOL_WAIT_TIME_TO_PING / 1000));
            }
        }
    }
}
                                     
/******************************************************************************************************************************
    Magic Packet
******************************************************************************************************************************/
bool SendWOL(void)
{
    uint8_t Packet[102];

    memset(Packet, 0xFF, 6);

    for (int i = 0; i < 16; i++)
    {
        memcpy(&Packet[6 + i * 6], NAS_MAC, 6);
    }

    if(!_UDP.beginPacket(BROAD_CAST_IP, WOL_PORT))
    {
        Serial.println("[WOL] Khong tao duoc UDP packet");

        return false;
    }

    _UDP.write(Packet, sizeof(Packet));

    if(_UDP.endPacket() != 1)
    {
        Serial.println("[WOL] Gui Magic Packet that bai");

        return false;
    }

    Serial.println("[WOL] -> Da phat Magic Packet thanh cong!");

    return true;
}

/******************************************************************************************************************************
   
******************************************************************************************************************************/
bool SendACKWithRetry(const String& Status, uint32_t WaitTimeSec)
{
    for (uint8_t i = 0; i < ACK_RETRY_COUNT; i++)
    {
        if(SendACKStatus(Status, WaitTimeSec))
        {
            return true;
        }

        Serial.printf("[ACK] Gui that bai (lan %u/%u), thu lai...\n", i + 1, ACK_RETRY_COUNT);

        delay(ACK_RETRY_DELAY);
    }

    Serial.println("[ACK] Khong gui duoc len Web!");

    return false;
}

/******************************************************************************************************************************
   
******************************************************************************************************************************/
bool SendACKStatus(const String& Status, uint32_t WaitTimeSec)
{
    String URL = String(WEB_ADDRESS_URL) + "?ACTION=ACK&KEY=" + DEVICE_KEY + "&STATUS=" + Status;

    if(WaitTimeSec > 0)
    {
        URL += "&WAIT_TIME=" + String(WaitTimeSec);
    }

    #if 1

    Serial.println(URL);

    #endif

    Serial.printf("[ACK] Gui trang thai: %s (WaitTime: %u s)\n", Status.c_str(), (unsigned int)WaitTimeSec);

    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.setTimeout(12000);

    if(!http.begin(_TLSClient, URL))
    {
        return false;
    }

    int code = http.GET();

    http.end();

    return (code == HTTP_CODE_OK);
}

/******************************************************************************************************************************
   
******************************************************************************************************************************/
void WOLProcess(void)
{
    if(WOLState == WOL_STATE_WAIT_BOOT)
    {
        unsigned long Elapsed = millis() - WOLStateTimer;

        if(Elapsed < WOL_WAIT_TIME_TO_PING)
        {
            static unsigned long lastBootDebug = 0;

            if(millis() - lastBootDebug >= 1000)
            {
                lastBootDebug = millis();

                uint32_t remainingSec = (WOL_WAIT_TIME_TO_PING - Elapsed) / 1000;

                Serial.printf("[BOOT] Dang cho NAS khoi dong... Con lai: %u giay\n", (unsigned int)remainingSec);
            }
        }
        else
        {
            Serial.println("[BOOT] Het thoi gian cho boot. Chuyen sang Ping kiem tra...");

            WOLState = WOL_STATE_PINGING;

            WOLStateTimer = millis() - WOL_TIME_EVERY_PING;

            PingRetryCount = 0;
        }
    }
    
    else 
    
    if(WOLState == WOL_STATE_PINGING)
    {
        if(millis() - WOLStateTimer >= WOL_TIME_EVERY_PING)
        {
            WOLStateTimer = millis();

            Serial.printf("[PING] Dang Ping NAS (Lan %d/%d)...\n", PingRetryCount + 1, WOL_NUMBER_PING);

            if(Ping.ping(NAS_IP, 1))
            {
                Serial.println("[KET QUA] -> SUCCESS: NAS da Online (Ping OK)!");

                SendACKWithRetry("ONLINE");

                WOLState = WOL_STATE_IDLE;
            }
            else
            {
                PingRetryCount++;

                if(PingRetryCount >= WOL_NUMBER_PING)
                {
                    Serial.println("[KET QUA] -> FAILED: Het luot Ping, NAS khong phan hoi!");

                    SendACKWithRetry("FAILED");

                    WOLState = WOL_STATE_IDLE;
                }
            }
        }
    }
}


