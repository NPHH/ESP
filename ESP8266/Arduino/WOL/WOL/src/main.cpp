#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <ESP8266Ping.h>





/******************************************************************************************************************************
    Khu Vực Cần Điều Chỉnh Thông Số
******************************************************************************************************************************/	
/******************************************************************************************************************************
		
******************************************************************************************************************************/	
const char* WIFI_SSID = "MissuBee";                                 // Tên Đăng Nhập Wifi

const char* WIFI_PASS = "12344321";                                 // Pass Wifi

/******************************************************************************************************************************
    Trang WEB Được Tao Ra Từ APP Sript Khi Triển Khai Dự Án
******************************************************************************************************************************/	
const char* WEBAPP_URL = "https://script.google.com/macros/s/AKfycbxEUiLffpZxOXKhnkSyvNv5Wzn0jWUtmCDqp3iZ4Jmusva9ebmWczSL-UN0wHuO_qLJ/exec";

/******************************************************************************************************************************
    Tự Đặt - Gia Tri Phải Giống Với File Code.gs - WEB_PIN Trong Code.gs Là Mật Khẩu Để Bật NAS Trong WEB
******************************************************************************************************************************/	
const char* DEVICE_KEY = "ESP8266WOL920PLUS_A7F93C21";

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
const uint8_t NAS_MAC[6] = 
{
    0x10, 0x90, 0x28, 0xEA, 0x2F, 0xDC
};

const IPAddress NAS_IP(192, 168, 1, 200);

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
#define WOL_WAIT_TIME_TO_PING                         120000         // Thời gian NAS Khởi Động Cho Đến Khi Ping Được

#define WOL_TIME_EVERY_PING                           3000          // Thời gian Giữa Các Lần Ping

#define WOL_NUMBER_PING                               10            // Tổng Số Lần Ping

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
enum WOL_STATE_t : uint8_t
{
    WOL_STATE_IDLE 																		= 0x00,

    WOL_STATE_WAIT_BOOT 															= 0x01,

    WOL_STATE_PINGING 																= 0x02,
};

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
int ledState = LOW;

unsigned long previousMillis = 0;

const long interval = 300;

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
const IPAddress BROADCAST_IP(192, 168, 1, 255);

const uint16_t WOL_PORT = 9;

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
WiFiUDP udp;

unsigned long lastPoll = 0;

const unsigned long POLL_INTERVAL = 5000;

WiFiClientSecure tlsClient;

WOL_STATE_t wolState = WOL_STATE_IDLE;

unsigned long wolStateTimer = 0;

uint8_t pingRetryCount = 0;

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
bool sendWOL()
{
    uint8_t packet[102];

    memset(packet, 0xFF, 6);

    for (int i = 0; i < 16; i++) 
    {
        memcpy(&packet[6 + i * 6], NAS_MAC, 6);
    }

    if (!udp.beginPacket(BROADCAST_IP, WOL_PORT)) 
    {
        Serial.println("Khong tao duoc UDP packet");

        return false;
    }

    udp.write(packet, sizeof(packet));

    if (udp.endPacket() != 1) 
    {
        Serial.println("Gui WOL that bai");

        return false;
    }

    Serial.println("Da phat Magic Packet");

    return true;
}

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
bool getWebResponse(const String& action, String& body)
{
    String url = String(WEBAPP_URL) + "?action=" + action + "&key=" + DEVICE_KEY;

    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.setTimeout(12000);

    if (!http.begin(tlsClient, url)) 
    {
        Serial.println("HTTP begin that bai");

        return false;
    }

    int code = http.GET();

    Serial.printf("HTTP status: %d\n", code);

    if (code > 0) 
    {
        Serial.println(http.getString());
    }

    if (code == HTTP_CODE_OK) 
    {
        body = http.getString();

        http.end();

        return true;
    }

    Serial.printf("HTTP loi: %d\n", code);

    http.end();

    return false;
}

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
bool sendAckStatus(const String& status)
{
    uint32_t totalWaitSec = (WOL_WAIT_TIME_TO_PING + (WOL_NUMBER_PING * WOL_TIME_EVERY_PING)) / 1000;

    String url = String(WEBAPP_URL) + "?action=ack&key=" + DEVICE_KEY + "&status=" + status + "&waitTime=" + String(totalWaitSec);

    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.setTimeout(12000);

    if (!http.begin(tlsClient, url)) 
    {
        Serial.println("HTTP ack begin that bai");

        return false;
    }

    int code = http.GET();

    if (code == HTTP_CODE_OK) 
    {
        http.end();

        return true;
    }

    http.end();

    return false;
}

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
void pollCommand()
{
    if (wolState != WOL_STATE_IDLE) return;

    String body;

    if (!getWebResponse("poll", body)) return;

    Serial.println(body);

    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, body);

    if (err) 
    {
        Serial.println("JSON khong hop le");

        return;
    }

    if (!doc["ok"].as<bool>()) 
    {
        Serial.println("Web App tu choi yeu cau");

        return;
    }

    const char* command = doc["command"] | "NONE";

    if (strcmp(command, "WOL") == 0) 
    {
        if (sendWOL()) 
        {
            sendAckStatus("BOOTING");

            wolState = WOL_STATE_WAIT_BOOT;

            wolStateTimer = millis();
        }
    }
}

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
void processWolProcess()
{
    if (wolState == WOL_STATE_WAIT_BOOT) 
    {
        if (millis() - wolStateTimer >= WOL_WAIT_TIME_TO_PING) 
        {
            wolState = WOL_STATE_PINGING;

            wolStateTimer = millis();

            pingRetryCount = 0;
        }
    }
    else if (wolState == WOL_STATE_PINGING) 
    {
        if (millis() - wolStateTimer >= WOL_TIME_EVERY_PING) 
        {
            wolStateTimer = millis();

            if (Ping.ping(NAS_IP, 1)) 
            {
                Serial.println("NAS da Online (Ping OK)");

                sendAckStatus("ONLINE");

                wolState = WOL_STATE_IDLE;
            } 
            else 
            {
                pingRetryCount++;

                if (pingRetryCount >= WOL_NUMBER_PING) 
                {
                    Serial.println("NAS khong phan hoi Ping");

                    sendAckStatus("FAILED");

                    wolState = WOL_STATE_IDLE;
                }
            }
        }
    }
}

/******************************************************************************************************************************
		
******************************************************************************************************************************/	
void connectWiFi()
{
    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Dang ket noi WiFi");

    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);

        Serial.print(".");
    }

    Serial.println();

    Serial.print("IP ESP8266: ");

    Serial.println(WiFi.localIP());
}






/******************************************************************************************************************************
		
******************************************************************************************************************************/	
void setup() 
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(9600);

    Serial.println("Begin...");

    connectWiFi();

    udp.begin(2390);

    tlsClient.setInsecure();

    Serial.println("ESP8266 WOL Ready");





}

/******************************************************************************************************************************
		
******************************************************************************************************************************/	


/******************************************************************************************************************************
		
******************************************************************************************************************************/	
void loop() 
{
    if (WiFi.status() != WL_CONNECTED) 
    {
        WiFi.reconnect();

        delay(1000);

        return;
    }

    if (millis() - lastPoll >= POLL_INTERVAL) 
    {
        lastPoll = millis();

        pollCommand();
    }

    processWolProcess();

    delay(10);
  



    unsigned long currentMillis = millis();
    
    if (currentMillis - previousMillis >= interval) 
    {
        previousMillis = currentMillis;

        if (ledState == LOW) 
        {
            ledState = HIGH;  
        } 
        else 
        {
            ledState = LOW;  
        }

        digitalWrite(LED_BUILTIN, ledState);
    }

}

/******************************************************************************************************************************
		
******************************************************************************************************************************/